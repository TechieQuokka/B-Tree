#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "include/btree.h"

/* B-Tree for verification testing */
BTREE_DECLARE_INT_INT(verify);
BTREE_DEFINE_INT_INT(verify);

/**
 * @brief Value pair structure for sorting
 */
typedef struct {
    int value;
    size_t index;
} value_pair_t;

/**
 * @brief Comparison function for value pairs
 */
int compare_value_pairs(const void *a, const void *b) {
    const value_pair_t *pa = (const value_pair_t*)a;
    const value_pair_t *pb = (const value_pair_t*)b;
    
    if (pa->value < pb->value) return -1;
    if (pa->value > pb->value) return 1;
    return (pa->index < pb->index) ? -1 : (pa->index > pb->index) ? 1 : 0;
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
 * @brief Custom random data generation function
 * @param arr Output array
 * @param size Array size
 * @param min_val Minimum value (inclusive)
 * @param max_val Maximum value (inclusive)
 */
void generate_random_function(int *arr, size_t size, int min_val, int max_val) {
    if (!arr || size == 0 || min_val > max_val) {
        printf("Error: Invalid parameters for random generation\n");
        return;
    }
    
    printf("Generating %zu unique random numbers between %d and %d...\n", size, min_val, max_val);
    
    /* Use simple approach to ensure uniqueness */
    bool *used = calloc(max_val - min_val + 1, sizeof(bool));
    if (!used) {
        printf("Error: Memory allocation failed for uniqueness tracking\n");
        return;
    }
    
    size_t generated = 0;
    while (generated < size) {
        int val = min_val + (rand() % (max_val - min_val + 1));
        int index = val - min_val;
        
        if (!used[index]) {
            used[index] = true;
            arr[generated++] = val;
        }
    }
    
    free(used);
    
    printf("Random data generation completed!\n");
}

/**
 * @brief Print array with formatting
 * @param title Array title
 * @param arr Array to print
 * @param size Array size
 * @param cols Number of columns for formatting
 */
void print_formatted_array(const char *title, const int *arr, size_t size, int cols) {
    if (!arr || !title) return;
    
    printf("\n%s (%zu elements):\n", title, size);
    printf("+");
    for (int i = 0; i < cols * 6 - 1; i++) printf("-");
    printf("+\n");
    
    for (size_t i = 0; i < size; i++) {
        if (i % cols == 0) printf("|");
        printf("%5d", arr[i]);
        if ((i + 1) % cols == 0 || i == size - 1) {
            if (i == size - 1 && (i + 1) % cols != 0) {
                int remaining = cols - ((i + 1) % cols);
                for (int j = 0; j < remaining * 5; j++) printf(" ");
            }
            printf(" |\n");
        }
    }
    
    printf("+");
    for (int i = 0; i < cols * 6 - 1; i++) printf("-");
    printf("+\n");
}

/**
 * @brief Comprehensive sorting verification
 * @param arr Array to check
 * @param size Array size
 * @return true if all checks pass
 */
bool comprehensive_sort_verification(const int *arr, size_t size) {
    if (!arr || size == 0) return false;
    
    printf("\n=== Comprehensive Sorting Verification ===\n");
    
    bool all_passed = true;
    
    /* Test 1: Basic sorted order check */
    printf("1. Basic sorted order check: ");
    bool basic_sorted = true;
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) {
            basic_sorted = false;
            printf("FAILED at index %zu: %d > %d\n", i, arr[i-1], arr[i]);
            all_passed = false;
            break;
        }
    }
    if (basic_sorted) printf("PASSED\n");
    
    /* Test 2: Min/Max values */
    printf("2. Min/Max value verification: ");
    int min_val = arr[0], max_val = arr[0];
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < min_val) min_val = arr[i];
        if (arr[i] > max_val) max_val = arr[i];
    }
    
    if (arr[0] == min_val && arr[size-1] == max_val) {
        printf("PASSED (min=%d, max=%d)\n", min_val, max_val);
    } else {
        printf("FAILED (expected min=%d at start, max=%d at end)\n", min_val, max_val);
        all_passed = false;
    }
    
    /* Test 3: Element count verification */
    printf("3. Element count verification: ");
    /* Count unique elements */
    int unique_count = 1;
    for (size_t i = 1; i < size; i++) {
        if (arr[i] != arr[i-1]) {
            unique_count++;
        }
    }
    printf("PASSED (%zu total elements, %d unique values)\n", size, unique_count);
    
    /* Test 4: Uniqueness check (no duplicates allowed) */
    printf("4. Uniqueness check: ");
    bool all_unique = true;
    int duplicates_found = 0;
    for (size_t i = 1; i < size; i++) {
        if (arr[i] == arr[i-1]) {
            if (all_unique) {
                printf("FAILED - duplicate %d found at indices %zu and %zu\n", arr[i], i-1, i);
                all_unique = false;
            }
            duplicates_found++;
        }
    }
    if (all_unique) {
        printf("PASSED (all elements unique)\n");
    } else {
        printf("Total duplicates found: %d\n", duplicates_found);
        all_passed = false;
    }
    
    /* Test 5: Range verification */
    printf("5. Value range verification: ");
    bool range_valid = true;
    int expected_min = 0, expected_max = 10000; // Based on generation parameters
    for (size_t i = 0; i < size; i++) {
        if (arr[i] < expected_min || arr[i] > expected_max) {
            printf("FAILED - value %d out of range [%d, %d]\n", arr[i], expected_min, expected_max);
            range_valid = false;
            all_passed = false;
            break;
        }
    }
    if (range_valid) printf("PASSED (all values in range [%d, %d])\n", expected_min, expected_max);
    
    printf("\nOverall verification result: %s\n", all_passed ? "[OK] ALL TESTS PASSED" : "[FAIL] SOME TESTS FAILED");
    return all_passed;
}

