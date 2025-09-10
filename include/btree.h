#ifndef BTREE_H
#define BTREE_H

/**
 * @file btree.h
 * @brief C언어 기반 일반화된 B-Tree 라이브러리
 * 
 * 이 라이브러리는 타입에 독립적인 B-Tree 자료구조를 제공합니다.
 * 매크로와 함수 포인터를 조합하여 타입 안전성과 성능을 동시에 보장합니다.
 * 
 * @version 1.0.0
 * @author B-Tree Project Team
 * @date 2024
 */

#include "btree_types.h"
#include "btree_generic.h"
#include "btree_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 라이브러리 버전 정보 */
#define BTREE_VERSION_MAJOR     1
#define BTREE_VERSION_MINOR     0
#define BTREE_VERSION_PATCH     0
#define BTREE_VERSION_STRING    "1.0.0"

/* 컴파일 타임 설정 */
#ifndef BTREE_DEFAULT_DEGREE
#define BTREE_DEFAULT_DEGREE    16
#endif

#ifndef BTREE_MAX_HEIGHT
#define BTREE_MAX_HEIGHT        20
#endif

/* B-Tree 초기화 및 정리 함수 */
btree_result_t btree_init(btree_t *tree, int degree,
                         const btree_type_info_t *key_type,
                         const btree_type_info_t *value_type,
                         btree_allocator_t *allocator);
void btree_cleanup(btree_t *tree);

/* 핵심 B-Tree 연산 함수 */
btree_result_t btree_insert(btree_t *tree, const void *key, const void *value);
void* btree_search(btree_t *tree, const void *key);
btree_result_t btree_delete(btree_t *tree, const void *key);
bool btree_contains(btree_t *tree, const void *key);

/* 범위 검색 함수 */
typedef struct {
    void *key;
    void *value;
} btree_key_value_pair_t;

size_t btree_range_search(btree_t *tree, const void *min_key, const void *max_key,
                         btree_key_value_pair_t *results, size_t max_results);
size_t btree_prefix_search(btree_t *tree, const void *prefix, size_t prefix_len,
                          btree_key_value_pair_t *results, size_t max_results);

/* 유틸리티 함수 */
size_t btree_size(const btree_t *tree);
int btree_height(const btree_t *tree);
bool btree_is_empty(const btree_t *tree);
void btree_clear(btree_t *tree);
btree_result_t btree_copy(btree_t *dest, const btree_t *src);

/* 반복자 함수 */
btree_iterator_t* btree_iterator_create(btree_t *tree);
btree_iterator_t* btree_iterator_create_range(btree_t *tree, 
                                             const void *min_key, 
                                             const void *max_key);
bool btree_iterator_next(btree_iterator_t *iter, void **key, void **value);
bool btree_iterator_prev(btree_iterator_t *iter, void **key, void **value);
bool btree_iterator_has_next(const btree_iterator_t *iter);
bool btree_iterator_has_prev(const btree_iterator_t *iter);
void btree_iterator_reset(btree_iterator_t *iter);
void btree_iterator_destroy(btree_iterator_t *iter);

/* 노드 관리 함수 */
btree_node_t* btree_node_create(btree_t *tree, bool is_leaf);
void btree_node_destroy(btree_t *tree, btree_node_t *node);
btree_result_t btree_node_insert_key(btree_node_t *node, int index,
                                    const void *key, const void *value,
                                    const btree_type_info_t *key_type,
                                    const btree_type_info_t *value_type);
btree_result_t btree_node_remove_key(btree_node_t *node, int index,
                                    const btree_type_info_t *key_type,
                                    const btree_type_info_t *value_type);
int btree_node_find_key(const btree_node_t *node, const void *key,
                       const btree_type_info_t *key_type);

/* 노드 분할 및 합병 */
btree_result_t btree_split_node(btree_t *tree, btree_node_t *node,
                               btree_node_t **new_node, void **split_key);
btree_result_t btree_merge_nodes(btree_t *tree, btree_node_t *left,
                                btree_node_t *right, const void *sep_key);
btree_result_t btree_redistribute_keys(btree_t *tree, btree_node_t *left,
                                      btree_node_t *right, const void *sep_key);

/* 디버깅 및 검증 함수 */
bool btree_validate_structure(const btree_t *tree);
bool btree_validate_node(const btree_node_t *node, const btree_type_info_t *key_type);
void btree_print_structure(const btree_t *tree, FILE *output);
void btree_print_node(const btree_node_t *node, const btree_type_info_t *key_type,
                     const btree_type_info_t *value_type, FILE *output, int depth);

/* 통계 정보 */
typedef struct {
    size_t node_count;                  /* 노드 수 */
    size_t key_count;                   /* 키 수 */
    size_t leaf_count;                  /* 리프 노드 수 */
    size_t internal_count;              /* 내부 노드 수 */
    int height;                         /* 트리 높이 */
    double fill_factor;                 /* 공간 활용률 */
    size_t memory_usage;                /* 메모리 사용량 */
    size_t wasted_space;                /* 낭비된 공간 */
} btree_statistics_t;

void btree_collect_statistics(const btree_t *tree, btree_statistics_t *stats);
void btree_print_statistics(const btree_t *tree, FILE *output);

/* 직렬화 및 역직렬화 */
size_t btree_serialize_size(const btree_t *tree);
btree_result_t btree_serialize(const btree_t *tree, void *buffer, size_t buffer_size);
btree_result_t btree_deserialize(btree_t *tree, const void *buffer, size_t buffer_size);

