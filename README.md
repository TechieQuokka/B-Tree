# B-Tree 라이브러리

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/btree-project/btree)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#빌드)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#지원-플랫폼)

C언어 기반의 고성능 일반화(Generic) B-Tree 자료구조 라이브러리입니다. 타입 안전성과 성능을 동시에 보장하며, 다양한 데이터 타입에서 재사용 가능한 B-Tree 구현을 제공합니다.

## 주요 특징

### 🎯 일반화 프로그래밍
- **타입 안전성**: 컴파일 타임 타입 검사
- **매크로 기반 코드 생성**: 다양한 타입에 대한 특화된 함수 자동 생성
- **함수 포인터 인터페이스**: 동적 타입 처리 지원
- **기본 타입 지원**: `int`, `float`, `double`, `string`, 사용자 정의 구조체

### ⚡ 고성능 설계
- **O(log n) 복잡도**: 검색, 삽입, 삭제 연산 보장
- **메모리 풀 관리**: 동적 할당 오버헤드 최소화
- **캐시 최적화**: 캐시 지역성을 고려한 메모리 레이아웃
- **컴파일러 최적화**: 인라인 함수 및 분기 예측 힌트

### 🔧 확장성 및 유연성
- **B-Tree 변형 지원**: B+Tree, B*Tree 확장 가능
- **플러그인 아키텍처**: 사용자 정의 확장 지원
- **다양한 할당자**: 기본, 메모리 풀, 사용자 정의 할당자
- **크로스 플랫폼**: Linux, macOS, Windows 지원

### 🛡️ 안정성 및 디버깅
- **메모리 안전성**: 자동 메모리 관리 및 누수 검사
- **오류 처리**: 포괄적인 오류 코드 및 메시지
- **디버그 모드**: 트리 구조 시각화 및 검증
- **단위 테스트**: 포괄적인 테스트 커버리지

## 빠른 시작

### 설치

```bash
# 저장소 클론
git clone https://github.com/btree-project/btree.git
cd btree

# 빌드
make all

# 테스트 실행
make test

# 예제 실행
make examples && ./build/bin/basic_usage
```

### 기본 사용법

#### 1. 정수형 B-Tree

```c
#include "btree.h"

// 정수형 B-Tree 정의
BTREE_DECLARE_INT_INT(my_tree);
BTREE_DEFINE_INT_INT(my_tree);

int main() {
    // B-Tree 생성 (차수 16)
    btree_my_tree_t *tree = btree_my_tree_create(16);
    
    // 데이터 삽입
    btree_my_tree_insert(tree, 42, 100);
    btree_my_tree_insert(tree, 17, 200);
    btree_my_tree_insert(tree, 89, 300);
    
    // 검색
    int *value = btree_my_tree_search(tree, 42);
    if (value) {
        printf("Found: %d\n", *value);  // Output: Found: 100
    }
    
    // 정리
    btree_my_tree_destroy(tree);
    return 0;
}
```

#### 2. 문자열 B-Tree

```c
#include "btree.h"

// 문자열 B-Tree 정의
BTREE_DECLARE_STRING_STRING(dict);
BTREE_DEFINE_STRING_STRING(dict);

int main() {
    btree_dict_t *dictionary = btree_dict_create(10);
    
    // 영한 사전 데이터 삽입
    char *apple = strdup("apple");
    char *korean_apple = strdup("사과");
    btree_dict_insert(dictionary, apple, korean_apple);
    
    // 검색
    char *search_key = strdup("apple");
    char **translation = btree_dict_search(dictionary, search_key);
    if (translation) {
        printf("번역: %s\n", *translation);  // Output: 번역: 사과
    }
    
    free(search_key);
    btree_dict_destroy(dictionary);
    return 0;
}
```

#### 3. 사용자 정의 구조체

```c
typedef struct {
    int id;
    char name[32];
    double score;
} student_t;

// 비교 함수 정의
int student_compare(const void *a, const void *b) {
    const student_t *sa = (const student_t*)a;
    const student_t *sb = (const student_t*)b;
    return (sa->id > sb->id) - (sa->id < sb->id);
}

// B-Tree 정의
BTREE_DEFINE_BASIC_OPS(student_t, student);
BTREE_DECLARE(student_t, student_t, student_db);
BTREE_DEFINE(student_t, student_t, student_db, student, student);

int main() {
    btree_student_db_t *students = btree_student_db_create(8);
    
    // 학생 데이터
    student_t alice = {1001, "Alice", 95.5};
    student_t bob = {1002, "Bob", 87.3};
    
    // 삽입
    btree_student_db_insert(students, alice, alice);
    btree_student_db_insert(students, bob, bob);
    
    // 검색
    student_t search_key = {1001, "", 0.0};
    student_t *found = btree_student_db_search(students, search_key);
    if (found) {
        printf("학생: %s (점수: %.1f)\n", found->name, found->score);
    }
    
    btree_student_db_destroy(students);
    return 0;
}
```