/**
 * @brief B-Tree traversal sorting implementation
 * @param arr Array to sort
 * @param size Array size
 * @param degree B-Tree degree
 * @return true if successful
 */
bool btree_traversal_sort_detailed(int *arr, size_t size, int degree) {
    if (!arr || size == 0) return false;
    
    printf("\n=== B-Tree Sorting Process (degree %d) ===\n", degree);
    
    /* Create B-Tree */
    btree_verify_t *tree = btree_verify_create(degree);
    if (!tree) {
        printf("Error: Failed to create B-Tree\n");
        return false;
    }
    
    printf("Step 1: Inserting %zu unique elements into B-Tree...\n", size);
    clock_t start = clock();
    
    /* Insert unique values (no duplicates) */
    int successful_insertions = 0;
    int insertion_failures = 0;
    
    for (size_t i = 0; i < size; i++) {
        /* Insert with value as key, dummy value of 1 */
        btree_result_t result = btree_verify_insert(tree, arr[i], 1);
        if (result == BTREE_SUCCESS) {
            successful_insertions++;
        } else {
            printf("Warning: Failed to insert %d: %s\n", arr[i], btree_error_string(result));
            insertion_failures++;
        }
        
        /* Progress indicator */
        if ((i + 1) % 100 == 0 || i == size - 1) {
            printf("  Progress: %zu/%zu elements processed\r", i + 1, size);
            fflush(stdout);
        }
    }
    printf("\n");
    
    clock_t mid = clock();
    double insert_time = ((double)(mid - start)) / CLOCKS_PER_SEC;
    
    printf("Step 2: B-Tree construction completed\n");
    printf("  - Insertion time: %.4fs\n", insert_time);
    printf("  - Unique elements stored: %zu\n", btree_verify_size(tree));
    printf("  - Tree height: %d levels\n", btree_verify_height(tree));
    printf("  - Successful insertions: %d\n", successful_insertions);
    printf("  - Insertion failures: %d\n", insertion_failures);
    printf("  - Average insertion rate: %.0f ops/s\n", size / insert_time);
    
    printf("Step 3: Extracting elements in sorted order...\n");
    
    /* Find value range */
    int min_val = arr[0], max_val = arr[0];
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < min_val) min_val = arr[i];
        if (arr[i] > max_val) max_val = arr[i];
    }
    
    printf("  - Value range: [%d, %d]\n", min_val, max_val);
    
    /* Extract in sorted order by iterating through possible values */
    size_t output_index = 0;
    int values_found = 0;
    
    for (int val = min_val; val <= max_val && output_index < size; val++) {
        int *found = btree_verify_search(tree, val);
        if (found) {
            arr[output_index++] = val;
            values_found++;
        }
    }
    
    clock_t end = clock();
    double extract_time = ((double)(end - mid)) / CLOCKS_PER_SEC;
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Step 4: Extraction completed\n");
    printf("  - Extraction time: %.4fs\n", extract_time);
    printf("  - Values found: %d\n", values_found);
    printf("  - Elements extracted: %zu/%zu\n", output_index, size);
    printf("  - Total sorting time: %.4fs\n", total_time);
    printf("  - Overall sorting rate: %.0f ops/s\n", size / total_time);
    
    /* Cleanup */
    btree_verify_destroy(tree);
    
    return output_index == size;
}



/**
 * @brief Compare with standard qsort
 * @param original Original array
 * @param size Array size
 */
