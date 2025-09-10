#include <stdio.h>
#include <stdlib.h>
#include "include/btree.h"

BTREE_DECLARE_INT_INT(debug);
BTREE_DEFINE_INT_INT(debug);

int main() {
    printf("Debug Test Started\n");
    
    /* B-Tree 생성 시도 */
    printf("Creating B-Tree... (degree: 5)\n");
    btree_debug_t *tree = btree_debug_create(5);
    
    if (tree == NULL) {
        printf("Error: B-Tree creation failed\n");
        printf("Last error: %s\n", btree_error_string(btree_get_last_error()));
        return 1;
    } else {
        printf("Success: B-Tree created\n");
    }
    
    /* 간단한 삽입 테스트 */
    printf("Insert test: 42 -> 84\n");
    btree_result_t result = btree_debug_insert(tree, 42, 84);
    if (result != BTREE_SUCCESS) {
        printf("Error: Insert failed - %s\n", btree_error_string(result));
    } else {
        printf("Success: Insert completed\n");
    }
    
    /* 검색 테스트 */
    printf("Search test: 42\n");
    int *value = btree_debug_search(tree, 42);
    if (value) {
        printf("Success: Found value = %d\n", *value);
    } else {
        printf("Error: Key not found\n");
    }
    
    /* 정리 */
    printf("Cleaning up B-Tree...\n");
    btree_debug_destroy(tree);
    printf("Cleanup completed\n");
    
    return 0;
}