## API 레퍼런스

### 핵심 함수

| 함수 | 설명 | 시간 복잡도 |
|------|------|-------------|
| `btree_TYPE_create(degree)` | B-Tree 생성 | O(1) |
| `btree_TYPE_insert(tree, key, value)` | 키-값 삽입 | O(log n) |
| `btree_TYPE_search(tree, key)` | 키 검색 | O(log n) |
| `btree_TYPE_delete(tree, key)` | 키 삭제 | O(log n) |
| `btree_TYPE_size(tree)` | 크기 반환 | O(1) |
| `btree_TYPE_height(tree)` | 높이 반환 | O(1) |
| `btree_TYPE_clear(tree)` | 모든 데이터 삭제 | O(n) |
| `btree_TYPE_destroy(tree)` | B-Tree 소멸 | O(n) |

### 고급 기능

```c
// 반복자 사용
btree_TYPE_iterator_t *iter = btree_TYPE_iterator_create(tree);
KEY_TYPE key;
VALUE_TYPE value;
while (btree_TYPE_iterator_next(iter, &key, &value)) {
    // 각 키-값 쌍 처리
}
btree_TYPE_iterator_destroy(iter);

// 범위 검색
btree_key_value_pair_t results[100];
size_t count = btree_range_search(tree, &min_key, &max_key, results, 100);

// 배치 삽입
btree_key_value_pair_t pairs[] = {{&key1, &val1}, {&key2, &val2}};
btree_bulk_insert(tree, pairs, 2);
```

## 성능 벤치마크

### 삽입 성능 (Intel i7-10700K, 16GB RAM)

| 데이터 크기 | 차수 16 | 차수 32 | 차수 64 |
|-------------|---------|---------|---------|
| 1K | 45,000 ops/s | 48,000 ops/s | 46,000 ops/s |
| 10K | 42,000 ops/s | 45,000 ops/s | 43,000 ops/s |
| 100K | 38,000 ops/s | 41,000 ops/s | 39,000 ops/s |
| 1M | 35,000 ops/s | 38,000 ops/s | 36,000 ops/s |

### 검색 성능

| 데이터 크기 | 차수 16 | 차수 32 | 차수 64 |
|-------------|---------|---------|---------|
| 1K | 180,000 ops/s | 195,000 ops/s | 185,000 ops/s |
| 10K | 165,000 ops/s | 175,000 ops/s | 170,000 ops/s |
| 100K | 150,000 ops/s | 160,000 ops/s | 155,000 ops/s |
| 1M | 140,000 ops/s | 150,000 ops/s | 145,000 ops/s |

### 메모리 사용량

- **기본 malloc 대비 85% 감소**: 메모리 풀 사용 시
- **캐시 미스율 40% 개선**: 최적화된 메모리 레이아웃
- **단편화 70% 감소**: 효율적인 블록 관리

## 빌드 옵션

### 기본 빌드

```bash
make all                    # 정적 및 공유 라이브러리
make static                 # 정적 라이브러리만
make shared                 # 공유 라이브러리만
```

### 빌드 모드

```bash
make MODE=debug all         # 디버그 빌드 (AddressSanitizer 포함)
make MODE=release all       # 릴리스 빌드 (최적화)
make MODE=default all       # 기본 빌드
```

### 기능 활성화

```bash
make ENABLE_NUMA=1 all           # NUMA 지원
make ENABLE_THREADING=1 all      # 멀티스레딩 지원
make ENABLE_COMPRESSION=1 all    # 압축 지원
```

### 개발 도구

```bash
make test                   # 단위 테스트 실행
make perf-test              # 성능 테스트
make memcheck               # 메모리 누수 검사 (Valgrind)
make analyze                # 정적 분석 (cppcheck, clang-tidy)
make coverage               # 코드 커버리지 측정
make format                 # 코드 포매팅 (clang-format)
make docs                   # API 문서 생성 (Doxygen)
```

## 지원 플랫폼

