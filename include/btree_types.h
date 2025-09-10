#ifndef BTREE_TYPES_H
#define BTREE_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 기본 상수 정의 */
#define BTREE_MIN_DEGREE           3
#define BTREE_MAX_DEGREE           1024
#define BTREE_DEFAULT_DEGREE       16
#define BTREE_CACHE_LINE_SIZE      64

/* 오류 코드 정의 */
typedef enum {
    BTREE_SUCCESS = 0,
    BTREE_ERROR_NULL_POINTER,
    BTREE_ERROR_INVALID_DEGREE,
    BTREE_ERROR_MEMORY_ALLOCATION,
    BTREE_ERROR_KEY_NOT_FOUND,
    BTREE_ERROR_DUPLICATE_KEY,
    BTREE_ERROR_INVALID_OPERATION,
    BTREE_ERROR_TYPE_MISMATCH,
    BTREE_ERROR_INVALID_SIZE,
    BTREE_ERROR_ALIGNMENT_ERROR
} btree_result_t;

/* 전방 선언 */
typedef struct btree_node btree_node_t;
typedef struct btree btree_t;
typedef struct btree_iterator btree_iterator_t;
typedef struct btree_type_info btree_type_info_t;
typedef struct btree_allocator btree_allocator_t;

/* 함수 포인터 타입 정의 */
typedef int (*btree_compare_func_t)(const void *a, const void *b);
typedef void (*btree_copy_func_t)(void *dest, const void *src, size_t count);
typedef void (*btree_destroy_func_t)(void *ptr, size_t count);
typedef uint64_t (*btree_hash_func_t)(const void *ptr);
typedef void (*btree_print_func_t)(const void *ptr, FILE *output);
typedef bool (*btree_validate_func_t)(const void *ptr);

/* 메모리 할당자 함수 포인터 */
typedef void* (*btree_alloc_func_t)(size_t size);
typedef void (*btree_free_func_t)(void *ptr);
typedef void* (*btree_realloc_func_t)(void *ptr, size_t new_size);

/* 타입 정보 구조체 */
struct btree_type_info {
    /* 기본 타입 메타데이터 */
    size_t key_size;                    /* 키 타입 크기 */
    size_t value_size;                  /* 값 타입 크기 */
    size_t alignment;                   /* 메모리 정렬 요구사항 */
    const char *type_name;              /* 타입 이름 (디버깅용) */
    uint32_t type_id;                   /* 고유 타입 식별자 */
    
    /* 기본 연산 함수들 */
    btree_compare_func_t compare;       /* 비교 함수 */
    btree_copy_func_t copy;             /* 복사 함수 */
    btree_copy_func_t move;             /* 이동 함수 */
    btree_destroy_func_t destroy;       /* 소멸 함수 */
    
    /* 선택적 연산 함수들 */
    btree_hash_func_t hash;             /* 해시 함수 */
    btree_print_func_t print;           /* 출력 함수 */
    btree_validate_func_t validate;     /* 유효성 검사 함수 */
};

/* 메모리 할당자 구조체 */
struct btree_allocator {
    btree_alloc_func_t alloc;           /* 할당 함수 */
    btree_free_func_t free;             /* 해제 함수 */
    btree_realloc_func_t realloc;       /* 재할당 함수 */
    void *context;                      /* 할당자 컨텍스트 */
    size_t total_allocated;             /* 총 할당된 메모리 */
    size_t total_freed;                 /* 총 해제된 메모리 */
};

/* B-Tree 노드 구조체 */
struct btree_node {
    /* 노드 기본 정보 */
    uint32_t is_leaf : 1;               /* 리프 노드 여부 */
    uint32_t num_keys : 31;             /* 현재 키 개수 */
    
    /* 데이터 포인터 */
    void *keys;                         /* 키 배열 */
    void *values;                       /* 값 배열 (리프 노드용) */
    btree_node_t **children;            /* 자식 노드 배열 */
    
    /* 트리 구조 정보 */
    btree_node_t *parent;               /* 부모 노드 */
    
