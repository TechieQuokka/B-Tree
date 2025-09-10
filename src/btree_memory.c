/**
 * @file btree_memory.c
 * @brief B-Tree 메모리 관리 시스템 구현
 */

#include "../include/btree_memory.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* C99 호환성을 위해 atomic 연산을 간단한 연산으로 대체 */
#define atomic_size_t size_t
#define atomic_flag int
#define atomic_fetch_add(ptr, val) (*(ptr) += (val))
#define atomic_fetch_sub(ptr, val) (*(ptr) -= (val))
#define atomic_load(ptr) (*(ptr))
#define atomic_store(ptr, val) (*(ptr) = (val))
#define atomic_compare_exchange_weak(ptr, expected, desired) \
    (*(ptr) == *(expected) ? (*(ptr) = (desired), 1) : (*(expected) = *(ptr), 0))
#define atomic_flag_test_and_set(ptr) (*(ptr) ? 1 : (*(ptr) = 1, 0))
#define atomic_flag_clear(ptr) (*(ptr) = 0)

#ifdef BTREE_PLATFORM_WINDOWS
#include <windows.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#endif

#ifdef BTREE_PLATFORM_LINUX
#include <sys/mman.h>
#include <unistd.h>
#endif

/* 전역 메모리 통계 */
static struct {
    atomic_size_t total_allocated;
    atomic_size_t total_freed;
    atomic_size_t peak_usage;
    atomic_size_t current_usage;
    atomic_flag lock;
} g_memory_stats = {0};

/* 기본 할당자 구현 - 간단한 버전 */
static void* default_alloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        g_memory_stats.current_usage += size;
        g_memory_stats.total_allocated += size;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
    }
    return ptr;
}

static void default_free(void *ptr) {
    if (ptr) {
        /* 간단하게 malloc으로 할당된 크기를 추적하지 않음 */
        free(ptr);
        g_memory_stats.total_freed += sizeof(void*);  /* 대략적인 크기 */
    }
}

static void* default_realloc(void *ptr, size_t new_size) {
    void *new_ptr = realloc(ptr, new_size);
    /* 간단한 통계만 유지 */
    if (new_ptr) {
        g_memory_stats.total_allocated += new_size;
    }
    return new_ptr;
}

static btree_allocator_t g_default_allocator = {
    .alloc = default_alloc,
    .free = default_free,
    .realloc = default_realloc,
    .context = NULL,
    .total_allocated = 0,
    .total_freed = 0
};

/**
 * @brief 기본 할당자 반환
 */
btree_allocator_t* btree_default_allocator(void) {
    return &g_default_allocator;
}

/**
 * @brief 메모리 풀 생성
 */
btree_memory_pool_t* btree_pool_create(size_t block_size, size_t pool_size, uint32_t flags) {
    if (block_size == 0 || pool_size < BTREE_MIN_POOL_SIZE || pool_size > BTREE_MAX_POOL_SIZE) {
        return NULL;
    }
    
    /* 블록 크기와 풀 크기를 정렬 */
    block_size = btree_align_size(block_size, BTREE_POOL_ALIGNMENT);
    pool_size = btree_align_size(pool_size, BTREE_CACHE_LINE_SIZE);
    
    /* 풀 구조체 할당 */
    btree_memory_pool_t *pool = malloc(sizeof(btree_memory_pool_t));
    if (!pool) return NULL;
    
    memset(pool, 0, sizeof(btree_memory_pool_t));
    
    /* 풀 메모리 할당 */
    pool->pool_start = btree_cache_aligned_alloc(pool_size);
    if (!pool->pool_start) {
        free(pool);
        return NULL;
    }
    
    /* 풀 기본 정보 설정 */
    pool->pool_size = pool_size;
    pool->block_size = block_size;
    pool->total_blocks = pool_size / block_size;
    pool->alignment = BTREE_POOL_ALIGNMENT;
    pool->flags = flags;
    
    /* 자유 블록 리스트 초기화 */
    pool->free_list = malloc(pool->total_blocks * sizeof(void*));
    if (!pool->free_list) {
        btree_cache_aligned_free(pool->pool_start);
        free(pool);
        return NULL;
    }
    
    /* 모든 블록을 자유 리스트에 추가 */
    for (size_t i = 0; i < pool->total_blocks; i++) {
        pool->free_list[i] = (char*)pool->pool_start + (i * block_size);
    }
    pool->free_count = pool->total_blocks;
    pool->next_free = pool->pool_start;
    
    /* 통계 초기화 */
    pool->stats.total_size = pool_size;
    pool->stats.free_size = pool_size;
    pool->stats.block_size = block_size;
    pool->stats.total_blocks = pool->total_blocks;
    pool->stats.free_blocks = pool->total_blocks;
    
    /* 스핀락 초기화 */
    atomic_flag_clear(&pool->lock);
    
    return pool;
}

