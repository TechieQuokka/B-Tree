#ifndef BTREE_GENERIC_H
#define BTREE_GENERIC_H

#include "btree_types.h"
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 기본 타입에 대한 비교 함수 생성 매크로 */
#define BTREE_DEFINE_COMPARE(type, suffix) \
    BTREE_INLINE int btree_compare_##suffix(const void *a, const void *b) { \
        const type *ta = (const type *)a; \
        const type *tb = (const type *)b; \
        return (*ta > *tb) - (*ta < *tb); \
    }

/* 기본 타입에 대한 복사 함수 생성 매크로 */
#define BTREE_DEFINE_COPY(type, suffix) \
    BTREE_INLINE void btree_copy_##suffix(void *dest, const void *src, size_t count) { \
        memcpy(dest, src, sizeof(type) * count); \
    }

/* 기본 타입에 대한 이동 함수 생성 매크로 */
#define BTREE_DEFINE_MOVE(type, suffix) \
    BTREE_INLINE void btree_move_##suffix(void *dest, const void *src, size_t count) { \
        memmove(dest, src, sizeof(type) * count); \
    }

/* 기본 타입에 대한 교환 함수 생성 매크로 */
#define BTREE_DEFINE_SWAP(type, suffix) \
    BTREE_INLINE void btree_swap_##suffix(void *a, void *b) { \
        type temp = *(type*)a; \
        *(type*)a = *(type*)b; \
        *(type*)b = temp; \
    }

/* 기본 타입에 대한 출력 함수 생성 매크로 */
#define BTREE_DEFINE_PRINT(type, suffix, format_spec) \
    BTREE_INLINE void btree_print_##suffix(const void *ptr, FILE *output) { \
        if (BTREE_LIKELY(ptr && output)) { \
            fprintf(output, format_spec, *(const type*)ptr); \
        } \
    }

/* 모든 기본 연산을 한번에 정의하는 매크로 */
#define BTREE_DEFINE_BASIC_OPS(type, suffix) \
    BTREE_DEFINE_COMPARE(type, suffix) \
    BTREE_DEFINE_COPY(type, suffix) \
    BTREE_DEFINE_MOVE(type, suffix) \
    BTREE_DEFINE_SWAP(type, suffix)

/* 숫자 타입에 대한 특화 연산 */
#define BTREE_DEFINE_NUMERIC_OPS(type, suffix, format_spec) \
    BTREE_DEFINE_BASIC_OPS(type, suffix) \
    BTREE_DEFINE_PRINT(type, suffix, format_spec) \
    \
    BTREE_INLINE bool btree_is_zero_##suffix(const void *ptr) { \
        return *(const type*)ptr == 0; \
    } \
    \
    BTREE_INLINE void btree_set_zero_##suffix(void *ptr) { \
        *(type*)ptr = 0; \
    }

/* 포인터 타입을 위한 특화 매크로 */
#define BTREE_DEFINE_POINTER_OPS(pointed_type, suffix, compare_func, copy_func, destroy_func) \
    BTREE_INLINE int btree_compare_##suffix(const void *a, const void *b) { \
        const pointed_type * const *pa = (const pointed_type * const *)a; \
        const pointed_type * const *pb = (const pointed_type * const *)b; \
        return compare_func(*pa, *pb); \
    } \
    \
    BTREE_INLINE void btree_copy_##suffix(void *dest, const void *src, size_t count) { \
        pointed_type **d = (pointed_type**)dest; \
        const pointed_type * const *s = (const pointed_type * const *)src; \
        for (size_t i = 0; i < count; i++) { \
            d[i] = copy_func ? copy_func(s[i]) : (pointed_type*)s[i]; \
        } \
    } \
    \
    BTREE_INLINE void btree_destroy_##suffix(void *ptr, size_t count) { \
        if (destroy_func) { \
            pointed_type **p = (pointed_type**)ptr; \
            for (size_t i = 0; i < count; i++) { \
                if (p[i]) { \
                    destroy_func(p[i]); \
                    p[i] = NULL; \
                } \
            } \
        } \
    }

