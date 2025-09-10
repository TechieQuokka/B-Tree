# C언어 일반화 프로그래밍 설계 명세서

## 1. 개요

### 1.1 목적
본 문서는 B-Tree 라이브러리에서 사용할 C언어 일반화 프로그래밍 기법의 설계 명세를 정의합니다. C언어의 제약사항 하에서 타입 안전성과 코드 재사용성을 동시에 보장하는 일반화 시스템을 제시합니다.

### 1.2 설계 목표
- **타입 안전성**: 컴파일 타임에 타입 오류 검출
- **성능 효율성**: 런타임 오버헤드 최소화
- **코드 재사용성**: 단일 코드베이스로 다양한 타입 지원
- **가독성**: 명확하고 이해하기 쉬운 API
- **확장성**: 새로운 타입 쉽게 추가 가능

### 1.3 접근 방식
C언어에서 일반화를 구현하기 위해 다음 기법들을 조합 사용합니다:
1. 매크로 기반 코드 생성
2. 함수 포인터를 통한 동적 디스패치
3. void 포인터와 타입 정보 구조체
4. 조건부 컴파일을 통한 최적화

## 2. 타입 시스템 설계

### 2.1 기본 타입 정보 구조체

```c
typedef struct btree_type_info {
    // 타입 메타데이터
    size_t key_size;                    // 키 타입 크기
    size_t value_size;                  // 값 타입 크기
    size_t alignment;                   // 메모리 정렬 요구사항
    const char *type_name;              // 타입 이름 (디버깅용)
    uint32_t type_id;                   // 고유 타입 식별자
    
    // 기본 연산 함수들
    int (*compare)(const void *a, const void *b);
    void (*copy)(void *dest, const void *src, size_t count);
    void (*move)(void *dest, void *src, size_t count);
    void (*swap)(void *a, void *b);
    void (*destroy)(void *ptr, size_t count);
    
    // 해시 및 직렬화 (선택사항)
    uint64_t (*hash)(const void *ptr);
    size_t (*serialize)(const void *ptr, void *buffer, size_t buffer_size);
    size_t (*deserialize)(void *ptr, const void *buffer, size_t buffer_size);
    
    // 디버깅 지원
    void (*print)(const void *ptr, FILE *output);
    bool (*validate)(const void *ptr);
} btree_type_info_t;
```

### 2.2 타입 안전성 보장 매크로

```c
// 타입 ID 생성 매크로
#define BTREE_TYPE_ID(type) \
    ((uint32_t)(__LINE__ ^ (size_t)#type[0] ^ sizeof(type)))

// 타입 검사 매크로
#define BTREE_TYPE_CHECK(tree, expected_type) \
    do { \
        if ((tree)->type_info->type_id != BTREE_TYPE_ID(expected_type)) { \
            fprintf(stderr, "Type mismatch: expected %s\n", #expected_type); \
            abort(); \
        } \
    } while(0)

// 디버그 모드에서만 타입 검사 수행
#ifdef BTREE_DEBUG
    #define BTREE_SAFE_TYPE_CHECK(tree, type) BTREE_TYPE_CHECK(tree, type)
#else
    #define BTREE_SAFE_TYPE_CHECK(tree, type) ((void)0)
#endif
```

### 2.3 기본 타입별 연산 함수 생성

```c
// 기본 타입에 대한 비교 함수 생성 매크로
#define BTREE_DEFINE_COMPARE(type, suffix) \
    static inline int btree_compare_##suffix(const void *a, const void *b) { \
        const type *ta = (const type *)a; \
        const type *tb = (const type *)b; \
        return (*ta > *tb) - (*ta < *tb); \
    }

// 기본 타입에 대한 복사 함수 생성
#define BTREE_DEFINE_COPY(type, suffix) \
    static inline void btree_copy_##suffix(void *dest, const void *src, size_t count) { \
        memcpy(dest, src, sizeof(type) * count); \
    }

// 기본 타입에 대한 이동 함수 생성
#define BTREE_DEFINE_MOVE(type, suffix) \
    static inline void btree_move_##suffix(void *dest, void *src, size_t count) { \
        memmove(dest, src, sizeof(type) * count); \
    }

// 기본 타입에 대한 교환 함수 생성
#define BTREE_DEFINE_SWAP(type, suffix) \
    static inline void btree_swap_##suffix(void *a, void *b) { \
        type temp = *(type*)a; \
        *(type*)a = *(type*)b; \
        *(type*)b = temp; \
    }

// 모든 기본 연산을 한번에 정의
#define BTREE_DEFINE_BASIC_OPS(type, suffix) \
    BTREE_DEFINE_COMPARE(type, suffix) \
    BTREE_DEFINE_COPY(type, suffix) \
    BTREE_DEFINE_MOVE(type, suffix) \
    BTREE_DEFINE_SWAP(type, suffix)
```