/**
 * @brief 메모리 풀 소멸
 */
void btree_pool_destroy(btree_memory_pool_t *pool) {
    if (!pool) return;
    
    /* 스핀락 획득 */
    while (atomic_flag_test_and_set(&pool->lock)) {
        /* 대기 */
    }
    
    /* 메모리 해제 */
    if (pool->pool_start) {
        btree_cache_aligned_free(pool->pool_start);
    }
    if (pool->free_list) {
        free(pool->free_list);
    }
    
    atomic_flag_clear(&pool->lock);
    free(pool);
}

/**
 * @brief 풀에서 메모리 할당
 */
void* btree_pool_alloc(btree_memory_pool_t *pool) {
    if (!pool || pool->free_count == 0) return NULL;
    
    /* 스핀락 획득 */
    if (pool->flags & BTREE_POOL_FLAG_THREAD_SAFE) {
        while (atomic_flag_test_and_set(&pool->lock)) {
            /* 스핀 대기 */
        }
    }
    
    void *ptr = NULL;
    if (pool->free_count > 0) {
        /* 자유 리스트에서 블록 가져오기 */
        ptr = pool->free_list[--pool->free_count];
        
        /* 메모리 초기화 */
        if (pool->flags & BTREE_POOL_FLAG_ZERO_MEMORY) {
            memset(ptr, 0, pool->block_size);
        }
        
        /* 통계 업데이트 */
        pool->stats.used_blocks++;
        pool->stats.free_blocks--;
        pool->stats.used_size += pool->block_size;
        pool->stats.free_size -= pool->block_size;
        pool->stats.allocation_count++;
        
        if (pool->stats.used_size > pool->stats.peak_usage) {
            pool->stats.peak_usage = pool->stats.used_size;
        }
    }
    
    /* 스핀락 해제 */
    if (pool->flags & BTREE_POOL_FLAG_THREAD_SAFE) {
        atomic_flag_clear(&pool->lock);
    }
    
    return ptr;
}

/**
 * @brief 풀에 메모리 반환
 */
void btree_pool_free(btree_memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr || !btree_pool_contains(pool, ptr)) return;
    
    /* 스핀락 획득 */
    if (pool->flags & BTREE_POOL_FLAG_THREAD_SAFE) {
        while (atomic_flag_test_and_set(&pool->lock)) {
            /* 스핀 대기 */
        }
    }
    
    /* 자유 리스트에 블록 반환 */
    if (pool->free_count < pool->total_blocks) {
        pool->free_list[pool->free_count++] = ptr;
        
        /* 통계 업데이트 */
        pool->stats.used_blocks--;
        pool->stats.free_blocks++;
        pool->stats.used_size -= pool->block_size;
        pool->stats.free_size += pool->block_size;
        pool->stats.deallocation_count++;
        
        /* 단편화 계산 */
        pool->stats.fragmentation_ratio = 
            (double)(pool->stats.total_blocks - pool->stats.free_blocks) / pool->stats.total_blocks;
    }
    
    /* 스핀락 해제 */
    if (pool->flags & BTREE_POOL_FLAG_THREAD_SAFE) {
        atomic_flag_clear(&pool->lock);
    }
}

/**
 * @brief 포인터가 풀에 속하는지 확인
 */
bool btree_pool_contains(btree_memory_pool_t *pool, const void *ptr) {
    if (!pool || !ptr) return false;
    
    const char *start = (const char*)pool->pool_start;
    const char *end = start + pool->pool_size;
    const char *check = (const char*)ptr;
    
    return (check >= start && check < end);
}

/**
 * @brief 풀 통계 수집
 */
void btree_pool_get_stats(btree_memory_pool_t *pool, btree_pool_stats_t *stats) {
    if (!pool || !stats) return;
    
    /* 스핀락 획득 */
    if (pool->flags & BTREE_POOL_FLAG_THREAD_SAFE) {
        while (atomic_flag_test_and_set(&pool->lock)) {
            /* 스핀 대기 */
        }
    }
    
    *stats = pool->stats;
    
    /* 스핀락 해제 */
    if (pool->flags & BTREE_POOL_FLAG_THREAD_SAFE) {
        atomic_flag_clear(&pool->lock);
    }
}

