#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "include/btree.h"

/* B-Tree traversal sorting을 위한 정수형 B-Tree 정의 */
BTREE_DECLARE_INT_INT(traversal);
BTREE_DEFINE_INT_INT(traversal);

/* Global array for collecting traversal results */
static int *traversal_result = NULL;
static size_t traversal_index = 0;
static size_t traversal_capacity = 0;

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
 * @brief Callback function for B-Tree traversal
 * This function will be called for each key-value pair during traversal
 */
void traversal_callback(int key, int value, void *user_data) {
    (void)value;      // We only care about keys for sorting
    (void)user_data;  // Not used in this implementation
    
    if (traversal_result && traversal_index < traversal_capacity) {
        traversal_result[traversal_index++] = key;
    }
}

/**
 * @brief Sort array using B-Tree in-order traversal
 * @param arr Array to sort
 * @param size Array size
 * @param degree B-Tree degree
 * @return true if successful, false otherwise
 */
bool btree_traversal_sort(int *arr, size_t size, int degree) {
    if (!arr || size == 0) return false;
    
    /* Create B-Tree */
    btree_traversal_t *tree = btree_traversal_create(degree);
    if (!tree) {
        printf("Error: Failed to create B-Tree with degree %d\n", degree);
        return false;
    }
    
    printf("Inserting %zu elements into B-Tree (degree %d)...\n", size, degree);
    clock_t start = clock();
    
    /* Insert all unique elements with their counts */
    for (size_t i = 0; i < size; i++) {
        int *existing = btree_traversal_search(tree, arr[i]);
        if (existing) {
            /* Increment count for duplicates */
            (*existing)++;
        } else {
            /* Insert new element with count = 1 */
            btree_result_t result = btree_traversal_insert(tree, arr[i], 1);
            if (result != BTREE_SUCCESS) {
                printf("Error: Failed to insert element %d: %s\n", 
                       arr[i], btree_error_string(result));
                btree_traversal_destroy(tree);
                return false;
            }
        }
    }
    
    clock_t mid = clock();
    double insert_time = ((double)(mid - start)) / CLOCKS_PER_SEC;
    printf("Insertion completed in %.3fs (%.0f ops/s)\n", 
           insert_time, size / insert_time);
    printf("Tree size: %zu, Tree height: %d\n", 
           btree_traversal_size(tree), btree_traversal_height(tree));
    
    /* Prepare for traversal */
    printf("Performing in-order traversal...\n");
    
    /* Setup global traversal state */
    traversal_result = malloc(size * sizeof(int));
    if (!traversal_result) {
        printf("Error: Memory allocation failed for traversal result\n");
        btree_traversal_destroy(tree);
        return false;
    }
    traversal_index = 0;
    traversal_capacity = size;
    
    /* Simulate in-order traversal by iterating through sorted keys */
    /* Since we don't have direct traversal API, we'll extract in sorted order */
    
    /* Find min and max values for range traversal */
    int min_val = arr[0], max_val = arr[0];
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < min_val) min_val = arr[i];
        if (arr[i] > max_val) max_val = arr[i];
    }
    
    /* Extract elements in sorted order (simulating in-order traversal) */
    size_t output_index = 0;
    for (int val = min_val; val <= max_val && output_index < size; val++) {
        int *count = btree_traversal_search(tree, val);
        if (count && *count > 0) {
            /* Add all occurrences of this value (sorted output) */
            for (int i = 0; i < *count && output_index < size; i++) {
                arr[output_index++] = val;
            }
        }
    }
    
    clock_t end = clock();
    double traversal_time = ((double)(end - mid)) / CLOCKS_PER_SEC;
    printf("Traversal completed in %.3fs\n", traversal_time);
    printf("Total sorting time: %.3fs\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    
    /* Cleanup */
    free(traversal_result);
    traversal_result = NULL;
    btree_traversal_destroy(tree);
    
    return output_index == size;
}

/**
 * @brief Standard qsort comparison function
 */