## 3. 매크로 기반 타입별 인터페이스

### 3.1 타입별 B-Tree 구조체 생성

```c
#define BTREE_DECLARE(KEY_TYPE, VALUE_TYPE, SUFFIX) \
    typedef struct btree_##SUFFIX { \
        btree_t base;                   /* 베이스 B-Tree 구조 */ \
        btree_type_info_t key_type;     /* 키 타입 정보 */ \
        btree_type_info_t value_type;   /* 값 타입 정보 */ \
    } btree_##SUFFIX##_t; \
    \
    /* 타입별 함수 선언들 */ \
    btree_##SUFFIX##_t* btree_##SUFFIX##_create(int degree); \
    void btree_##SUFFIX##_destroy(btree_##SUFFIX##_t *tree); \
    bool btree_##SUFFIX##_insert(btree_##SUFFIX##_t *tree, \
                                 KEY_TYPE key, VALUE_TYPE value); \
    VALUE_TYPE* btree_##SUFFIX##_search(btree_##SUFFIX##_t *tree, \
                                       KEY_TYPE key); \
    bool btree_##SUFFIX##_delete(btree_##SUFFIX##_t *tree, KEY_TYPE key); \
    \
    /* 반복자 타입 및 함수들 */ \
    typedef struct btree_##SUFFIX##_iterator { \
        btree_iterator_t base; \
        KEY_TYPE current_key; \
        VALUE_TYPE current_value; \
    } btree_##SUFFIX##_iterator_t; \
    \
    btree_##SUFFIX##_iterator_t* btree_##SUFFIX##_iterator_create(btree_##SUFFIX##_t *tree); \
    bool btree_##SUFFIX##_iterator_next(btree_##SUFFIX##_iterator_t *iter, \
                                       KEY_TYPE *key, VALUE_TYPE *value); \
    void btree_##SUFFIX##_iterator_destroy(btree_##SUFFIX##_iterator_t *iter);
```

### 3.2 타입별 함수 구현 생성

```c
#define BTREE_DEFINE(KEY_TYPE, VALUE_TYPE, SUFFIX, KEY_COMPARE, VALUE_COMPARE) \
    /* 타입 정보 초기화 함수 */ \
    static void init_##SUFFIX##_type_info(btree_type_info_t *key_info, \
                                          btree_type_info_t *value_info) { \
        /* 키 타입 정보 */ \
        key_info->key_size = sizeof(KEY_TYPE); \
        key_info->value_size = 0; \
        key_info->alignment = _Alignof(KEY_TYPE); \
        key_info->type_name = #KEY_TYPE; \
        key_info->type_id = BTREE_TYPE_ID(KEY_TYPE); \
        key_info->compare = KEY_COMPARE; \
        key_info->copy = btree_copy_##SUFFIX##_key; \
        key_info->move = btree_move_##SUFFIX##_key; \
        key_info->swap = btree_swap_##SUFFIX##_key; \
        key_info->destroy = NULL; /* 기본 타입은 소멸자 불필요 */ \
        \
        /* 값 타입 정보 */ \
        value_info->key_size = 0; \
        value_info->value_size = sizeof(VALUE_TYPE); \
        value_info->alignment = _Alignof(VALUE_TYPE); \
        value_info->type_name = #VALUE_TYPE; \
        value_info->type_id = BTREE_TYPE_ID(VALUE_TYPE); \
        value_info->compare = VALUE_COMPARE; \
        value_info->copy = btree_copy_##SUFFIX##_value; \
        value_info->move = btree_move_##SUFFIX##_value; \
        value_info->swap = btree_swap_##SUFFIX##_value; \
        value_info->destroy = NULL; \
    } \
    \
    /* 생성 함수 */ \
    btree_##SUFFIX##_t* btree_##SUFFIX##_create(int degree) { \
        btree_##SUFFIX##_t *tree = malloc(sizeof(btree_##SUFFIX##_t)); \
        if (!tree) return NULL; \
        \
        init_##SUFFIX##_type_info(&tree->key_type, &tree->value_type); \
        \
        if (!btree_init(&tree->base, degree, &tree->key_type, &tree->value_type)) { \
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
    bool btree_##SUFFIX##_insert(btree_##SUFFIX##_t *tree, \
                                 KEY_TYPE key, VALUE_TYPE value) { \
        BTREE_SAFE_TYPE_CHECK(tree, KEY_TYPE); \
        return btree_insert(&tree->base, &key, &value); \
    } \
    \
    /* 검색 함수 */ \
    VALUE_TYPE* btree_##SUFFIX##_search(btree_##SUFFIX##_t *tree, KEY_TYPE key) { \
        BTREE_SAFE_TYPE_CHECK(tree, KEY_TYPE); \
        return (VALUE_TYPE*)btree_search(&tree->base, &key); \
    } \
    \
    /* 삭제 함수 */ \
    bool btree_##SUFFIX##_delete(btree_##SUFFIX##_t *tree, KEY_TYPE key) { \
        BTREE_SAFE_TYPE_CHECK(tree, KEY_TYPE); \
        return btree_delete(&tree->base, &key); \
    }
```