/**
 * @brief 풀 리셋
 */
void btree_pool_reset(btree_memory_pool_t *pool) {
    if (!pool) return;
    
    /* 스핀락 획득 */
    while (atomic_flag_test_and_set(&pool->lock)) {
        /* 스핀 대기 */
    }
    
    /* 모든 블록을 자유 리스트로 되돌림 */
    for (size_t i = 0; i < pool->total_blocks; i++) {
        pool->free_list[i] = (char*)pool->pool_start + (i * pool->block_size);
    }
    pool->free_count = pool->total_blocks;
    
    /* 통계 리셋 */
    pool->stats.used_size = 0;
    pool->stats.free_size = pool->pool_size;
    pool->stats.used_blocks = 0;
    pool->stats.free_blocks = pool->total_blocks;
    pool->stats.fragmentation_ratio = 0.0;
    
    atomic_flag_clear(&pool->lock);
}

/**
 * @brief 메모리 매니저 생성
 */
btree_memory_manager_t* btree_memory_manager_create(void) {
    btree_memory_manager_t *manager = malloc(sizeof(btree_memory_manager_t));
    if (!manager) return NULL;
    
    memset(manager, 0, sizeof(btree_memory_manager_t));
    
    /* 기본 설정 */
    manager->fallback_allocator = btree_default_allocator();
    manager->large_allocation_threshold = 64 * 1024;  /* 64KB */
    
    /* 통계 초기화 */
    atomic_store(&manager->total_allocated, 0);
    atomic_store(&manager->total_freed, 0);
    atomic_store(&manager->peak_usage, 0);
    atomic_store(&manager->current_usage, 0);
    
    /* 스핀락 초기화 */
    atomic_flag_clear(&manager->manager_lock);
    
    return manager;
}

/**
 * @brief 메모리 매니저 소멸
 */
void btree_memory_manager_destroy(btree_memory_manager_t *manager) {
    if (!manager) return;
    
    /* 모든 풀 소멸 */
    for (size_t i = 0; i < manager->pool_count; i++) {
        if (manager->pools[i]) {
            btree_pool_destroy(manager->pools[i]);
        }
    }
    
    free(manager);
}

/**
 * @brief 적절한 풀 찾기
 */
static btree_memory_pool_t* find_suitable_pool(btree_memory_manager_t *manager, size_t size) {
    for (size_t i = 0; i < manager->pool_count; i++) {
        btree_memory_pool_t *pool = manager->pools[i];
        if (pool && pool->block_size >= size && pool->free_count > 0) {
            return pool;
        }
    }
    return NULL;
}

/**
 * @brief 매니저에서 메모리 할당
 */
void* btree_memory_manager_alloc(btree_memory_manager_t *manager, size_t size) {
    if (!manager || size == 0) return NULL;
    
    /* 큰 할당은 fallback 할당자 사용 */
    if (size > manager->large_allocation_threshold) {
        void *ptr = manager->fallback_allocator->alloc(size);
        if (ptr) {
            atomic_fetch_add(&manager->total_allocated, size);
            atomic_fetch_add(&manager->current_usage, size);
        }
        return ptr;
    }
    
    /* 적절한 풀 찾기 */
    btree_memory_pool_t *pool = find_suitable_pool(manager, size);
    if (pool) {
        void *ptr = btree_pool_alloc(pool);
        if (ptr) {
            atomic_fetch_add(&manager->total_allocated, pool->block_size);
            atomic_fetch_add(&manager->current_usage, pool->block_size);
        }
        return ptr;
    }
    
    /* 적절한 풀이 없으면 새 풀 생성 */
    if (manager->pool_count < BTREE_MAX_POOLS) {
        size_t block_size = btree_next_power_of_two(size);
        size_t pool_size = BTREE_DEFAULT_POOL_SIZE;
        
        btree_memory_pool_t *new_pool = btree_pool_create(block_size, pool_size, 
                                                         BTREE_POOL_FLAG_THREAD_SAFE);
        if (new_pool) {
            manager->pools[manager->pool_count++] = new_pool;
            
            void *ptr = btree_pool_alloc(new_pool);
            if (ptr) {
                atomic_fetch_add(&manager->total_allocated, block_size);
                atomic_fetch_add(&manager->current_usage, block_size);
            }
            return ptr;
        }
    }
    
    /* 모든 방법이 실패하면 fallback 할당자 사용 */
    void *ptr = manager->fallback_allocator->alloc(size);
    if (ptr) {
        atomic_fetch_add(&manager->total_allocated, size);
        atomic_fetch_add(&manager->current_usage, size);
    }
    return ptr;
}

