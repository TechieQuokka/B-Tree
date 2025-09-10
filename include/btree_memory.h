#ifndef BTREE_MEMORY_H
#define BTREE_MEMORY_H

#include "btree_types.h"
#include <stdlib.h>

/* C99 호환성을 위해 atomic 타입을 간단한 타입으로 정의 */
#ifndef atomic_size_t
typedef size_t atomic_size_t;
typedef int atomic_flag;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 메모리 풀 설정 */
#define BTREE_DEFAULT_POOL_SIZE        (1024 * 1024)  /* 1MB */
#define BTREE_MIN_POOL_SIZE            (64 * 1024)    /* 64KB */
#define BTREE_MAX_POOL_SIZE            (64 * 1024 * 1024) /* 64MB */
#define BTREE_POOL_ALIGNMENT           64
#define BTREE_MAX_POOLS                8

/* 메모리 풀 통계 구조체 */
typedef struct {
    size_t total_size;                  /* 풀 전체 크기 */
    size_t used_size;                   /* 사용된 크기 */
    size_t free_size;                   /* 여유 크기 */
    size_t block_size;                  /* 블록 크기 */
    size_t total_blocks;                /* 전체 블록 수 */
    size_t used_blocks;                 /* 사용된 블록 수 */
    size_t free_blocks;                 /* 여유 블록 수 */
    size_t peak_usage;                  /* 최대 사용량 */
    size_t allocation_count;            /* 할당 횟수 */
    size_t deallocation_count;          /* 해제 횟수 */
    double fragmentation_ratio;         /* 단편화 비율 */
} btree_pool_stats_t;

/* 메모리 풀 구조체 */
typedef struct btree_memory_pool {
    /* 풀 기본 정보 */
    void *pool_start;                   /* 풀 시작 주소 */
    size_t pool_size;                   /* 풀 전체 크기 */
    size_t block_size;                  /* 고정 블록 크기 */
    size_t total_blocks;                /* 전체 블록 수 */
    size_t alignment;                   /* 메모리 정렬 */
    
    /* 자유 블록 관리 */
    void **free_list;                   /* 자유 블록 리스트 */
    size_t free_count;                  /* 자유 블록 수 */
    void *next_free;                    /* 다음 자유 블록 */
    
    /* 통계 정보 */
    btree_pool_stats_t stats;           /* 풀 통계 */
    
    /* 동시성 제어 */
    atomic_flag lock;                   /* 스핀락 */
    
    /* 설정 플래그 */
    uint32_t flags;                     /* 풀 설정 플래그 */
    
    /* 다음 풀 (연결 리스트) */
    struct btree_memory_pool *next;
} btree_memory_pool_t;

/* 메모리 풀 플래그 */
#define BTREE_POOL_FLAG_THREAD_SAFE    0x01
#define BTREE_POOL_FLAG_ZERO_MEMORY    0x02
#define BTREE_POOL_FLAG_DEBUG_MODE     0x04
#define BTREE_POOL_FLAG_TRACK_STATS    0x08

/* 메모리 매니저 구조체 */
typedef struct {
    /* 풀 관리 */
    btree_memory_pool_t *pools[BTREE_MAX_POOLS];  /* 크기별 풀 배열 */
    size_t pool_count;                  /* 활성 풀 수 */
    
    /* 큰 할당을 위한 일반 할당자 */
    btree_allocator_t *fallback_allocator;
    size_t large_allocation_threshold;  /* 큰 할당 기준 크기 */
    
    /* 전역 통계 */
    atomic_size_t total_allocated;      /* 총 할당된 메모리 */
    atomic_size_t total_freed;          /* 총 해제된 메모리 */
    atomic_size_t peak_usage;           /* 최대 사용량 */
    atomic_size_t current_usage;        /* 현재 사용량 */
    
    /* 설정 */
    uint32_t flags;                     /* 전역 플래그 */
    
    /* 동시성 제어 */
    atomic_flag manager_lock;           /* 매니저 레벨 락 */
} btree_memory_manager_t;

/* 메모리 풀 생성 및 관리 함수 */
btree_memory_pool_t* btree_pool_create(size_t block_size, size_t pool_size, uint32_t flags);
void btree_pool_destroy(btree_memory_pool_t *pool);
void* btree_pool_alloc(btree_memory_pool_t *pool);
void btree_pool_free(btree_memory_pool_t *pool, void *ptr);
bool btree_pool_contains(btree_memory_pool_t *pool, const void *ptr);
void btree_pool_get_stats(btree_memory_pool_t *pool, btree_pool_stats_t *stats);
void btree_pool_reset(btree_memory_pool_t *pool);

/* 메모리 매니저 함수 */
btree_memory_manager_t* btree_memory_manager_create(void);
void btree_memory_manager_destroy(btree_memory_manager_t *manager);
void* btree_memory_manager_alloc(btree_memory_manager_t *manager, size_t size);
void btree_memory_manager_free(btree_memory_manager_t *manager, void *ptr);
void* btree_memory_manager_realloc(btree_memory_manager_t *manager, void *ptr, size_t new_size);

