#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "include/btree.h"

/* B-Tree sorting을 위한 정수형 B-Tree 정의 */
BTREE_DECLARE_INT_INT(sort);
BTREE_DEFINE_INT_INT(sort);

/**
 * @brief Generate random integer array
 * @param arr Output array
 * @param size Array size
 * @param min_val Minimum value (inclusive)
 * @param max_val Maximum value (inclusive)
 */
void generate_random_data(int *arr, size_t size, int min_val, int max_val) {
    if (!arr || size == 0 || min_val > max_val) return;
    
    for (size_t i = 0; i < size; i++) {
        arr[i] = min_val + (rand() % (max_val - min_val + 1));
    }
}

/**
 * @brief Print array (first n elements)
 * @param arr Array to print
 * @param size Array size
 * @param max_print Maximum elements to print
 */
void print_array(const int *arr, size_t size, size_t max_print) {
    if (!arr) return;
    
    size_t print_size = (size < max_print) ? size : max_print;
    printf("Array (%zu elements): [", size);
    
    for (size_t i = 0; i < print_size; i++) {
        printf("%d", arr[i]);
        if (i < print_size - 1) printf(", ");
    }
    
    if (size > max_print) {
        printf(", ... (%zu more)", size - max_print);
    }
    printf("]\n");
}

/**
 * @brief Verify if array is sorted in ascending order
 * @param arr Array to check
 * @param size Array size
 * @return true if sorted, false otherwise
 */
bool is_sorted(const int *arr, size_t size) {
    if (!arr || size <= 1) return true;
    
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Sort array using B-Tree (simpler approach with counting)
 * @param arr Array to sort
 * @param size Array size
 * @param degree B-Tree degree
 * @return true if successful, false otherwise
 */
bool btree_sort(int *arr, size_t size, int degree) {
    if (!arr || size == 0) return false;
    
    /* Create B-Tree for counting occurrences */
    btree_sort_t *tree = btree_sort_create(degree);
    if (!tree) {
        printf("Error: Failed to create B-Tree with degree %d\n", degree);
        return false;
    }
    
    printf("Inserting %zu elements into B-Tree...\n", size);
    clock_t start = clock();
    
    /* Insert elements, counting duplicates */
    for (size_t i = 0; i < size; i++) {
        int *existing = btree_sort_search(tree, arr[i]);
        if (existing) {
            /* Increment count */
            (*existing)++;
        } else {
            /* Insert with count = 1 */
            btree_result_t result = btree_sort_insert(tree, arr[i], 1);
            if (result != BTREE_SUCCESS) {
                printf("Error: Failed to insert element %d: %s\n", 
                       arr[i], btree_error_string(result));
                btree_sort_destroy(tree);
                return false;
            }
        }
    }
    
    clock_t mid = clock();
    double insert_time = ((double)(mid - start)) / CLOCKS_PER_SEC;
    printf("Insertion completed in %.3fs (%.0f ops/s)\n", 
           insert_time, size / insert_time);
    
    /* Extract sorted elements */
    printf("Extracting sorted elements...\n");
    
    /* Find min and max values */
    int min_val = arr[0], max_val = arr[0];
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < min_val) min_val = arr[i];
        if (arr[i] > max_val) max_val = arr[i];
    }
    
    /* Extract in sorted order */
    size_t output_index = 0;
    for (int val = min_val; val <= max_val && output_index < size; val++) {
        int *count = btree_sort_search(tree, val);
        if (count && *count > 0) {
            /* Add all occurrences of this value */
            for (int i = 0; i < *count && output_index < size; i++) {
                arr[output_index++] = val;
            }
        }
    }
    
    clock_t end = clock();
    double extract_time = ((double)(end - mid)) / CLOCKS_PER_SEC;
    printf("Extraction completed in %.3fs\n", extract_time);
    
    printf("Total sorting time: %.3fs\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    
    /* Cleanup */
    btree_sort_destroy(tree);
    
    return output_index == size;
}

/**
 * @brief Standard qsort comparison function for integers
 */