### 3.3 복잡한 타입을 위한 확장 매크로

```c
// 포인터 타입을 위한 특별한 처리
#define BTREE_DEFINE_POINTER_TYPE(POINTED_TYPE, SUFFIX, COMPARE_FUNC, COPY_FUNC, DESTROY_FUNC) \
    static void btree_copy_##SUFFIX(void *dest, const void *src, size_t count) { \
        POINTED_TYPE **d = (POINTED_TYPE**)dest; \
        const POINTED_TYPE * const *s = (const POINTED_TYPE * const *)src; \
        for (size_t i = 0; i < count; i++) { \
            d[i] = COPY_FUNC(s[i]); \
        } \
    } \
    \
    static void btree_destroy_##SUFFIX(void *ptr, size_t count) { \
        POINTED_TYPE **p = (POINTED_TYPE**)ptr; \
        for (size_t i = 0; i < count; i++) { \
            if (p[i]) { \
                DESTROY_FUNC(p[i]); \
                p[i] = NULL; \
            } \
        } \
    } \
    \
    static btree_type_info_t btree_type_info_##SUFFIX = { \
        .key_size = sizeof(POINTED_TYPE*), \
        .value_size = sizeof(POINTED_TYPE*), \
        .alignment = _Alignof(POINTED_TYPE*), \
        .type_name = #POINTED_TYPE "*", \
        .type_id = BTREE_TYPE_ID(POINTED_TYPE*), \
        .compare = COMPARE_FUNC, \
        .copy = btree_copy_##SUFFIX, \
        .move = NULL, /* 포인터는 단순 이동 */ \
        .swap = btree_swap_pointer, \
        .destroy = btree_destroy_##SUFFIX \
    };

// 문자열을 위한 특화 매크로
#define BTREE_DEFINE_STRING_TYPE(SUFFIX) \
    BTREE_DEFINE_POINTER_TYPE(char, SUFFIX, strcmp_wrapper, strdup, free)
```

## 4. 컴파일 타임 최적화

### 4.1 조건부 컴파일을 통한 최적화

```c
// 작은 타입에 대한 최적화
#define BTREE_SMALL_TYPE_THRESHOLD 16

#define BTREE_OPTIMIZE_SMALL_TYPE(type, suffix) \
    _Static_assert(sizeof(type) <= BTREE_SMALL_TYPE_THRESHOLD, \
                   "Type too large for small type optimization"); \
    \
    static inline void btree_fast_copy_##suffix(void *dest, const void *src, size_t count) { \
        if (count == 1) { \
            *(type*)dest = *(const type*)src; \
        } else { \
            memcpy(dest, src, sizeof(type) * count); \
        } \
    }

// 숫자 타입에 대한 특화
#define BTREE_OPTIMIZE_NUMERIC(type, suffix) \
    static inline bool btree_is_zero_##suffix(const void *ptr) { \
        return *(const type*)ptr == 0; \
    } \
    \
    static inline void btree_set_zero_##suffix(void *ptr) { \
        *(type*)ptr = 0; \
    }
```