    /* B+Tree를 위한 추가 필드 */
    btree_node_t *next_leaf;            /* 다음 리프 노드 */
    btree_node_t *prev_leaf;            /* 이전 리프 노드 */
    
    /* 메모리 관리 정보 */
    size_t capacity;                    /* 최대 키 용량 */
    uint32_t ref_count;                 /* 참조 카운트 */
};

/* 메인 B-Tree 구조체 */
struct btree {
    /* 트리 기본 정보 */
    btree_node_t *root;                 /* 루트 노드 */
    int degree;                         /* B-Tree 차수 */
    int max_keys;                       /* 최대 키 개수 */
    int min_keys;                       /* 최소 키 개수 */
    int height;                         /* 트리 높이 */
    
    /* 타입 정보 */
    btree_type_info_t key_type;         /* 키 타입 정보 */
    btree_type_info_t value_type;       /* 값 타입 정보 */
    
    /* 메모리 관리 */
    btree_allocator_t *allocator;       /* 메모리 할당자 */
    
    /* 통계 정보 */
    size_t node_count;                  /* 전체 노드 수 */
    size_t key_count;                   /* 전체 키 수 */
    size_t total_memory;                /* 총 메모리 사용량 */
    
    /* 설정 플래그 */
    uint32_t flags;                     /* 설정 플래그들 */
    
    /* 동기화 (향후 멀티스레드 지원용) */
    void *lock;                         /* 동기화 객체 */
};

/* B-Tree 설정 플래그 */
#define BTREE_FLAG_ALLOW_DUPLICATES    0x01
#define BTREE_FLAG_CASE_INSENSITIVE    0x02
#define BTREE_FLAG_AUTO_BALANCE        0x04
#define BTREE_FLAG_THREAD_SAFE         0x08

/* 반복자 구조체 */
struct btree_iterator {
    btree_t *tree;                      /* 대상 트리 */
    btree_node_t *current_node;         /* 현재 노드 */
    int current_index;                  /* 현재 인덱스 */
    bool is_valid;                      /* 반복자 유효성 */
    bool is_reverse;                    /* 역방향 반복 여부 */
};

/* 타입 ID 생성 매크로 */
#define BTREE_TYPE_ID(type) \
    ((uint32_t)(sizeof(type) ^ (size_t)#type[0] ^ __LINE__))

/* 디버그 모드 타입 검사 매크로 */
#ifdef BTREE_DEBUG
    #define BTREE_TYPE_CHECK(tree, expected_type) \
        do { \
            if ((tree)->key_type.type_id != BTREE_TYPE_ID(expected_type)) { \
                fprintf(stderr, "Type mismatch: expected %s at %s:%d\n", \
                       #expected_type, __FILE__, __LINE__); \
                abort(); \
            } \
        } while(0)
#else
    #define BTREE_TYPE_CHECK(tree, type) ((void)0)
#endif

/* 메모리 정렬 매크로 */
#define BTREE_ALIGN(size, alignment) \
    (((size) + (alignment) - 1) & ~((alignment) - 1))

/* 최적 차수 계산 매크로 */
#define BTREE_OPTIMAL_DEGREE(key_size, value_size) \
    ((BTREE_CACHE_LINE_SIZE - sizeof(btree_node_t)) / \
     ((key_size) + (value_size) + sizeof(void*)))

/* 컴파일러별 속성 정의 */
#if defined(__GNUC__) || defined(__clang__)
    #define BTREE_INLINE static inline __attribute__((always_inline))
    #define BTREE_LIKELY(x) __builtin_expect(!!(x), 1)
    #define BTREE_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define BTREE_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define BTREE_INLINE static __forceinline
    #define BTREE_LIKELY(x) (x)
    #define BTREE_UNLIKELY(x) (x)
    #define BTREE_RESTRICT __restrict
#else
    #define BTREE_INLINE static inline
    #define BTREE_LIKELY(x) (x)
    #define BTREE_UNLIKELY(x) (x)
    #define BTREE_RESTRICT
#endif

/* C++ 지원 */
#ifdef __cplusplus
}
#endif

#endif /* BTREE_TYPES_H */