int compare_ints(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

/**
 * @brief Test B-Tree sorting with different configurations
 */
void run_sorting_tests() {
    const size_t test_sizes[] = {100, 1000, 5000, 10000};
    const int degrees[] = {3, 5, 16, 32};
    const size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    const size_t num_degrees = sizeof(degrees) / sizeof(degrees[0]);
    
    printf("=== B-Tree Sorting Performance Tests ===\n\n");
    
    for (size_t s = 0; s < num_sizes; s++) {
        size_t size = test_sizes[s];
        printf("Testing with %zu elements:\n", size);
        
        /* Generate test data */
        int *original = malloc(size * sizeof(int));
        int *btree_sorted = malloc(size * sizeof(int));
        int *qsort_sorted = malloc(size * sizeof(int));
        
        if (!original || !btree_sorted || !qsort_sorted) {
            printf("Error: Memory allocation failed\n");
            free(original);
            free(btree_sorted);
            free(qsort_sorted);
            continue;
        }
        
        generate_random_data(original, size, 1, (int)(size * 2));
        
        if (size <= 20) {
            printf("Original: ");
            print_array(original, size, size);
        }
        
        for (size_t d = 0; d < num_degrees; d++) {
            int degree = degrees[d];
            printf("\n  Degree %d:\n", degree);
            
            /* Copy data for B-Tree sorting */
            memcpy(btree_sorted, original, size * sizeof(int));
            
            /* Test B-Tree sorting */
            clock_t start = clock();
            bool btree_success = btree_sort(btree_sorted, size, degree);
            clock_t end = clock();
            
            double btree_time = ((double)(end - start)) / CLOCKS_PER_SEC;
            
            if (btree_success && is_sorted(btree_sorted, size)) {
                printf("    B-Tree sort: SUCCESS (%.3fs, %.0f ops/s)\n", 
                       btree_time, size / btree_time);
            } else {
                printf("    B-Tree sort: FAILED\n");
                if (!btree_success) printf("      Reason: Sorting function failed\n");
                if (!is_sorted(btree_sorted, size)) printf("      Reason: Result not sorted\n");
            }
            
            /* Compare with qsort for verification */
            if (d == 0) { // Only do this once per size
                memcpy(qsort_sorted, original, size * sizeof(int));
                
                start = clock();
                qsort(qsort_sorted, size, sizeof(int), compare_ints);
                end = clock();
                
                double qsort_time = ((double)(end - start)) / CLOCKS_PER_SEC;
                printf("    qsort:       %.3fs (%.0f ops/s) [reference]\n", 
                       qsort_time, size / qsort_time);
                
                /* Verify results match */
                bool results_match = (memcmp(btree_sorted, qsort_sorted, size * sizeof(int)) == 0);
                printf("    Results match qsort: %s\n", results_match ? "YES" : "NO");
                
                if (!results_match && size <= 20) {
                    printf("    B-Tree result: ");
                    print_array(btree_sorted, size, size);
                    printf("    qsort result:  ");
                    print_array(qsort_sorted, size, size);
                }
            }
        }
        
        if (size <= 20) {
            printf("\nFinal sorted: ");
            print_array(btree_sorted, size, size);
        }
        
        free(original);
        free(btree_sorted);
        free(qsort_sorted);
        printf("\n");
    }
}

/**
 * @brief Main function
 */
int main() {
    printf("B-Tree Sorting Test Program\n");
    printf("Version: %s\n", btree_version_string());
    printf("==================================\n\n");
    
    /* Initialize random seed */
    srand((unsigned int)time(NULL));
    
    /* Initialize B-Tree library */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize B-Tree library\n");
        return 1;
    }
    
    /* Run sorting tests */
    run_sorting_tests();
    
    /* Test with edge cases */
    printf("=== Edge Case Tests ===\n");
    
    /* Test with duplicates */
    printf("Testing with many duplicates:\n");
    int duplicates[] = {5, 2, 8, 2, 1, 5, 9, 1, 5, 2, 8, 1};
    size_t dup_size = sizeof(duplicates) / sizeof(duplicates[0]);
    
    printf("Original: ");
    print_array(duplicates, dup_size, dup_size);
    
    if (btree_sort(duplicates, dup_size, 5)) {
        printf("Sorted:   ");
        print_array(duplicates, dup_size, dup_size);
        printf("Is sorted: %s\n", is_sorted(duplicates, dup_size) ? "YES" : "NO");
    } else {
        printf("Sorting failed!\n");
    }
    
    /* Memory statistics */
    printf("\n=== Memory Statistics ===\n");
    btree_memory_print_stats(stdout);
    
    /* Cleanup */
    btree_library_cleanup();
    
    printf("\nSorting tests completed!\n");
    return 0;
}