### 4.2 템플릿 유사 인라인 함수

```c
// 타입별 인라인 함수 생성 매크로
#define BTREE_INLINE_SEARCH(type, suffix) \
    static inline type* btree_inline_search_##suffix(btree_##suffix##_t *tree, type key) { \
        btree_node_t *node = tree->base.root; \
        \
        while (node) { \
            type *keys = (type*)node->keys; \
            int left = 0, right = node->num_keys - 1; \
            \
            /* 이진 검색 */ \
            while (left <= right) { \
                int mid = (left + right) / 2; \
                if (keys[mid] == key) { \
                    return (type*)node->values + mid; \
                } else if (keys[mid] < key) { \
                    left = mid + 1; \
                } else { \
                    right = mid - 1; \
                } \
            } \
            \
            if (node->is_leaf) break; \
            node = node->children[left]; \
        } \
        \
        return NULL; \
    }
```

## 5. 메모리 관리 통합

### 5.1 타입별 메모리 풀

```c
typedef struct btree_typed_allocator {
    btree_allocator_t base;             // 기본 할당자
    size_t element_size;                // 요소 크기
    size_t alignment;                   // 정렬 요구사항
    size_t pool_size;                   // 풀 크기
    void *type_pool;                    // 타입별 메모리 풀
    size_t allocated_count;             // 할당된 요소 수
    size_t free_count;                  // 여유 요소 수
} btree_typed_allocator_t;

#define BTREE_CREATE_TYPED_ALLOCATOR(type, suffix, pool_size) \
    static btree_typed_allocator_t* create_##suffix##_allocator(void) { \
        btree_typed_allocator_t *alloc = malloc(sizeof(btree_typed_allocator_t)); \
        if (!alloc) return NULL; \
        \
        alloc->element_size = sizeof(type); \
        alloc->alignment = _Alignof(type); \
        alloc->pool_size = pool_size; \
        \
        size_t total_size = sizeof(type) * pool_size; \
        alloc->type_pool = aligned_alloc(alloc->alignment, total_size); \
        if (!alloc->type_pool) { \
            free(alloc); \
            return NULL; \
        } \
        \
        /* 자유 리스트 초기화 */ \
        init_free_list_##suffix(alloc); \
        \
        return alloc; \
    }
```

### 5.2 스마트 포인터 유사 기능

```c
// 참조 카운팅을 위한 래퍼
typedef struct btree_ref_counted {
    void *data;
    size_t ref_count;
    void (*destructor)(void *);
} btree_ref_counted_t;

#define BTREE_DEFINE_SMART_PTR(type, suffix) \
    typedef struct btree_smart_##suffix { \
        btree_ref_counted_t *ref; \
    } btree_smart_##suffix##_t; \
    \
    static btree_smart_##suffix##_t btree_make_smart_##suffix(type *data) { \
        btree_ref_counted_t *ref = malloc(sizeof(btree_ref_counted_t)); \
        ref->data = data; \
        ref->ref_count = 1; \
        ref->destructor = free; \
        return (btree_smart_##suffix##_t){.ref = ref}; \
    } \
    \
    static void btree_smart_retain_##suffix(btree_smart_##suffix##_t *smart) { \
        if (smart->ref) { \
            smart->ref->ref_count++; \
        } \
    } \
    \
    static void btree_smart_release_##suffix(btree_smart_##suffix##_t *smart) { \
        if (smart->ref && --smart->ref->ref_count == 0) { \
            smart->ref->destructor(smart->ref->data); \
            free(smart->ref); \
            smart->ref = NULL; \
        } \
    }
```

## 6. 오류 처리 및 디버깅

### 6.1 타입별 오류 처리