/* 파일 I/O */
btree_result_t btree_save_to_file(const btree_t *tree, const char *filename);
btree_result_t btree_load_from_file(btree_t *tree, const char *filename);

/* B-Tree 변형 지원 */
typedef enum {
    BTREE_VARIANT_STANDARD,             /* 표준 B-Tree */
    BTREE_VARIANT_PLUS,                 /* B+Tree */
    BTREE_VARIANT_STAR,                 /* B*Tree */
    BTREE_VARIANT_CONCURRENT            /* 동시성 B-Tree */
} btree_variant_t;

btree_result_t btree_set_variant(btree_t *tree, btree_variant_t variant);
btree_variant_t btree_get_variant(const btree_t *tree);

/* 성능 최적화 */
void btree_optimize_layout(btree_t *tree);
void btree_compact(btree_t *tree);
void btree_rebuild(btree_t *tree);
void btree_set_cache_hint(btree_t *tree, btree_alloc_hint_t hint);

/* 배치 연산 */
btree_result_t btree_bulk_insert(btree_t *tree, 
                                const btree_key_value_pair_t *pairs, 
                                size_t count);
size_t btree_bulk_delete(btree_t *tree, const void **keys, size_t count);

/* 트랜잭션 지원 (기본) */
typedef struct btree_transaction btree_transaction_t;

btree_transaction_t* btree_transaction_begin(btree_t *tree);
btree_result_t btree_transaction_commit(btree_transaction_t *tx);
btree_result_t btree_transaction_rollback(btree_transaction_t *tx);

/* 콜백 및 이벤트 */
typedef enum {
    BTREE_EVENT_INSERT,
    BTREE_EVENT_DELETE,
    BTREE_EVENT_SEARCH,
    BTREE_EVENT_SPLIT,
    BTREE_EVENT_MERGE
} btree_event_type_t;

typedef void (*btree_event_callback_t)(btree_t *tree, btree_event_type_t event,
                                      const void *key, const void *value, 
                                      void *context);

btree_result_t btree_set_event_callback(btree_t *tree, btree_event_type_t event,
                                       btree_event_callback_t callback, void *context);
btree_result_t btree_remove_event_callback(btree_t *tree, btree_event_type_t event);

/* 라이브러리 초기화 및 정리 */
btree_result_t btree_library_init(void);
void btree_library_cleanup(void);
const char* btree_version_string(void);
int btree_version_major(void);
int btree_version_minor(void);
int btree_version_patch(void);

/* 오류 처리 */
const char* btree_error_string(btree_result_t error);
btree_result_t btree_get_last_error(void);
void btree_set_error_handler(void (*handler)(btree_result_t error, const char *message));

/* 편의 매크로 */

/**
 * @brief 기본 정수형 B-Tree를 위한 편의 매크로
 * 
 * 사용 예:
 * @code
 * BTREE_DECLARE_INT_INT(my_tree);
 * BTREE_DEFINE_INT_INT(my_tree);
 * 
 * btree_my_tree_t *tree = btree_my_tree_create(16);
 * btree_my_tree_insert(tree, 42, 84);
 * int *value = btree_my_tree_search(tree, 42);
 * @endcode
 */
#define BTREE_DECLARE_INT_INT(suffix) \
    BTREE_DECLARE(int, int, suffix)

#define BTREE_DEFINE_INT_INT(suffix) \
    BTREE_DEFINE(int, int, suffix, int, int) \
    BTREE_DEFINE_DEBUG_OPS(int, suffix, "%d")

/**
 * @brief 문자열-문자열 B-Tree를 위한 편의 매크로
 */
#define BTREE_DECLARE_STRING_STRING(suffix) \
    BTREE_DECLARE(char*, char*, suffix)

#define BTREE_DEFINE_STRING_STRING(suffix) \
    BTREE_DEFINE(char*, char*, suffix, string, string) \
    BTREE_DEFINE_DEBUG_OPS(char*, suffix, "%s")

/**
 * @brief 포인터 B-Tree를 위한 편의 매크로
 */
#define BTREE_DECLARE_PTR_PTR(suffix) \
    BTREE_DECLARE(void*, void*, suffix)

#define BTREE_DEFINE_PTR_PTR(suffix) \
    BTREE_DEFINE(void*, void*, suffix, ptr, ptr) \
    BTREE_DEFINE_DEBUG_OPS(void*, suffix, "%p")

/* 빌드 설정 검증 */
#if BTREE_MIN_DEGREE < 2
#error "BTREE_MIN_DEGREE must be at least 2"
#endif

#if BTREE_MAX_DEGREE <= BTREE_MIN_DEGREE
#error "BTREE_MAX_DEGREE must be greater than BTREE_MIN_DEGREE"
#endif

#if BTREE_DEFAULT_DEGREE < BTREE_MIN_DEGREE || BTREE_DEFAULT_DEGREE > BTREE_MAX_DEGREE
#error "BTREE_DEFAULT_DEGREE must be between BTREE_MIN_DEGREE and BTREE_MAX_DEGREE"
#endif

/* 플랫폼별 최적화 힌트 */
#ifdef __linux__
#define BTREE_PLATFORM_LINUX 1
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define BTREE_PLATFORM_WINDOWS 1
#include <windows.h>
#include <malloc.h>
#endif

#ifdef __APPLE__
#define BTREE_PLATFORM_MACOS 1
#include <mach/mach.h>
#include <sys/mman.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* BTREE_H */