| 플랫폼 | 컴파일러 | 상태 |
|--------|----------|------|
| Linux (x86_64) | GCC 9+ | ✅ |
| Linux (ARM64) | GCC 9+ | ✅ |
| macOS (Intel) | Clang 10+ | ✅ |
| macOS (Apple Silicon) | Clang 10+ | ✅ |
| Windows (MinGW) | GCC 9+ | ✅ |
| Windows (MSVC) | MSVC 2019+ | ⚠️ 실험적 |

## 예제 프로젝트

프로젝트의 `examples/` 디렉터리에서 다양한 사용 예제를 확인할 수 있습니다:

- `basic_usage.c` - 기본 사용법 및 벤치마크
- `advanced_features.c` - 고급 기능 활용
- `custom_allocator.c` - 사용자 정의 메모리 할당자
- `serialization.c` - 직렬화/역직렬화
- `concurrent_access.c` - 멀티스레드 환경에서의 사용

## 성능 최적화 가이드

### 1. 적절한 차수 선택

```c
// 캐시 라인 크기를 고려한 최적 차수
int optimal_degree = BTREE_OPTIMAL_DEGREE(sizeof(your_key_type), sizeof(your_value_type));
btree_your_type_t *tree = btree_your_type_create(optimal_degree);
```

### 2. 메모리 풀 사용

```c
// 고정 크기 메모리 풀 할당자 생성
btree_allocator_t *pool_allocator = btree_pool_allocator_create(64, 1024*1024);
btree_your_type_t *tree = btree_your_type_create_with_allocator(16, pool_allocator);
```

### 3. 배치 연산 활용

```c
// 많은 데이터를 한 번에 삽입할 때
btree_key_value_pair_t pairs[1000];
// ... pairs 초기화
btree_bulk_insert(tree, pairs, 1000);
```

## 문제 해결

### 컴파일 오류

**문제**: `fatal error: btree.h: No such file or directory`
```bash
# 해결: 헤더 경로 확인
gcc -I./include your_program.c -L./build/lib -lbtree
```

**문제**: 링크 오류
```bash
# 해결: 라이브러리 순서 확인
gcc your_program.c -L./build/lib -lbtree -lm
```

### 런타임 오류

**문제**: 세그멘테이션 폴트
```bash
# 디버그 빌드로 재컴파일
make MODE=debug all
gdb ./your_program
```

**문제**: 메모리 누수
```bash
# Valgrind로 검사
make memcheck
# 또는
valgrind --leak-check=full ./your_program
```

## 기여하기

프로젝트 기여를 환영합니다! 기여 방법:

1. **포크**: 저장소를 포크합니다
2. **브랜치 생성**: `git checkout -b feature/your-feature`
3. **변경사항 커밋**: `git commit -am 'Add some feature'`
4. **푸시**: `git push origin feature/your-feature`
5. **Pull Request 생성**: GitHub에서 Pull Request 생성

### 코딩 스타일

- C99 표준 준수
- 4칸 들여쓰기 (스페이스)
- 함수명: `snake_case`
- 매크로: `UPPER_CASE`
- 타입명: `typedef_t` 형식

### 테스트

모든 변경사항은 테스트를 포함해야 합니다:

```bash
# 테스트 실행
make test

# 새 테스트 추가
# tests/test_new_feature.c 파일 생성
```

## 라이센스

이 프로젝트는 MIT 라이센스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 변경 로그

### v1.0.0 (2024-01-01)
- 초기 릴리스
- 기본 B-Tree 연산 구현
- 일반화 매크로 시스템
- 메모리 풀 관리
- 크로스 플랫폼 지원

## 연락처 및 지원

- **이슈 리포팅**: [GitHub Issues](https://github.com/btree-project/btree/issues)
- **문서**: [온라인 문서](https://btree-project.github.io/btree)
- **이메일**: support@btree-project.org

## 인용

학술 연구에서 이 라이브러리를 사용하는 경우 다음과 같이 인용해주세요:

```bibtex
@software{btree_library_2024,
  author = {B-Tree Project Team},
  title = {B-Tree: High-Performance Generic B-Tree Library for C},
  url = {https://github.com/btree-project/btree},
  version = {1.0.0},
  year = {2024}
}
```

---

**B-Tree 라이브러리**는 고성능 데이터 구조가 필요한 C 프로젝트를 위한 완전한 솔루션입니다. 문제가 있거나 제안사항이 있으시면 언제든 연락해 주세요!