# B-Tree 아키텍처 설계 문서

## 1. 개요

### 1.1 목적
본 문서는 C언어 기반의 일반화된 B-Tree 자료구조 구현을 위한 아키텍처 설계를 다룹니다. 타입에 독립적이고 재사용 가능한 B-Tree 라이브러리를 설계하여 다양한 데이터 타입에서 활용할 수 있도록 합니다.

### 1.2 설계 목표
- **일반화(Generic Programming)**: 다양한 데이터 타입 지원
- **성능 최적화**: O(log n) 검색, 삽입, 삭제 연산 보장
- **메모리 효율성**: 동적 메모리 관리 최적화
- **모듈성**: 독립적이고 재사용 가능한 컴포넌트 설계
- **확장성**: 다양한 B-Tree 변형(B+Tree, B*Tree) 확장 가능

## 2. 시스템 아키텍처

### 2.1 계층 구조
```
┌─────────────────────────────┐
│      Application Layer      │
├─────────────────────────────┤
│        API Layer           │
├─────────────────────────────┤
│      Core B-Tree Layer     │
├─────────────────────────────┤
│    Memory Management       │
├─────────────────────────────┤
│    Platform Abstraction    │
└─────────────────────────────┘
```

### 2.2 주요 컴포넌트

#### 2.2.1 Core B-Tree Module
- **btree_core.h/c**: B-Tree 핵심 알고리즘 구현
- **btree_node.h/c**: 노드 구조 및 관리
- **btree_operations.h/c**: 기본 연산(검색, 삽입, 삭제)

#### 2.2.2 Generic Interface Module
- **btree_generic.h**: 일반화 매크로 정의
- **btree_types.h**: 타입 추상화 인터페이스
- **btree_compare.h**: 비교 함수 인터페이스

#### 2.2.3 Memory Management Module
- **btree_memory.h/c**: 메모리 풀 관리
- **btree_allocator.h/c**: 커스텀 할당자

#### 2.2.4 Utility Module
- **btree_debug.h/c**: 디버깅 및 시각화
- **btree_iterator.h/c**: 반복자 패턴
- **btree_serialize.h/c**: 직렬화/역직렬화

## 3. 데이터 구조 설계

### 3.1 B-Tree 노드 구조
```c
typedef struct btree_node {
    int is_leaf;                    // 리프 노드 여부
    int num_keys;                   // 현재 키 개수
    void **keys;                    // 키 배열
    void **values;                  // 값 배열 (리프 노드용)
    struct btree_node **children;  // 자식 노드 배열
    struct btree_node *parent;      // 부모 노드
    struct btree_node *next;        // 다음 리프 노드 (B+Tree용)
} btree_node_t;
```

### 3.2 B-Tree 구조체
```c
typedef struct btree {
    btree_node_t *root;             // 루트 노드
    int degree;                     // B-Tree 차수
    int max_keys;                   // 최대 키 개수
    int min_keys;                   // 최소 키 개수
    size_t key_size;                // 키 크기
    size_t value_size;              // 값 크기
    compare_func_t compare;         // 비교 함수
    allocator_t *allocator;         // 메모리 할당자
    size_t node_count;              // 전체 노드 수
    size_t key_count;               // 전체 키 수
} btree_t;
```

### 3.3 일반화 인터페이스
```c
typedef int (*compare_func_t)(const void *a, const void *b);
typedef void (*copy_func_t)(void *dest, const void *src);
typedef void (*destroy_func_t)(void *data);

typedef struct btree_config {
    int degree;                     // B-Tree 차수
    size_t key_size;                // 키 크기
    size_t value_size;              // 값 크기
    compare_func_t key_compare;     // 키 비교 함수
    copy_func_t key_copy;           // 키 복사 함수
    copy_func_t value_copy;         // 값 복사 함수
    destroy_func_t key_destroy;     // 키 소멸 함수
    destroy_func_t value_destroy;   // 값 소멸 함수
} btree_config_t;
```

## 4. 핵심 알고리즘 설계

### 4.1 검색 (Search)
```
Algorithm: btree_search(tree, key)
1. current = tree->root
2. while current is not NULL:
   a. Find position i where key <= current->keys[i]
   b. if key == current->keys[i]:
      return current->values[i]
   c. if current->is_leaf:
      return NULL
   d. current = current->children[i]
3. return NULL
```

### 4.2 삽입 (Insert)
```
Algorithm: btree_insert(tree, key, value)
1. if tree->root is NULL:
   Create root node and insert key-value pair
2. leaf = find_leaf_for_insertion(tree, key)
3. if leaf has space:
   Insert key-value pair in sorted order
4. else:
   Split leaf node and propagate split upward
   Handle root split if necessary
```

