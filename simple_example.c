#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "include/btree.h"

/* 정수형 B-Tree만 사용하는 간단한 예제 */
BTREE_DECLARE_INT_INT(simple);
BTREE_DEFINE_INT_INT(simple);

int main() {
    printf("B-Tree Simple Usage Example\n");
    printf("Version: %s\n\n", btree_version_string());
    
    /* Library initialization */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "Library initialization failed\n");
        return 1;
    }
    
    /* B-Tree creation */
    btree_simple_t *tree = btree_simple_create(5);
    if (!tree) {
        fprintf(stderr, "B-Tree creation failed\n");
        return 1;
    }
    
    printf("=== Basic Operations Test ===\n");
    
    /* Data insertion */
    printf("Data insertion:\n");
    for (int i = 1; i <= 10; i++) {
        btree_result_t result = btree_simple_insert(tree, i, i * 10);
        if (result == BTREE_SUCCESS) {
            printf("  %d -> %d insert success\n", i, i * 10);
        } else {
            printf("  %d insert failed: %s\n", i, btree_error_string(result));
        }
    }
    
    printf("\nTree status:\n");
    printf("  Size: %zu\n", btree_simple_size(tree));
    printf("  Height: %d\n", btree_simple_height(tree));
    printf("  Empty: %s\n", btree_simple_is_empty(tree) ? "Yes" : "No");
    
    /* Search test */
    printf("\n=== Search Test ===\n");
    int search_keys[] = {3, 7, 15};
    for (size_t i = 0; i < sizeof(search_keys)/sizeof(search_keys[0]); i++) {
        int key = search_keys[i];
        int *value = btree_simple_search(tree, key);
        if (value) {
            printf("  Key %d found: value = %d\n", key, *value);
        } else {
            printf("  Key %d not found\n", key);
        }
    }
    
    /* Performance test */
    printf("\n=== Performance Test ===\n");
    const int perf_size = 1000;
    
    btree_simple_t *perf_tree = btree_simple_create(16);
    if (perf_tree) {
        clock_t start = clock();
        
        /* Insert performance */
        for (int i = 0; i < perf_size; i++) {
            btree_simple_insert(perf_tree, i, i * 2);
        }
        clock_t mid = clock();
        
        /* Search performance */
        int found_count = 0;
        for (int i = 0; i < perf_size; i++) {
            if (btree_simple_search(perf_tree, i)) {
                found_count++;
            }
        }
        clock_t end = clock();
        
        double insert_time = ((double)(mid - start)) / CLOCKS_PER_SEC;
        double search_time = ((double)(end - mid)) / CLOCKS_PER_SEC;
        
        printf("  Insert: %d items, %.3fs (%.0f ops/s)\n", 
               perf_size, insert_time, perf_size / insert_time);
        printf("  Search: %d items, %d found, %.3fs (%.0f ops/s)\n", 
               perf_size, found_count, search_time, perf_size / search_time);
        printf("  Final tree size: %zu, height: %d\n", 
               btree_simple_size(perf_tree), btree_simple_height(perf_tree));
        
        btree_simple_destroy(perf_tree);
    }
    
    /* Memory usage */
    printf("\n=== Memory Usage ===\n");
    btree_memory_print_stats(stdout);
    
    /* Cleanup */
    btree_simple_destroy(tree);
    btree_library_cleanup();
    
    printf("\nExample completed!\n");
    return 0;
}