```c
typedef enum {
    BTREE_TYPE_ERROR_NONE = 0,
    BTREE_TYPE_ERROR_NULL_POINTER,
    BTREE_TYPE_ERROR_TYPE_MISMATCH,
    BTREE_TYPE_ERROR_INVALID_SIZE,
    BTREE_TYPE_ERROR_ALIGNMENT_ERROR,
    BTREE_TYPE_ERROR_CONVERSION_FAILED
} btree_type_error_t;

#define BTREE_DEFINE_ERROR_HANDLING(suffix) \
    static thread_local btree_type_error_t last_##suffix##_error = BTREE_TYPE_ERROR_NONE; \
    static thread_local char last_##suffix##_error_msg[256] = {0}; \
    \
    static void set_##suffix##_error(btree_type_error_t error, const char *msg) { \
        last_##suffix##_error = error; \
        strncpy(last_##suffix##_error_msg, msg, sizeof(last_##suffix##_error_msg) - 1); \
    } \
    \
    static btree_type_error_t get_last_##suffix##_error(void) { \
        return last_##suffix##_error; \
    } \
    \
    static const char* get_##suffix##_error_message(void) { \
        return last_##suffix##_error_msg; \
    }
```

### 6.2 디버깅 지원

```c
// 타입 정보 출력
#define BTREE_DEFINE_DEBUG_PRINT(type, suffix, format_spec) \
    static void btree_debug_print_##suffix(const void *ptr, FILE *output) { \
        if (!ptr || !output) return; \
        fprintf(output, format_spec, *(const type*)ptr); \
    } \
    \
    static void btree_debug_print_array_##suffix(const void *array, size_t count, FILE *output) { \
        const type *arr = (const type*)array; \
        fprintf(output, "["); \
        for (size_t i = 0; i < count; i++) { \
            if (i > 0) fprintf(output, ", "); \
            fprintf(output, format_spec, arr[i]); \
        } \
        fprintf(output, "]"); \
    }

// 트리 구조 시각화
#define BTREE_DEFINE_VISUALIZER(type, suffix) \
    static void btree_visualize_##suffix(btree_##suffix##_t *tree, FILE *output) { \
        fprintf(output, "B-Tree<%s> (degree=%d, height=%d)\n", \
                #type, tree->base.degree, btree_get_height(&tree->base)); \
        btree_visualize_recursive_##suffix(tree->base.root, 0, output); \
    } \
    \
    static void btree_visualize_recursive_##suffix(btree_node_t *node, int depth, FILE *output) { \
        if (!node) return; \
        \
        for (int i = 0; i < depth; i++) fprintf(output, "  "); \
        fprintf(output, "Node[%d keys]: ", node->num_keys); \
        \
        type *keys = (type*)node->keys; \
        for (int i = 0; i < node->num_keys; i++) { \
            if (i > 0) fprintf(output, ", "); \
            btree_debug_print_##suffix(&keys[i], output); \
        } \
        fprintf(output, "\n"); \
        \
        if (!node->is_leaf) { \
            for (int i = 0; i <= node->num_keys; i++) { \
                btree_visualize_recursive_##suffix(node->children[i], depth + 1, output); \
            } \
        } \
    }
```

## 7. 사용 예제

### 7.1 기본 타입 사용 예제

```c
// 정수형 B-Tree 정의 및 사용
BTREE_DEFINE_BASIC_OPS(int, int)
BTREE_DECLARE(int, int, int_int)
BTREE_DEFINE(int, int, int_int, btree_compare_int, btree_compare_int)
BTREE_DEFINE_DEBUG_PRINT(int, int_int, "%d")
BTREE_DEFINE_VISUALIZER(int, int_int)

int main() {
    // B-Tree 생성
    btree_int_int_t *tree = btree_int_int_create(5);
    
    // 데이터 삽입
    for (int i = 0; i < 100; i++) {
        btree_int_int_insert(tree, i, i * i);
    }
    
    // 검색
    int *result = btree_int_int_search(tree, 50);
    if (result) {
        printf("50^2 = %d\n", *result);
    }
    
    // 디버깅 - 트리 구조 출력
    btree_visualize_int_int(tree, stdout);
    
    // 정리
    btree_int_int_destroy(tree);
    return 0;
}
```

### 7.2 문자열 사용 예제