/* 문자열을 위한 특화 매크로 */
#define BTREE_DEFINE_STRING_OPS(suffix) \
    BTREE_INLINE int btree_compare_##suffix(const void *a, const void *b) { \
        const char * const *sa = (const char * const *)a; \
        const char * const *sb = (const char * const *)b; \
        return strcmp(*sa, *sb); \
    } \
    \
    BTREE_INLINE void btree_copy_##suffix(void *dest, const void *src, size_t count) { \
        char **d = (char**)dest; \
        const char * const *s = (const char * const *)src; \
        for (size_t i = 0; i < count; i++) { \
            d[i] = strdup(s[i]); \
        } \
    } \
    \
    BTREE_INLINE void btree_destroy_##suffix(void *ptr, size_t count) { \
        char **p = (char**)ptr; \
        for (size_t i = 0; i < count; i++) { \
            if (p[i]) { \
                free(p[i]); \
                p[i] = NULL; \
            } \
        } \
    } \
    \
    BTREE_INLINE void btree_print_##suffix(const void *ptr, FILE *output) { \
        if (ptr && output) { \
            const char * const *s = (const char * const *)ptr; \
            fprintf(output, "\"%s\"", *s ? *s : "(null)"); \
        } \
    }

/* 타입별 B-Tree 구조체 선언 매크로 */
#define BTREE_DECLARE(KEY_TYPE, VALUE_TYPE, SUFFIX) \
    typedef struct btree_##SUFFIX { \
        btree_t base; \
    } btree_##SUFFIX##_t; \
    \
    typedef struct btree_##SUFFIX##_iterator { \
        btree_iterator_t base; \
        KEY_TYPE current_key; \
        VALUE_TYPE current_value; \
    } btree_##SUFFIX##_iterator_t; \
    \
    /* 생성 및 소멸 함수 */ \
    btree_##SUFFIX##_t* btree_##SUFFIX##_create(int degree); \
    btree_##SUFFIX##_t* btree_##SUFFIX##_create_with_allocator(int degree, btree_allocator_t *allocator); \
    void btree_##SUFFIX##_destroy(btree_##SUFFIX##_t *tree); \
    \
    /* 기본 연산 함수 */ \
    btree_result_t btree_##SUFFIX##_insert(btree_##SUFFIX##_t *tree, KEY_TYPE key, VALUE_TYPE value); \
    VALUE_TYPE* btree_##SUFFIX##_search(btree_##SUFFIX##_t *tree, KEY_TYPE key); \
    btree_result_t btree_##SUFFIX##_delete(btree_##SUFFIX##_t *tree, KEY_TYPE key); \
    bool btree_##SUFFIX##_contains(btree_##SUFFIX##_t *tree, KEY_TYPE key); \
    \
    /* 유틸리티 함수 */ \
    size_t btree_##SUFFIX##_size(btree_##SUFFIX##_t *tree); \
    int btree_##SUFFIX##_height(btree_##SUFFIX##_t *tree); \
    bool btree_##SUFFIX##_is_empty(btree_##SUFFIX##_t *tree); \
    void btree_##SUFFIX##_clear(btree_##SUFFIX##_t *tree); \
    \
    /* 반복자 함수 */ \
    btree_##SUFFIX##_iterator_t* btree_##SUFFIX##_iterator_create(btree_##SUFFIX##_t *tree); \
    bool btree_##SUFFIX##_iterator_next(btree_##SUFFIX##_iterator_t *iter, KEY_TYPE *key, VALUE_TYPE *value); \
    bool btree_##SUFFIX##_iterator_prev(btree_##SUFFIX##_iterator_t *iter, KEY_TYPE *key, VALUE_TYPE *value); \
    void btree_##SUFFIX##_iterator_destroy(btree_##SUFFIX##_iterator_t *iter); \
    \
    /* 디버깅 함수 */ \
    void btree_##SUFFIX##_print(btree_##SUFFIX##_t *tree, FILE *output); \
    bool btree_##SUFFIX##_validate(btree_##SUFFIX##_t *tree); \
    void btree_##SUFFIX##_print_stats(btree_##SUFFIX##_t *tree, FILE *output);

