/**
 * @file test_btree.c
 * @brief B-Tree 라이브러리 단위 테스트
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../include/btree.h"

/* 테스트 매크로 */
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s (line %d): %s\n", __func__, __LINE__, message); \
            test_failures++; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_NE(not_expected, actual, message) \
    TEST_ASSERT((not_expected) != (actual), message)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

/* 전역 테스트 통계 */
static int test_count = 0;
static int test_failures = 0;
static int test_passes = 0;

/* 정수형 B-Tree 정의 */
BTREE_DECLARE_INT_INT(test_int);
BTREE_DEFINE_INT_INT(test_int);

/**
 * @brief 테스트 시작 매크로
 */
#define RUN_TEST(test_func) \
    do { \
        printf("실행 중: %s... ", #test_func); \
        fflush(stdout); \
        test_count++; \
        if (test_func()) { \
            printf("통과\n"); \
            test_passes++; \
        } else { \
            printf("실패\n"); \
        } \
    } while(0)

/**
 * @brief 기본 생성/소멸 테스트
 */
bool test_btree_creation_destruction() {
    btree_test_int_t *tree = btree_test_int_create(5);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    TEST_ASSERT_EQ(0, btree_test_int_size(tree), "초기 크기가 0이 아님");
    TEST_ASSERT_EQ(true, btree_test_int_is_empty(tree), "초기 상태가 비어있지 않음");
    TEST_ASSERT_EQ(0, btree_test_int_height(tree), "초기 높이가 0이 아님");
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 단일 삽입/검색 테스트
 */
bool test_single_insert_search() {
    btree_test_int_t *tree = btree_test_int_create(3);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    /* 단일 키-값 삽입 */
    btree_result_t result = btree_test_int_insert(tree, 42, 84);
    TEST_ASSERT_EQ(BTREE_SUCCESS, result, "삽입 실패");
    
    /* 크기 확인 */
    TEST_ASSERT_EQ(1, btree_test_int_size(tree), "삽입 후 크기가 1이 아님");
    TEST_ASSERT_EQ(false, btree_test_int_is_empty(tree), "삽입 후에도 비어있음");
    TEST_ASSERT_EQ(1, btree_test_int_height(tree), "삽입 후 높이가 1이 아님");
    
    /* 검색 테스트 */
    int *value = btree_test_int_search(tree, 42);
    TEST_ASSERT_NOT_NULL(value, "삽입한 키를 찾을 수 없음");
    TEST_ASSERT_EQ(84, *value, "검색된 값이 올바르지 않음");
    
    /* 존재하지 않는 키 검색 */
    int *null_value = btree_test_int_search(tree, 100);
    TEST_ASSERT_NULL(null_value, "존재하지 않는 키가 발견됨");
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 다중 삽입 테스트
 */
bool test_multiple_insert() {
    btree_test_int_t *tree = btree_test_int_create(5);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    /* 순차적 삽입 */
    for (int i = 1; i <= 10; i++) {
        btree_result_t result = btree_test_int_insert(tree, i, i * 10);
        TEST_ASSERT_EQ(BTREE_SUCCESS, result, "순차 삽입 실패");
    }
    
    TEST_ASSERT_EQ(10, btree_test_int_size(tree), "삽입 후 크기가 올바르지 않음");
    
    /* 모든 키 검색 확인 */
    for (int i = 1; i <= 10; i++) {
        int *value = btree_test_int_search(tree, i);
        TEST_ASSERT_NOT_NULL(value, "삽입한 키를 찾을 수 없음");
        TEST_ASSERT_EQ(i * 10, *value, "검색된 값이 올바르지 않음");
    }
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 역순 삽입 테스트
 */
bool test_reverse_insert() {
    btree_test_int_t *tree = btree_test_int_create(5);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    /* 역순 삽입 */
    for (int i = 10; i >= 1; i--) {
        btree_result_t result = btree_test_int_insert(tree, i, i * 10);
        TEST_ASSERT_EQ(BTREE_SUCCESS, result, "역순 삽입 실패");
    }
    
    TEST_ASSERT_EQ(10, btree_test_int_size(tree), "삽입 후 크기가 올바르지 않음");
    
    /* 모든 키 검색 확인 */
    for (int i = 1; i <= 10; i++) {
        int *value = btree_test_int_search(tree, i);
        TEST_ASSERT_NOT_NULL(value, "삽입한 키를 찾을 수 없음");
        TEST_ASSERT_EQ(i * 10, *value, "검색된 값이 올바르지 않음");
    }
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 무작위 삽입 테스트
 */
bool test_random_insert() {
    btree_test_int_t *tree = btree_test_int_create(7);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    const int test_size = 100;
    int keys[test_size];
    
    /* 무작위 키 배열 생성 */
    srand((unsigned int)time(NULL));
    for (int i = 0; i < test_size; i++) {
        keys[i] = rand() % 1000;
    }
    
    /* 무작위 삽입 */
    for (int i = 0; i < test_size; i++) {
        btree_test_int_insert(tree, keys[i], keys[i] * 2);
    }
    
    /* 삽입된 키들 검색 확인 */
    int found_count = 0;
    for (int i = 0; i < test_size; i++) {
        int *value = btree_test_int_search(tree, keys[i]);
        if (value && *value == keys[i] * 2) {
            found_count++;
        }
    }
    
    TEST_ASSERT(found_count > 0, "무작위 삽입된 키들을 찾을 수 없음");
    printf(" (삽입: %d, 발견: %d) ", test_size, found_count);
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 중복 키 테스트
 */
bool test_duplicate_keys() {
    btree_test_int_t *tree = btree_test_int_create(5);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    /* 첫 번째 삽입 */
    btree_result_t result1 = btree_test_int_insert(tree, 42, 100);
    TEST_ASSERT_EQ(BTREE_SUCCESS, result1, "첫 번째 삽입 실패");
    
    /* 중복 키 삽입 (기본적으로 중복 허용하지 않음) */
    btree_result_t result2 = btree_test_int_insert(tree, 42, 200);
    TEST_ASSERT_EQ(BTREE_ERROR_DUPLICATE_KEY, result2, "중복 키 삽입이 허용됨");
    
    /* 원래 값이 유지되는지 확인 */
    int *value = btree_test_int_search(tree, 42);
    TEST_ASSERT_NOT_NULL(value, "키를 찾을 수 없음");
    TEST_ASSERT_EQ(100, *value, "중복 삽입으로 값이 변경됨");
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 대용량 데이터 테스트
 */
bool test_large_dataset() {
    btree_test_int_t *tree = btree_test_int_create(16);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    const int large_size = 5000;
    
    /* 대용량 순차 삽입 */
    for (int i = 0; i < large_size; i++) {
        btree_result_t result = btree_test_int_insert(tree, i, i * 3);
        TEST_ASSERT_EQ(BTREE_SUCCESS, result, "대용량 삽입 실패");
        
        /* 중간 중간 진행 상황 출력 */
        if (i % 1000 == 999) {
            printf(".");
            fflush(stdout);
        }
    }
    
    TEST_ASSERT_EQ(large_size, btree_test_int_size(tree), "대용량 삽입 후 크기 불일치");
    
    /* 샘플 검색 확인 */
    int sample_indices[] = {0, 100, 1000, 2500, 4999};
    for (size_t i = 0; i < sizeof(sample_indices)/sizeof(sample_indices[0]); i++) {
        int key = sample_indices[i];
        int *value = btree_test_int_search(tree, key);
        TEST_ASSERT_NOT_NULL(value, "대용량 데이터에서 키를 찾을 수 없음");
        TEST_ASSERT_EQ(key * 3, *value, "대용량 데이터의 값이 올바르지 않음");
    }
    
    printf(" (크기: %zu, 높이: %d) ", btree_test_int_size(tree), btree_test_int_height(tree));
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 트리 클리어 테스트
 */
bool test_tree_clear() {
    btree_test_int_t *tree = btree_test_int_create(5);
    TEST_ASSERT_NOT_NULL(tree, "B-Tree 생성 실패");
    
    /* 데이터 삽입 */
    for (int i = 1; i <= 20; i++) {
        btree_test_int_insert(tree, i, i * 5);
    }
    
    TEST_ASSERT_EQ(20, btree_test_int_size(tree), "삽입 후 크기가 올바르지 않음");
    TEST_ASSERT_EQ(false, btree_test_int_is_empty(tree), "데이터 삽입 후에도 비어있음");
    
    /* 트리 클리어 */
    btree_test_int_clear(tree);
    
    TEST_ASSERT_EQ(0, btree_test_int_size(tree), "클리어 후 크기가 0이 아님");
    TEST_ASSERT_EQ(true, btree_test_int_is_empty(tree), "클리어 후에도 비어있지 않음");
    TEST_ASSERT_EQ(0, btree_test_int_height(tree), "클리어 후 높이가 0이 아님");
    
    /* 클리어 후 검색 */
    int *value = btree_test_int_search(tree, 10);
    TEST_ASSERT_NULL(value, "클리어 후에도 키가 발견됨");
    
    btree_test_int_destroy(tree);
    return true;
}

/**
 * @brief 메모리 풀 테스트
 */
bool test_memory_pool() {
    btree_memory_pool_t *pool = btree_pool_create(64, 64 * 1024, BTREE_POOL_FLAG_ZERO_MEMORY);
    TEST_ASSERT_NOT_NULL(pool, "메모리 풀 생성 실패");
    
    /* 메모리 할당 테스트 */
    void *ptr1 = btree_pool_alloc(pool);
    TEST_ASSERT_NOT_NULL(ptr1, "풀에서 메모리 할당 실패");
    
    void *ptr2 = btree_pool_alloc(pool);
    TEST_ASSERT_NOT_NULL(ptr2, "풀에서 두 번째 메모리 할당 실패");
    TEST_ASSERT_NE(ptr1, ptr2, "동일한 메모리 주소가 할당됨");
    
    /* 메모리 소속 확인 */
    TEST_ASSERT(btree_pool_contains(pool, ptr1), "할당된 메모리가 풀에 속하지 않음");
    TEST_ASSERT(btree_pool_contains(pool, ptr2), "할당된 메모리가 풀에 속하지 않음");
    
    /* 통계 확인 */
    btree_pool_stats_t stats;
    btree_pool_get_stats(pool, &stats);
    TEST_ASSERT(stats.used_blocks >= 2, "사용된 블록 수가 올바르지 않음");
    
    /* 메모리 해제 */
    btree_pool_free(pool, ptr1);
    btree_pool_free(pool, ptr2);
    
    btree_pool_destroy(pool);
    return true;
}

/**
 * @brief 오류 처리 테스트
 */
bool test_error_handling() {
    /* NULL 포인터 테스트 */
    btree_result_t result = btree_insert(NULL, NULL, NULL);
    TEST_ASSERT_EQ(BTREE_ERROR_NULL_POINTER, result, "NULL 포인터 오류가 감지되지 않음");
    
    void *search_result = btree_search(NULL, NULL);
    TEST_ASSERT_NULL(search_result, "NULL 트리에서 검색이 성공함");
    
    /* 잘못된 차수 테스트 */
    btree_test_int_t *invalid_tree = btree_test_int_create(1);  /* 최소 차수보다 작음 */
    TEST_ASSERT_NULL(invalid_tree, "잘못된 차수로 트리가 생성됨");
    
    /* 오류 문자열 테스트 */
    const char *error_msg = btree_error_string(BTREE_ERROR_MEMORY_ALLOCATION);
    TEST_ASSERT_NOT_NULL(error_msg, "오류 문자열이 NULL임");
    TEST_ASSERT(strlen(error_msg) > 0, "오류 문자열이 비어있음");
    
    return true;
}

/**
 * @brief 라이브러리 정보 테스트
 */
bool test_library_info() {
    /* 버전 정보 */
    const char *version = btree_version_string();
    TEST_ASSERT_NOT_NULL(version, "버전 문자열이 NULL임");
    TEST_ASSERT(strlen(version) > 0, "버전 문자열이 비어있음");
    
    int major = btree_version_major();
    int minor = btree_version_minor();
    int patch = btree_version_patch();
    
    TEST_ASSERT(major >= 0, "주 버전이 음수임");
    TEST_ASSERT(minor >= 0, "부 버전이 음수임");
    TEST_ASSERT(patch >= 0, "패치 버전이 음수임");
    
    printf(" (버전: %d.%d.%d) ", major, minor, patch);
    
    return true;
}

/**
 * @brief 모든 테스트 실행
 */
void run_all_tests() {
    printf("B-Tree 라이브러리 단위 테스트 시작\n");
    printf("====================================\n\n");
    
    /* 라이브러리 초기화 */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "라이브러리 초기화 실패\n");
        return;
    }
    
    /* 기본 기능 테스트 */
    RUN_TEST(test_btree_creation_destruction);
    RUN_TEST(test_single_insert_search);
    RUN_TEST(test_multiple_insert);
    RUN_TEST(test_reverse_insert);
    RUN_TEST(test_random_insert);
    RUN_TEST(test_duplicate_keys);
    RUN_TEST(test_tree_clear);
    
    /* 고급 기능 테스트 */
    RUN_TEST(test_large_dataset);
    RUN_TEST(test_memory_pool);
    
    /* 오류 처리 테스트 */
    RUN_TEST(test_error_handling);
    RUN_TEST(test_library_info);
    
    /* 라이브러리 정리 */
    btree_library_cleanup();
    
    /* 결과 출력 */
    printf("\n====================================\n");
    printf("테스트 결과:\n");
    printf("  총 테스트: %d\n", test_count);
    printf("  통과:     %d\n", test_passes);
    printf("  실패:     %d\n", test_failures);
    printf("  성공률:   %.1f%%\n", test_count > 0 ? (100.0 * test_passes / test_count) : 0.0);
    
    if (test_failures == 0) {
        printf("\n🎉 모든 테스트가 통과했습니다!\n");
    } else {
        printf("\n❌ %d개의 테스트가 실패했습니다.\n", test_failures);
    }
    
    /* 메모리 누수 검사 */
    printf("\n메모리 상태:\n");
    btree_memory_print_stats(stdout);
    
    if (btree_memory_check_leaks()) {
        printf("⚠️  메모리 누수가 감지되었습니다.\n");
    } else {
        printf("✅ 메모리 누수 없음\n");
    }
}

/**
 * @brief 성능 테스트
 */
void run_performance_tests() {
    printf("\n\n성능 테스트 실행\n");
    printf("================\n");
    
    const int sizes[] = {1000, 5000, 10000, 50000};
    const int degrees[] = {5, 10, 16, 32};
    
    for (size_t d = 0; d < sizeof(degrees)/sizeof(degrees[0]); d++) {
        printf("\n차수 %d 테스트:\n", degrees[d]);
        
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            btree_test_int_t *tree = btree_test_int_create(degrees[d]);
            if (!tree) continue;
            
            int size = sizes[s];
            
            /* 삽입 성능 */
            clock_t start = clock();
            for (int i = 0; i < size; i++) {
                btree_test_int_insert(tree, i, i * 2);
            }
            clock_t end = clock();
            
            double insert_time = ((double)(end - start)) / CLOCKS_PER_SEC;
            
            /* 검색 성능 */
            start = clock();
            int found = 0;
            for (int i = 0; i < size; i++) {
                if (btree_test_int_search(tree, i)) {
                    found++;
                }
            }
            end = clock();
            
            double search_time = ((double)(end - start)) / CLOCKS_PER_SEC;
            
            printf("  크기 %d: 삽입 %.3fs (%.0f ops/s), 검색 %.3fs (%.0f ops/s), 높이 %d\n",
                   size, insert_time, size / insert_time,
                   search_time, size / search_time, btree_test_int_height(tree));
            
            btree_test_int_destroy(tree);
        }
    }
}

/**
 * @brief 메인 함수
 */
int main(int argc, char *argv[]) {
    bool run_perf_tests = false;
    
    /* 명령행 인수 처리 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--perf") == 0) {
            run_perf_tests = true;
        }
    }
    
    /* 기본 테스트 실행 */
    run_all_tests();
    
    /* 성능 테스트 실행 (옵션) */
    if (run_perf_tests) {
        run_performance_tests();
    }
    
    return test_failures > 0 ? 1 : 0;
}