```c
// 문자열 비교 함수
static int string_compare(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// 문자열 B-Tree 정의
BTREE_DEFINE_STRING_TYPE(string)
BTREE_DECLARE(char*, char*, string_string)
BTREE_DEFINE(char*, char*, string_string, string_compare, string_compare)

int main() {
    btree_string_string_t *dict = btree_string_string_create(10);
    
    // 단어 사전 구축
    btree_string_string_insert(dict, "apple", "사과");
    btree_string_string_insert(dict, "banana", "바나나");
    btree_string_string_insert(dict, "cherry", "체리");
    
    // 검색
    char **translation = btree_string_string_search(dict, "apple");
    if (translation) {
        printf("apple = %s\n", *translation);
    }
    
    btree_string_string_destroy(dict);
    return 0;
}
```

### 7.3 사용자 정의 구조체 예제

```c
typedef struct {
    int id;
    char name[32];
    double score;
} student_t;

// 학생 ID로 비교
static int student_compare(const void *a, const void *b) {
    const student_t *sa = (const student_t*)a;
    const student_t *sb = (const student_t*)b;
    return (sa->id > sb->id) - (sa->id < sb->id);
}

// 학생 정보 출력
static void student_print(const void *ptr, FILE *output) {
    const student_t *s = (const student_t*)ptr;
    fprintf(output, "{id:%d, name:\"%s\", score:%.2f}", 
            s->id, s->name, s->score);
}

// 학생 B-Tree 정의
BTREE_DEFINE_BASIC_OPS(student_t, student)
BTREE_DECLARE(student_t, student_t, student_student)
BTREE_DEFINE(student_t, student_t, student_student, student_compare, student_compare)

int main() {
    btree_student_student_t *students = btree_student_student_create(5);
    
    // 학생 정보 추가
    student_t alice = {1001, "Alice", 95.5};
    student_t bob = {1002, "Bob", 87.3};
    student_t charlie = {1003, "Charlie", 92.1};
    
    btree_student_student_insert(students, alice, alice);
    btree_student_student_insert(students, bob, bob);
    btree_student_student_insert(students, charlie, charlie);
    
    // 학생 검색
    student_t search_key = {1002, "", 0.0};
    student_t *found = btree_student_student_search(students, search_key);
    if (found) {
        printf("Found student: ");
        student_print(found, stdout);
        printf("\n");
    }
    
    btree_student_student_destroy(students);
    return 0;
}
```

## 8. 성능 고려사항

### 8.1 인라인 함수 최적화

```c
// 자주 사용되는 함수들을 인라인으로 정의
#define BTREE_DEFINE_INLINE_OPS(type, suffix) \
    static inline bool btree_##suffix##_is_empty(btree_##suffix##_t *tree) { \
        return tree->base.root == NULL; \
    } \
    \
    static inline size_t btree_##suffix##_size(btree_##suffix##_t *tree) { \
        return tree->base.key_count; \
    } \
    \
    static inline int btree_##suffix##_height(btree_##suffix##_t *tree) { \
        return btree_get_height(&tree->base); \
    }
```

### 8.2 캐시 친화적 메모리 레이아웃

```c
// 캐시 라인에 맞는 최적 차수 계산
#define BTREE_OPTIMAL_DEGREE(key_type, value_type) \
    ((CACHE_LINE_SIZE - sizeof(btree_node_t)) / \
     (sizeof(key_type) + sizeof(value_type) + sizeof(void*)))

// 타입별 최적화된 생성 함수
#define BTREE_DEFINE_OPTIMIZED_CREATE(type, suffix) \
    static btree_##suffix##_t* btree_##suffix##_create_optimized(void) { \
        int optimal_degree = BTREE_OPTIMAL_DEGREE(type, type); \
        if (optimal_degree < 3) optimal_degree = 3; \
        return btree_##suffix##_create(optimal_degree); \
    }
```

### 8.3 컴파일 타임 상수 폴딩

```c
// 컴파일 타임에 크기가 결정되는 경우 최적화
#define BTREE_STATIC_SIZE_OPTIMIZATION(type, suffix) \
    _Static_assert(sizeof(type) > 0, "Invalid type size"); \
    \
    static const size_t btree_##suffix##_element_size = sizeof(type); \
    static const size_t btree_##suffix##_alignment = _Alignof(type); \
    \
    static inline void* btree_##suffix##_get_key_ptr(btree_node_t *node, int index) { \
        return (char*)node->keys + (btree_##suffix##_element_size * index); \
    } \
    \
    static inline void* btree_##suffix##_get_value_ptr(btree_node_t *node, int index) { \
        return (char*)node->values + (btree_##suffix##_element_size * index); \
    }
```