/* 타입별 함수 구현 생성 매크로 */
#define BTREE_DEFINE(KEY_TYPE, VALUE_TYPE, SUFFIX, KEY_OPS_SUFFIX, VALUE_OPS_SUFFIX) \
    /* 타입 정보 초기화 함수 */ \
    static btree_type_info_t btree_##SUFFIX##_key_type_info = { \
        .key_size = sizeof(KEY_TYPE), \
        .value_size = 0, \
        .alignment = _Alignof(KEY_TYPE), \
        .type_name = #KEY_TYPE, \
        .type_id = BTREE_TYPE_ID(KEY_TYPE), \
        .compare = btree_compare_##KEY_OPS_SUFFIX, \
        .copy = (btree_copy_func_t)btree_copy_##KEY_OPS_SUFFIX, \
        .move = (btree_copy_func_t)btree_move_##KEY_OPS_SUFFIX, \
        .destroy = NULL \
    }; \
    \
    static btree_type_info_t btree_##SUFFIX##_value_type_info = { \
        .key_size = 0, \
        .value_size = sizeof(VALUE_TYPE), \
        .alignment = _Alignof(VALUE_TYPE), \
        .type_name = #VALUE_TYPE, \
        .type_id = BTREE_TYPE_ID(VALUE_TYPE), \
        .compare = btree_compare_##VALUE_OPS_SUFFIX, \
        .copy = (btree_copy_func_t)btree_copy_##VALUE_OPS_SUFFIX, \
        .move = (btree_copy_func_t)btree_move_##VALUE_OPS_SUFFIX, \
        .destroy = NULL \
    }; \
    \
    /* 생성 함수 */ \
    btree_##SUFFIX##_t* btree_##SUFFIX##_create(int degree) { \
        return btree_##SUFFIX##_create_with_allocator(degree, NULL); \
    } \
    \
    btree_##SUFFIX##_t* btree_##SUFFIX##_create_with_allocator(int degree, btree_allocator_t *allocator) { \
        btree_##SUFFIX##_t *tree = malloc(sizeof(btree_##SUFFIX##_t)); \
        if (BTREE_UNLIKELY(!tree)) return NULL; \
        \
        if (btree_init(&tree->base, degree, &btree_##SUFFIX##_key_type_info, \
                      &btree_##SUFFIX##_value_type_info, allocator) != BTREE_SUCCESS) { \
            free(tree); \
            return NULL; \
        } \
        \
        return tree; \
    } \
    \
    /* 소멸 함수 */ \
    void btree_##SUFFIX##_destroy(btree_##SUFFIX##_t *tree) { \
        if (tree) { \
            btree_cleanup(&tree->base); \
            free(tree); \
        } \
    } \
    \
    /* 삽입 함수 */ \
    btree_result_t btree_##SUFFIX##_insert(btree_##SUFFIX##_t *tree, KEY_TYPE key, VALUE_TYPE value) { \
        BTREE_TYPE_CHECK(&tree->base, KEY_TYPE); \
        return btree_insert(&tree->base, &key, &value); \
    } \
    \
    /* 검색 함수 */ \
    VALUE_TYPE* btree_##SUFFIX##_search(btree_##SUFFIX##_t *tree, KEY_TYPE key) { \
        BTREE_TYPE_CHECK(&tree->base, KEY_TYPE); \
        return (VALUE_TYPE*)btree_search(&tree->base, &key); \
    } \
    \
    /* 삭제 함수 */ \
    btree_result_t btree_##SUFFIX##_delete(btree_##SUFFIX##_t *tree, KEY_TYPE key) { \
        BTREE_TYPE_CHECK(&tree->base, KEY_TYPE); \
        return btree_delete(&tree->base, &key); \
    } \
    \
    /* 포함 여부 확인 함수 */ \
    bool btree_##SUFFIX##_contains(btree_##SUFFIX##_t *tree, KEY_TYPE key) { \
        return btree_##SUFFIX##_search(tree, key) != NULL; \
    } \
    \
    /* 유틸리티 함수들 구현 */ \
    size_t btree_##SUFFIX##_size(btree_##SUFFIX##_t *tree) { \
        return tree ? tree->base.key_count : 0; \
    } \
    \
    int btree_##SUFFIX##_height(btree_##SUFFIX##_t *tree) { \
        return tree ? tree->base.height : 0; \
    } \
    \
    bool btree_##SUFFIX##_is_empty(btree_##SUFFIX##_t *tree) { \
        return tree ? (tree->base.root == NULL) : true; \
    } \
    \
    void btree_##SUFFIX##_clear(btree_##SUFFIX##_t *tree) { \
        if (tree) { \
            btree_clear(&tree->base); \
        } \
    }

/* 컴파일 타임 최적화를 위한 매크로 */
#define BTREE_SMALL_TYPE_THRESHOLD 16

