#include <stdio.h>
#include <stdlib.h>
#include "include/btree.h"

BTREE_DECLARE_INT_INT(debug_multi);
BTREE_DEFINE_INT_INT(debug_multi);

int main() {
    printf("다중 삽입 디버그 테스트 시작\n");
    
    /* B-Tree 생성 */
    printf("B-Tree 생성 중... (차수: 5)\n");
    btree_debug_multi_t *tree = btree_debug_multi_create(5);
    
    if (tree == NULL) {
        printf("오류: B-Tree 생성 실패\n");
        return 1;
    }
    
    printf("성공: B-Tree가 생성되었습니다\n");
    
    /* 순차적 삽입 테스트 */
    printf("\n=== 순차적 삽입 테스트 ===\n");
    for (int i = 1; i <= 10; i++) {
        printf("삽입: %d -> %d\n", i, i * 10);
        btree_result_t result = btree_debug_multi_insert(tree, i, i * 10);
        if (result != BTREE_SUCCESS) {
            printf("오류: 삽입 실패 - %s (키: %d)\n", btree_error_string(result), i);
            break;
        } else {
            printf("성공: 삽입 완료\n");
            
            /* 트리 상태 출력 */
            printf("  현재 크기: %zu\n", btree_debug_multi_size(tree));
            printf("  현재 높이: %d\n", btree_debug_multi_height(tree));
        }
    }
    
    /* 검색 테스트 */
    printf("\n=== 검색 테스트 ===\n");
    for (int i = 1; i <= 10; i++) {
        printf("검색: %d\n", i);
        int *value = btree_debug_multi_search(tree, i);
        if (value) {
            printf("성공: 검색된 값 = %d\n", *value);
        } else {
            printf("오류: 키 %d를 찾을 수 없음\n", i);
            printf("마지막 오류: %s\n", btree_error_string(btree_get_last_error()));
        }
    }
    
    /* 정리 */
    printf("\nB-Tree 정리 중...\n");
    btree_debug_multi_destroy(tree);
    printf("정리 완료\n");
    
    return 0;
}