### 4.3 삭제 (Delete)
```
Algorithm: btree_delete(tree, key)
1. node = find_node_containing_key(tree, key)
2. if node is NULL:
   return FALSE
3. if node is leaf:
   Remove key-value pair
   Handle underflow if necessary
4. else (internal node):
   Replace with predecessor or successor
   Delete replacement key from leaf
   Handle underflow cascade
```

## 5. 메모리 관리 전략

### 5.1 메모리 풀 설계
```c
typedef struct memory_pool {
    void *pool_start;               // 풀 시작 주소
    size_t pool_size;               // 풀 전체 크기
    size_t block_size;              // 블록 크기
    size_t free_blocks;             // 여유 블록 수
    void *free_list;                // 여유 블록 리스트
} memory_pool_t;
```

### 5.2 할당 전략
- **노드 풀**: 고정 크기 노드용 메모리 풀
- **키-값 풀**: 가변 크기 데이터용 별도 풀
- **캐시 지역성**: 관련 노드들을 인접 메모리에 할당

## 6. 성능 최적화 전략

### 6.1 캐시 최적화
- **메모리 레이아웃**: 캐시 라인에 맞는 노드 크기 조정
- **프리패칭**: 순차 접근 시 미리 로드
- **지역성**: 관련 데이터 인접 배치

### 6.2 알고리즘 최적화
- **이진 검색**: 노드 내 키 검색 최적화
- **배치 연산**: 여러 연산을 함께 처리
- **지연 분할**: 필요시에만 노드 분할

## 7. 확장성 고려사항

### 7.1 B-Tree 변형 지원
- **B+Tree**: 리프 노드 연결 리스트
- **B*Tree**: 더 높은 공간 활용도
- **Concurrent B-Tree**: 멀티스레드 지원

### 7.2 플러그인 아키텍처
```c
typedef struct btree_plugin {
    const char *name;
    int (*init)(btree_t *tree);
    int (*insert_hook)(btree_t *tree, void *key, void *value);
    int (*delete_hook)(btree_t *tree, void *key);
    void (*cleanup)(btree_t *tree);
} btree_plugin_t;
```

## 8. 오류 처리 및 디버깅

### 8.1 오류 코드 정의
```c
typedef enum {
    BTREE_SUCCESS = 0,
    BTREE_ERROR_NULL_POINTER,
    BTREE_ERROR_INVALID_DEGREE,
    BTREE_ERROR_MEMORY_ALLOCATION,
    BTREE_ERROR_KEY_NOT_FOUND,
    BTREE_ERROR_DUPLICATE_KEY,
    BTREE_ERROR_INVALID_OPERATION
} btree_result_t;
```

### 8.2 디버깅 지원
- **구조 시각화**: 트리 구조 출력
- **통계 정보**: 노드 수, 높이, 공간 활용도
- **검증 함수**: B-Tree 불변조건 검사

## 9. 테스트 전략

### 9.1 단위 테스트
- 각 모듈별 개별 테스트
- 경계 조건 테스트
- 오류 상황 시뮬레이션

### 9.2 통합 테스트
- 전체 시나리오 테스트
- 성능 벤치마크
- 메모리 누수 검사

### 9.3 스트레스 테스트
- 대용량 데이터 처리
- 장시간 운영 테스트
- 메모리 제한 환경 테스트

## 10. 구현 로드맵

### Phase 1: 기본 구조 (2주)
- 핵심 데이터 구조 정의
- 기본 메모리 관리
- 단순 타입 지원

### Phase 2: 핵심 알고리즘 (3주)
- 검색, 삽입, 삭제 구현
- 노드 분할/합병
- 기본 테스트 작성

### Phase 3: 일반화 (2주)
- 제네릭 인터페이스 구현
- 타입 추상화
- 다양한 데이터 타입 지원

### Phase 4: 최적화 (2주)
- 성능 최적화
- 메모리 효율성 개선
- 벤치마크 테스트

### Phase 5: 확장 기능 (1주)
- 반복자 구현
- 직렬화 지원
- 디버깅 도구

## 11. 결론

본 아키텍처는 C언어의 제약사항 하에서 최대한 일반화된 B-Tree 구현을 제공하는 것을 목표로 합니다. 모듈러 설계를 통해 확장성을 보장하고, 성능 최적화를 통해 실용적인 사용이 가능하도록 설계되었습니다.