## 9. 확장성 및 플러그인

### 9.1 커스텀 할당자 지원

```c
#define BTREE_DEFINE_CUSTOM_ALLOCATOR(type, suffix, alloc_func, free_func) \
    static void* btree_##suffix##_custom_alloc(size_t size) { \
        return alloc_func(size); \
    } \
    \
    static void btree_##suffix##_custom_free(void *ptr) { \
        free_func(ptr); \
    } \
    \
    static btree_##suffix##_t* btree_##suffix##_create_with_allocator(int degree) { \
        btree_allocator_t custom_allocator = { \
            .alloc = btree_##suffix##_custom_alloc, \
            .free = btree_##suffix##_custom_free, \
            .realloc = NULL /* 기본 realloc 사용 */ \
        }; \
        \
        btree_##suffix##_t *tree = btree_##suffix##_create(degree); \
        if (tree) { \
            tree->base.allocator = &custom_allocator; \
        } \
        return tree; \
    }
```

### 9.2 이벤트 훅 시스템

```c
// 이벤트 콜백 타입 정의
typedef void (*btree_event_callback_t)(void *context, const void *key, const void *value);

#define BTREE_DEFINE_EVENTS(type, suffix) \
    typedef struct btree_##suffix##_events { \
        btree_event_callback_t on_insert; \
        btree_event_callback_t on_delete; \
        btree_event_callback_t on_search; \
        void *context; \
    } btree_##suffix##_events_t; \
    \
    static void btree_##suffix##_set_events(btree_##suffix##_t *tree, \
                                           btree_##suffix##_events_t *events) { \
        /* 이벤트 핸들러 설정 로직 */ \
    }
```

## 10. 컴파일 및 사용 가이드

### 10.1 헤더 파일 구조

```
btree/
├── include/
│   ├── btree_core.h        // 핵심 구조체 및 함수
│   ├── btree_generic.h     // 일반화 매크로
│   ├── btree_types.h       // 기본 타입 정의
│   └── btree.h             // 메인 헤더 (모든 것 포함)
├── src/
│   ├── btree_core.c        // 핵심 구현
│   ├── btree_memory.c      // 메모리 관리
│   └── btree_debug.c       // 디버깅 유틸리티
└── examples/
    ├── basic_usage.c       // 기본 사용법
    ├── custom_types.c      // 사용자 정의 타입
    └── performance.c       // 성능 벤치마크
```

### 10.2 사용법

1. **헤더 포함**:
```c
#include "btree.h"
```

2. **타입 정의**:
```c
// 정수형 B-Tree
BTREE_DECLARE(int, int, int_int);
BTREE_DEFINE_BASIC_OPS(int, int);
BTREE_DEFINE(int, int, int_int, btree_compare_int, btree_compare_int);
```

3. **사용**:
```c
btree_int_int_t *tree = btree_int_int_create(5);
btree_int_int_insert(tree, 42, 84);
int *result = btree_int_int_search(tree, 42);
btree_int_int_destroy(tree);
```

## 11. 결론

본 일반화 프로그래밍 명세서는 C언어의 제약사항 하에서 타입 안전하고 성능 효율적인 B-Tree 라이브러리를 구현하기 위한 포괄적인 설계를 제시합니다. 매크로 기반 코드 생성, 함수 포인터를 통한 동적 디스패치, 그리고 컴파일 타임 최적화를 조합하여 실용적이고 확장 가능한 솔루션을 제공합니다.

이 설계는 다음과 같은 장점을 제공합니다:

- **타입 안전성**: 컴파일 타임 타입 검사
- **성능**: 최소한의 런타임 오버헤드
- **재사용성**: 단일 코드베이스로 다양한 타입 지원
- **확장성**: 새로운 타입과 기능 쉽게 추가 가능
- **유지보수성**: 명확한 인터페이스와 모듈 분리

향후 이 설계를 바탕으로 실제 구현을 진행하여 C언어에서도 현대적인 일반화 프로그래밍의 이점을 활용할 수 있을 것입니다.