/**
 * @brief 매니저에서 메모리 해제
 */
void btree_memory_manager_free(btree_memory_manager_t *manager, void *ptr) {
    if (!manager || !ptr) return;
    
    /* 어느 풀에 속하는지 확인 */
    for (size_t i = 0; i < manager->pool_count; i++) {
        btree_memory_pool_t *pool = manager->pools[i];
        if (pool && btree_pool_contains(pool, ptr)) {
            btree_pool_free(pool, ptr);
            manager->total_freed += pool->block_size;
            manager->current_usage -= pool->block_size;
            return;
        }
    }
    
    /* 풀에 속하지 않으면 fallback 할당자 사용 */
    manager->fallback_allocator->free(ptr);
    manager->total_freed += sizeof(void*);  /* 대략적인 크기 */
}

/**
 * @brief 풀 할당자 생성
 */
btree_allocator_t* btree_pool_allocator_create(size_t block_size, size_t pool_size) {
    btree_memory_pool_t *pool = btree_pool_create(block_size, pool_size, 
                                                 BTREE_POOL_FLAG_THREAD_SAFE);
    if (!pool) return NULL;
    
    btree_allocator_t *allocator = malloc(sizeof(btree_allocator_t));
    if (!allocator) {
        btree_pool_destroy(pool);
        return NULL;
    }
    
    allocator->alloc = (btree_alloc_func_t)btree_pool_alloc;
    allocator->free = (btree_free_func_t)btree_pool_free;
    allocator->realloc = NULL;  /* 풀은 재할당 지원하지 않음 */
    allocator->context = pool;
    allocator->total_allocated = 0;
    allocator->total_freed = 0;
    
    return allocator;
}

/**
 * @brief 메모리 프리페치
 */
void btree_memory_prefetch(const void *addr, size_t size) {
    if (!addr) return;
    
#if defined(__GNUC__) || defined(__clang__)
    /* GCC/Clang 내장 함수 사용 */
    const char *ptr = (const char*)addr;
    const char *end = ptr + size;
    
    while (ptr < end) {
        __builtin_prefetch(ptr, 0, 3);  /* 읽기 위한 프리페치 */
        ptr += BTREE_CACHE_LINE_SIZE;
    }
#elif defined(_MSC_VER)
    /* MSVC 내장 함수 사용 */
    const char *ptr = (const char*)addr;
    const char *end = ptr + size;
    
    while (ptr < end) {
        _mm_prefetch(ptr, _MM_HINT_T0);
        ptr += BTREE_CACHE_LINE_SIZE;
    }
#endif
}

/**
 * @brief 현재 메모리 사용량 반환
 */
size_t btree_memory_get_usage(void) {
    return atomic_load(&g_memory_stats.current_usage);
}

/**
 * @brief 메모리 통계 출력
 */
void btree_memory_print_stats(FILE *output) {
    if (!output) return;
    
    size_t total_allocated = atomic_load(&g_memory_stats.total_allocated);
    size_t total_freed = atomic_load(&g_memory_stats.total_freed);
    size_t current_usage = atomic_load(&g_memory_stats.current_usage);
    size_t peak_usage = atomic_load(&g_memory_stats.peak_usage);
    
    fprintf(output, "Memory Statistics:\n");
    fprintf(output, "  Total Allocated: %zu bytes\n", total_allocated);
    fprintf(output, "  Total Freed:     %zu bytes\n", total_freed);
    fprintf(output, "  Current Usage:   %zu bytes\n", current_usage);
    fprintf(output, "  Peak Usage:      %zu bytes\n", peak_usage);
    fprintf(output, "  Efficiency:      %.2f%%\n", 
           total_allocated > 0 ? (100.0 * total_freed / total_allocated) : 0.0);
}

/**
 * @brief 메모리 누수 확인
 */
bool btree_memory_check_leaks(void) {
    size_t current_usage = atomic_load(&g_memory_stats.current_usage);
    return current_usage > 0;
}

/**
 * @brief 보안 메모리 초기화
 */
void btree_memory_secure_zero(void *ptr, size_t size) {
    if (!ptr || size == 0) return;
    
    /* 컴파일러 최적화 방지를 위한 volatile 포인터 사용 */
    volatile unsigned char *p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
}