#define BTREE_OPTIMIZE_SMALL_TYPE(type, suffix) \
    _Static_assert(sizeof(type) <= BTREE_SMALL_TYPE_THRESHOLD, \
                   "Type too large for small type optimization"); \
    \
    BTREE_INLINE void btree_fast_copy_##suffix(void *dest, const void *src, size_t count) { \
        if (BTREE_LIKELY(count == 1)) { \
            *(type*)dest = *(const type*)src; \
        } else { \
            memcpy(dest, src, sizeof(type) * count); \
        } \
    }

/* 커스텀 할당자 지원 매크로 */
#define BTREE_DEFINE_CUSTOM_ALLOCATOR(type, suffix, alloc_func, free_func) \
    static void* btree_##suffix##_custom_alloc(size_t size) { \
        return alloc_func(size); \
    } \
    \
    static void btree_##suffix##_custom_free(void *ptr) { \
        free_func(ptr); \
    } \
    \
    static btree_allocator_t btree_##suffix##_custom_allocator = { \
        .alloc = btree_##suffix##_custom_alloc, \
        .free = btree_##suffix##_custom_free, \
        .realloc = NULL, \
        .context = NULL, \
        .total_allocated = 0, \
        .total_freed = 0 \
    }; \
    \
    btree_##suffix##_t* btree_##suffix##_create_custom(int degree) { \
        return btree_##suffix##_create_with_allocator(degree, &btree_##suffix##_custom_allocator); \
    }

/* 디버깅 지원 매크로 */
#ifdef BTREE_DEBUG
    #define BTREE_DEFINE_DEBUG_OPS(type, suffix, format_spec) \
        void btree_##suffix##_print(btree_##suffix##_t *tree, FILE *output) { \
            if (tree && output) { \
                fprintf(output, "B-Tree<%s> (degree=%d, height=%d, size=%zu)\n", \
                        #type, tree->base.degree, tree->base.height, tree->base.key_count); \
                btree_print_structure(&tree->base, output); \
            } \
        } \
        \
        bool btree_##suffix##_validate(btree_##suffix##_t *tree) { \
            return tree ? btree_validate_structure(&tree->base) : false; \
        } \
        \
        void btree_##suffix##_print_stats(btree_##suffix##_t *tree, FILE *output) { \
            if (tree && output) { \
                btree_print_statistics(&tree->base, output); \
            } \
        }
#else
    #define BTREE_DEFINE_DEBUG_OPS(type, suffix, format_spec) \
        void btree_##suffix##_print(btree_##suffix##_t *tree, FILE *output) { \
            (void)tree; (void)output; \
        } \
        \
        bool btree_##suffix##_validate(btree_##suffix##_t *tree) { \
            (void)tree; return true; \
        } \
        \
        void btree_##suffix##_print_stats(btree_##suffix##_t *tree, FILE *output) { \
            (void)tree; (void)output; \
        }
#endif

/* 기본 타입들에 대한 연산 정의 */
BTREE_DEFINE_NUMERIC_OPS(int, int, "%d")
BTREE_DEFINE_NUMERIC_OPS(long, long, "%ld")
BTREE_DEFINE_NUMERIC_OPS(long long, llong, "%lld")
BTREE_DEFINE_NUMERIC_OPS(unsigned int, uint, "%u")
BTREE_DEFINE_NUMERIC_OPS(unsigned long, ulong, "%lu")
BTREE_DEFINE_NUMERIC_OPS(unsigned long long, ullong, "%llu")
BTREE_DEFINE_NUMERIC_OPS(float, float, "%f")
BTREE_DEFINE_NUMERIC_OPS(double, double, "%lf")

/* 문자열 타입 연산 정의 */
BTREE_DEFINE_STRING_OPS(string)

/* 포인터 타입 연산 정의 */
BTREE_INLINE int btree_compare_ptr(const void *a, const void *b) {
    const void * const *pa = (const void * const *)a;
    const void * const *pb = (const void * const *)b;
    return (*pa > *pb) - (*pa < *pb);
}

BTREE_DEFINE_COPY(void*, ptr)
BTREE_DEFINE_MOVE(void*, ptr)
BTREE_DEFINE_SWAP(void*, ptr)

#ifdef __cplusplus
}
#endif

#endif /* BTREE_GENERIC_H */