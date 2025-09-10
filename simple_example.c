#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "include/btree.h"

/* 정수형 B-Tree만 사용하는 간단한 예제 */
BTREE_DECLARE_INT_INT(simple);
BTREE_DEFINE_INT_INT(simple);

int main() {
    printf("B-Tree 간단한 사용 예제\n");
    printf("버전: %s\n\n", btree_version_string());
    
    /* 라이브러리 초기화 */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "라이브러리 초기화 실패\n");
        return 1;
    }
    
    /* B-Tree 생성 */
    btree_simple_t *tree = btree_simple_create(5);
    if (!tree) {
        fprintf(stderr, "B-Tree 생성 실패\n");
        return 1;
    }
    
    printf("=== 기본 연산 테스트 ===\n");
    
    /* 데이터 삽입 */
    printf("데이터 삽입:\n");
    for (int i = 1; i <= 10; i++) {
        btree_result_t result = btree_simple_insert(tree, i, i * 10);
        if (result == BTREE_SUCCESS) {
            printf("  %d -> %d 삽입 성공\n", i, i * 10);
        } else {
            printf("  %d 삽입 실패: %s\n", i, btree_error_string(result));
        }
    }
    
    printf("\n트리 상태:\n");
    printf("  크기: %zu\n", btree_simple_size(tree));
    printf("  높이: %d\n", btree_simple_height(tree));
    printf("  비어있음: %s\n", btree_simple_is_empty(tree) ? "예" : "아니오");
    
    /* 검색 테스트 */
    printf("\n=== 검색 테스트 ===\n");
    int search_keys[] = {3, 7, 15};
    for (size_t i = 0; i < sizeof(search_keys)/sizeof(search_keys[0]); i++) {
        int key = search_keys[i];
        int *value = btree_simple_search(tree, key);
        if (value) {
            printf("  키 %d 검색 성공: 값 = %d\n", key, *value);
        } else {
            printf("  키 %d 검색 실패: 존재하지 않음\n", key);
        }
    }
    
    /* 성능 테스트 */
    printf("\n=== 성능 테스트 ===\n");
    const int perf_size = 1000;
    
    btree_simple_t *perf_tree = btree_simple_create(16);
    if (perf_tree) {
        clock_t start = clock();
        
        /* 삽입 성능 */
        for (int i = 0; i < perf_size; i++) {
            btree_simple_insert(perf_tree, i, i * 2);
        }
        clock_t mid = clock();
        
        /* 검색 성능 */
        int found_count = 0;
        for (int i = 0; i < perf_size; i++) {
            if (btree_simple_search(perf_tree, i)) {
                found_count++;
            }
        }
        clock_t end = clock();
        
        double insert_time = ((double)(mid - start)) / CLOCKS_PER_SEC;
        double search_time = ((double)(end - mid)) / CLOCKS_PER_SEC;
        
        printf("  삽입: %d개, %.3f초 (%.0f ops/s)\n", 
               perf_size, insert_time, perf_size / insert_time);
        printf("  검색: %d개, %d개 발견, %.3f초 (%.0f ops/s)\n", 
               perf_size, found_count, search_time, perf_size / search_time);
        printf("  최종 트리 크기: %zu, 높이: %d\n", 
               btree_simple_size(perf_tree), btree_simple_height(perf_tree));
        
        btree_simple_destroy(perf_tree);
    }
    
    /* 메모리 정보 */
    printf("\n=== 메모리 사용량 ===\n");
    btree_memory_print_stats(stdout);
    
    /* 정리 */
    btree_simple_destroy(tree);
    btree_library_cleanup();
    
    printf("\n예제 완료!\n");
    return 0;
}