void compare_with_qsort(const int *original, size_t size) {
    printf("\n=== Comparison with Standard qsort ===\n");
    
    /* Create copies for both algorithms */
    int *btree_copy = malloc(size * sizeof(int));
    int *qsort_copy = malloc(size * sizeof(int));
    
    if (!btree_copy || !qsort_copy) {
        printf("Error: Memory allocation failed\n");
        free(btree_copy);
        free(qsort_copy);
        return;
    }
    
    memcpy(btree_copy, original, size * sizeof(int));
    memcpy(qsort_copy, original, size * sizeof(int));
    
    /* B-Tree sorting */
    printf("Testing B-Tree sorting...\n");
    clock_t start = clock();
    bool btree_success = btree_traversal_sort_detailed(btree_copy, size, 16);
    clock_t mid = clock();
    double btree_time = ((double)(mid - start)) / CLOCKS_PER_SEC;
    
    /* qsort sorting */
    printf("\nTesting standard qsort...\n");
    
    start = clock();
    qsort(qsort_copy, size, sizeof(int), compare_ints);
    clock_t end = clock();
    double qsort_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    /* Results comparison */
    printf("\n--- Performance Comparison ---\n");
    printf("B-Tree sort: %.4fs (%.0f ops/s) - %s\n", 
           btree_time, size / btree_time, btree_success ? "SUCCESS" : "FAILED");
    printf("qsort:       %.4fs (%.0f ops/s) - SUCCESS\n", 
           qsort_time, size / qsort_time);
    
    if (btree_time > 0 && qsort_time > 0) {
        printf("Speed ratio: qsort is %.2fx %s than B-Tree sort\n", 
               btree_time / qsort_time, 
               btree_time > qsort_time ? "faster" : "slower");
    }
    
    /* Correctness verification */
    bool results_match = (memcmp(btree_copy, qsort_copy, size * sizeof(int)) == 0);
    printf("Results match: %s\n", results_match ? "[OK] YES" : "[FAIL] NO");
    
    if (!results_match) {
        printf("First 10 elements comparison:\n");
        printf("B-Tree: ");
        for (int i = 0; i < 10 && i < (int)size; i++) printf("%d ", btree_copy[i]);
        printf("\nqsort:  ");
        for (int i = 0; i < 10 && i < (int)size; i++) printf("%d ", qsort_copy[i]);
        printf("\n");
    }
    
    free(btree_copy);
    free(qsort_copy);
}

/**
 * @brief Main verification program
 */
int main() {
    printf("+===========================================+\n");
    printf("|    B-Tree Large Data Verification      |\n");
    printf("|          Version: %s               |\n", btree_version_string());
    printf("+===========================================+\n\n");
    
    /* Initialize random seed */
    srand((unsigned int)time(NULL));
    
    /* Initialize B-Tree library */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize B-Tree library\n");
        return 1;
    }
    
    /* Test with 1000 elements */
    const size_t test_size = 1000;
    printf("Testing with %zu elements...\n", test_size);
    
    /* Allocate memory */
    int *original_data = malloc(test_size * sizeof(int));
    int *sorted_data = malloc(test_size * sizeof(int));
    
    if (!original_data || !sorted_data) {
        printf("Error: Memory allocation failed\n");
        free(original_data);
        free(sorted_data);
        btree_library_cleanup();
        return 1;
    }
    
    /* Generate unique random data using custom function */
    generate_random_function(original_data, test_size, 0, 10000);
    
    /* Display original data */
    print_formatted_array("Original Random Data", original_data, test_size, 10);
    
    /* Copy for sorting */
    memcpy(sorted_data, original_data, test_size * sizeof(int));
    
    /* Perform B-Tree sorting */
    printf("\n=== Starting B-Tree Sorting Process ===\n");
    bool sorting_success = btree_traversal_sort_detailed(sorted_data, test_size, 16);
    
    if (sorting_success) {
        /* Display sorted data */
        print_formatted_array("B-Tree Sorted Data", sorted_data, test_size, 10);
        
        /* Comprehensive verification */
        bool verification_passed = comprehensive_sort_verification(sorted_data, test_size);
        
        if (verification_passed) {
            printf("\n[SUCCESS] B-Tree sorting verification: PASSED!\n");
        } else {
            printf("\n[FAIL] B-Tree sorting verification: FAILED!\n");
        }
        
        /* Compare with qsort */
        compare_with_qsort(original_data, test_size);
        
    } else {
        printf("\n[FAIL] B-Tree sorting failed!\n");
    }
    
    /* Memory statistics */
    printf("\n=== Memory Usage Statistics ===\n");
    btree_memory_print_stats(stdout);
    
    printf("\n=== Verification Complete ===\n");
    
    bool final_result = sorting_success;
    if (sorting_success) {
        final_result = comprehensive_sort_verification(sorted_data, test_size);
    }
    
    /* Cleanup */
    free(original_data);
    free(sorted_data);
    btree_library_cleanup();
    
    return final_result ? 0 : 1;
}