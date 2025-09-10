#include <stdio.h>
#include <stdlib.h>
#include "include/btree.h"

BTREE_DECLARE_INT_INT(debug);
BTREE_DEFINE_INT_INT(debug);

int main() {
    printf("디버그 테스트 시작\n");
    
    /* B-Tree 생성 시도 */
    printf("B-Tree 생성 중... (차수: 5)\n");
    btree_debug_t *tree = btree_debug_create(5);
    
    if (tree == NULL) {
        printf("오류: B-Tree 생성 실패\n");
        printf("마지막 오류: %s\n", btree_error_string(btree_get_last_error()));
        return 1;
    } else {
        printf("성공: B-Tree가 생성되었습니다\n");
    }
    
    /* 간단한 삽입 테스트 */
    printf("삽입 테스트: 42 -> 84\n");
    btree_result_t result = btree_debug_insert(tree, 42, 84);
    if (result != BTREE_SUCCESS) {
        printf("오류: 삽입 실패 - %s\n", btree_error_string(result));
    } else {
        printf("성공: 삽입 완료\n");
    }
    
    /* 검색 테스트 */
    printf("검색 테스트: 42\n");
    int *value = btree_debug_search(tree, 42);
    if (value) {
        printf("성공: 검색된 값 = %d\n", *value);
    } else {
        printf("오류: 키를 찾을 수 없음\n");
    }
    
    /* 정리 */
    printf("B-Tree 정리 중...\n");
    btree_debug_destroy(tree);
    printf("정리 완료\n");
    
    return 0;
}