int compare_ints(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

/**
 * @brief Demonstrate B-Tree structure visualization
 */
void demonstrate_btree_structure(const int *data, size_t size, int degree) {
    printf("\n=== B-Tree Structure Demonstration (degree %d) ===\n", degree);
    
    btree_traversal_t *tree = btree_traversal_create(degree);
    if (!tree) {
        printf("Error: Failed to create demonstration tree\n");
        return;
    }
    
    printf("Original data: ");
    print_array(data, size, size);
    
    printf("\nInserting elements step by step:\n");
    for (size_t i = 0; i < size; i++) {
        btree_result_t result = btree_traversal_insert(tree, data[i], data[i]);
        if (result == BTREE_SUCCESS) {
            printf("Inserted %d -> Tree size: %zu, height: %d\n", 
                   data[i], btree_traversal_size(tree), btree_traversal_height(tree));
        } else if (result == BTREE_ERROR_DUPLICATE_KEY) {
            printf("Skipped duplicate %d\n", data[i]);
        }
    }
    
    printf("\nFinal tree properties:\n");
    printf("- Size: %zu unique elements\n", btree_traversal_size(tree));
    printf("- Height: %d levels\n", btree_traversal_height(tree));
    printf("- Empty: %s\n", btree_traversal_is_empty(tree) ? "Yes" : "No");
    
    /* Demonstrate sorted extraction */
    printf("\nExtracting in sorted order (simulated in-order traversal):\n");
    int *sorted_data = malloc(size * sizeof(int));
    if (sorted_data) {
        memcpy(sorted_data, data, size * sizeof(int));
        
        if (btree_traversal_sort(sorted_data, size, degree)) {
            printf("Sorted result: ");
            print_array(sorted_data, size, size);
            printf("Is correctly sorted: %s\n", is_sorted(sorted_data, size) ? "YES" : "NO");
        }
        
        free(sorted_data);
    }
    
    btree_traversal_destroy(tree);
}

/**
 * @brief Run comprehensive tests
 */
void run_sorting_tests() {
    const size_t test_sizes[] = {10, 100, 1000, 5000, 10000};
    const int degrees[] = {3, 5, 16, 32};
    
    printf("=== B-Tree Traversal Sorting Tests ===\n\n");
    
    for (size_t s = 0; s < sizeof(test_sizes)/sizeof(test_sizes[0]); s++) {
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
        
        generate_random_data(original, size, 1, (int)(size / 2)); // More duplicates
        
        if (size <= 20) {
            printf("Original: ");
            print_array(original, size, size);
        }
        
        for (size_t d = 0; d < sizeof(degrees)/sizeof(degrees[0]); d++) {
            int degree = degrees[d];
            printf("\n  Degree %d:\n", degree);
            
            /* Test B-Tree sorting */
            memcpy(btree_sorted, original, size * sizeof(int));
            
            clock_t start = clock();
            bool success = btree_traversal_sort(btree_sorted, size, degree);
            clock_t end = clock();
            
            double btree_time = ((double)(end - start)) / CLOCKS_PER_SEC;
            
            if (success && is_sorted(btree_sorted, size)) {
                printf("    B-Tree sort: SUCCESS (%.3fs, %.0f ops/s)\n", 
                       btree_time, size / btree_time);
            } else {
                printf("    B-Tree sort: FAILED\n");
                if (!success) printf("      Reason: Sorting function failed\n");
                if (!is_sorted(btree_sorted, size)) printf("      Reason: Result not sorted\n");
            }
            
            /* Compare with qsort for first degree */
            if (d == 0) {
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
            }
            
            if (size <= 20) {
                printf("    Sorted: ");
                print_array(btree_sorted, size, size);
            }
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
    printf("B-Tree In-Order Traversal Sorting Test\n");
    printf("Version: %s\n", btree_version_string());
    printf("=======================================\n\n");
    
    /* Initialize random seed */
    srand((unsigned int)time(NULL));
    
    /* Initialize B-Tree library */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize B-Tree library\n");
        return 1;
    }
    
    /* Demonstrate with small dataset first */
    int demo_data[] = {7, 3, 11, 1, 5, 9, 13, 2, 4, 6, 8, 10, 12, 14};
    size_t demo_size = sizeof(demo_data) / sizeof(demo_data[0]);
    
    demonstrate_btree_structure(demo_data, demo_size, 5);
    
    /* Test with duplicates */
    printf("\n=== Duplicate Handling Test ===\n");
    int dup_data[] = {5, 2, 8, 2, 1, 5, 9, 1, 5, 2, 8, 1};
    size_t dup_size = sizeof(dup_data) / sizeof(dup_data[0]);
    
    demonstrate_btree_structure(dup_data, dup_size, 3);
    
    /* Run comprehensive tests */
    printf("\n");
    run_sorting_tests();
    
    /* Memory statistics */
    printf("=== Memory Statistics ===\n");
    btree_memory_print_stats(stdout);
    
    /* Cleanup */
    btree_library_cleanup();
    
    printf("\nB-Tree traversal sorting tests completed!\n");
    return 0;
}