/* 기본 메모리 할당자 구현 */
btree_allocator_t* btree_default_allocator(void);
btree_allocator_t* btree_pool_allocator_create(size_t block_size, size_t pool_size);
btree_allocator_t* btree_debug_allocator_create(btree_allocator_t *base_allocator);

/* 메모리 디버깅 및 추적 */
void btree_memory_print_stats(FILE *output);
bool btree_memory_check_leaks(void);
void btree_memory_set_debug_mode(bool enable);
size_t btree_memory_get_usage(void);

/* 인라인 유틸리티 함수들 */
BTREE_INLINE size_t btree_align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

BTREE_INLINE bool btree_is_power_of_two(size_t n) {
    return n && !(n & (n - 1));
}

BTREE_INLINE size_t btree_next_power_of_two(size_t n) {
    if (btree_is_power_of_two(n)) return n;
    
    size_t power = 1;
    while (power < n) {
        power <<= 1;
    }
    return power;
}

/* 캐시 친화적 메모리 할당 - 간단한 버전 */
BTREE_INLINE void* btree_cache_aligned_alloc(size_t size) {
    size_t aligned_size = btree_align_size(size, BTREE_CACHE_LINE_SIZE);
    return malloc(aligned_size);  /* 간단하게 malloc 사용 */
}

BTREE_INLINE void btree_cache_aligned_free(void *ptr) {
    free(ptr);
}

/* 메모리 워킹 세트 관리 */
typedef struct {
    void *base_addr;                    /* 기준 주소 */
    size_t working_set_size;            /* 워킹 세트 크기 */
    size_t page_size;                   /* 페이지 크기 */
    uint32_t access_pattern;            /* 접근 패턴 */
} btree_memory_locality_hint_t;

void btree_memory_prefetch(const void *addr, size_t size);
void btree_memory_set_locality_hint(const btree_memory_locality_hint_t *hint);
void btree_memory_optimize_layout(void *base, size_t size);

/* 메모리 압축 및 최적화 */
typedef struct {
    size_t original_size;               /* 원본 크기 */
    size_t compressed_size;             /* 압축된 크기 */
    float compression_ratio;            /* 압축률 */
    uint32_t algorithm;                 /* 압축 알고리즘 */
} btree_compression_info_t;

bool btree_memory_compress(const void *src, size_t src_size, void *dst, 
                          size_t *dst_size, btree_compression_info_t *info);
bool btree_memory_decompress(const void *src, size_t src_size, void *dst, 
                            size_t dst_size, const btree_compression_info_t *info);

/* NUMA 지원 (Non-Uniform Memory Access) */
#ifdef BTREE_NUMA_SUPPORT
typedef struct {
    int node_id;                        /* NUMA 노드 ID */
    size_t local_memory;                /* 로컬 메모리 크기 */
    size_t remote_memory;               /* 원격 메모리 크기 */
    double access_latency;              /* 접근 지연시간 */
} btree_numa_info_t;

bool btree_numa_get_info(btree_numa_info_t *info);
void* btree_numa_alloc_local(size_t size);
void* btree_numa_alloc_interleaved(size_t size);
void btree_numa_free(void *ptr);
#endif

/* 메모리 보안 기능 */
void btree_memory_secure_zero(void *ptr, size_t size);
bool btree_memory_is_readable(const void *ptr, size_t size);
bool btree_memory_is_writable(void *ptr, size_t size);
void btree_memory_protect(void *ptr, size_t size, int protection);

/* 메모리 풀 최적화 힌트 */
typedef enum {
    BTREE_ALLOC_HINT_SMALL_FREQUENT,   /* 작고 빈번한 할당 */
    BTREE_ALLOC_HINT_LARGE_INFREQUENT, /* 크고 드문 할당 */
    BTREE_ALLOC_HINT_SEQUENTIAL,       /* 순차 접근 */
    BTREE_ALLOC_HINT_RANDOM,           /* 무작위 접근 */
    BTREE_ALLOC_HINT_TEMPORARY,        /* 임시 할당 */
    BTREE_ALLOC_HINT_PERSISTENT        /* 지속적 할당 */
} btree_alloc_hint_t;

void btree_memory_set_alloc_hint(btree_alloc_hint_t hint);
btree_allocator_t* btree_optimized_allocator_create(btree_alloc_hint_t hint);

/* 메모리 풀 자동 조정 */
typedef struct {
    size_t min_pool_size;               /* 최소 풀 크기 */
    size_t max_pool_size;               /* 최대 풀 크기 */
    float growth_factor;                /* 증가 비율 */
    float shrink_threshold;             /* 축소 임계값 */
    size_t measurement_window;          /* 측정 윈도우 크기 */
} btree_pool_auto_tune_t;

void btree_pool_enable_auto_tune(btree_memory_pool_t *pool, 
                                const btree_pool_auto_tune_t *config);
void btree_pool_disable_auto_tune(btree_memory_pool_t *pool);
void btree_pool_manual_resize(btree_memory_pool_t *pool, size_t new_size);

#ifdef __cplusplus
}
#endif

#endif /* BTREE_MEMORY_H */