# B-Tree Library Release Package

## 📦 포함된 실행파일들

### 1. test_btree.exe
- **설명**: 전체 테스트 스위트 실행
- **사용법**: `./test_btree.exe [--perf]`
- **기능**: 11개 단위 테스트 + 성능 테스트 실행

### 2. simple_demo.exe
- **설명**: B-Tree 기본 사용법 데모
- **사용법**: `./simple_demo.exe`
- **기능**: 기본 연산, 검색, 성능 테스트 데모

### 3. simple_example.exe
- **설명**: 간단한 B-Tree 예제
- **사용법**: `./simple_example.exe`
- **기능**: 기본적인 B-Tree 사용 예시

### 4. debug_test.exe
- **설명**: 디버깅용 테스트 프로그램
- **사용법**: `./debug_test.exe`
- **기능**: B-Tree 내부 동작 확인

### 5. debug_multiple.exe
- **설명**: 다중 B-Tree 인스턴스 테스트
- **사용법**: `./debug_multiple.exe`
- **기능**: 여러 B-Tree 동시 사용 테스트

### 6. btree_sort_test.exe
- **설명**: B-Tree 정렬 성능 테스트
- **사용법**: `./btree_sort_test.exe`
- **기능**: 다양한 크기와 차수로 정렬 성능 측정

### 7. btree_traversal_sort.exe
- **설명**: B-Tree 중위 순회 정렬 데모
- **사용법**: `./btree_traversal_sort.exe`
- **기능**: In-order traversal을 통한 정렬 시연

### 8. large_data_verification.exe ⭐ **NEW**
- **설명**: 대용량 데이터 정렬 검증 프로그램
- **사용법**: `./large_data_verification.exe`
- **기능**: 100개 요소로 B-Tree 정렬 상세 검증

## 📊 라이브러리 파일

### libbtree.a
- **설명**: B-Tree 정적 라이브러리
- **용도**: 다른 프로그램에서 B-Tree 기능 사용

## ✅ 테스트 결과

- **총 테스트**: 11개
- **통과율**: 100%
- **메모리 누수**: 없음 ✅
- **성능**: 최대 16.7M ops/s (50K 요소, 차수 32)

## 🚀 성능 특성

| 차수 | 50K 요소 삽입 | 50K 요소 검색 | 트리 높이 |
|------|---------------|---------------|-----------|
| 5    | 4.5M ops/s    | 10M ops/s     | 7         |
| 10   | 8.3M ops/s    | 12.5M ops/s   | 5         |
| 16   | 10M ops/s     | 10M ops/s     | 4         |
| 32   | 10M ops/s     | 16.7M ops/s   | 3         |

## 💡 권장사항

1. **최적 차수**: 16-32 (성능과 메모리 사용량 균형)
2. **메모리 관리**: 모든 B-Tree는 `destroy()` 함수로 해제 필요
3. **라이브러리 정리**: 프로그램 종료 전 `btree_library_cleanup()` 호출

## 🔧 컴파일 환경

- **컴파일러**: GCC (MinGW64)
- **표준**: C99
- **최적화**: -O2
- **플래그**: -Wall -Wextra -Wpedantic