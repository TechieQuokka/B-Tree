/**
 * @file basic_usage.c
 * @brief B-Tree 라이브러리 기본 사용 예제
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/btree.h"

/* 정수형 B-Tree 정의 */
BTREE_DECLARE_INT_INT(int_int);
BTREE_DEFINE_INT_INT(int_int);

/* 문자열 B-Tree 정의 */  
BTREE_DECLARE_STRING_STRING(string_string);
BTREE_DEFINE_STRING_STRING(string_string);

/**
 * @brief 정수형 B-Tree 기본 예제
 */
void example_integer_btree() {
    printf("=== 정수형 B-Tree 예제 ===\n");
    
    /* B-Tree 생성 (차수 5) */
    btree_int_int_t *tree = btree_int_int_create(5);
    if (!tree) {
        fprintf(stderr, "B-Tree 생성 실패\n");
        return;
    }
    
    printf("B-Tree 생성됨 (차수: 5)\n");
    
    /* 데이터 삽입 */
    printf("데이터 삽입 중...\n");
    for (int i = 1; i <= 20; i++) {
        btree_result_t result = btree_int_int_insert(tree, i, i * 10);
        if (result != BTREE_SUCCESS) {
            fprintf(stderr, "삽입 실패: %s\n", btree_error_string(result));
            continue;
        }
        printf("삽입: %d -> %d\n", i, i * 10);
    }
    
    printf("\n트리 크기: %zu\n", btree_int_int_size(tree));
    printf("트리 높이: %d\n", btree_int_int_height(tree));
    
    /* 검색 테스트 */
    printf("\n=== 검색 테스트 ===\n");
    int search_keys[] = {5, 10, 15, 25};
    for (int i = 0; i < 4; i++) {
        int key = search_keys[i];
        int *value = btree_int_int_search(tree, key);
        if (value) {
            printf("검색 성공: %d -> %d\n", key, *value);
        } else {
            printf("검색 실패: %d (키가 존재하지 않음)\n", key);
        }
    }
    
    /* 트리 구조 출력 */
    printf("\n=== 트리 구조 ===\n");
    btree_int_int_print(tree, stdout);
    
    /* 통계 정보 출력 */
    printf("\n=== 통계 정보 ===\n");
    btree_int_int_print_stats(tree, stdout);
    
    /* 반복자 테스트 */
    printf("\n=== 반복자 테스트 ===\n");
    btree_int_int_iterator_t *iter = btree_int_int_iterator_create(tree);
    if (iter) {
        int key, value;
        printf("모든 키-값 쌍:\n");
        while (btree_int_int_iterator_next(iter, &key, &value)) {
            printf("  %d -> %d\n", key, value);
        }
        btree_int_int_iterator_destroy(iter);
    }
    
    /* 정리 */
    btree_int_int_destroy(tree);
    printf("\nB-Tree 정리 완료\n");
}

/**
 * @brief 문자열 B-Tree 예제
 */
void example_string_btree() {
    printf("\n\n=== 문자열 B-Tree 예제 ===\n");
    
    /* B-Tree 생성 */
    btree_string_string_t *dict = btree_string_string_create(10);
    if (!dict) {
        fprintf(stderr, "문자열 B-Tree 생성 실패\n");
        return;
    }
    
    printf("문자열 사전 B-Tree 생성됨\n");
    
    /* 영한 사전 데이터 */
    struct {
        const char *english;
        const char *korean;
    } words[] = {
        {"apple", "사과"},
        {"banana", "바나나"},
        {"cherry", "체리"},
        {"dog", "개"},
        {"elephant", "코끼리"},
        {"fish", "물고기"},
        {"grape", "포도"},
        {"house", "집"},
        {"ice", "얼음"},
        {"juice", "주스"}
    };
    
    size_t word_count = sizeof(words) / sizeof(words[0]);
    
    /* 단어 삽입 */
    printf("단어 삽입 중...\n");
    for (size_t i = 0; i < word_count; i++) {
        char *eng = strdup(words[i].english);
        char *kor = strdup(words[i].korean);
        
        btree_result_t result = btree_string_string_insert(dict, eng, kor);
        if (result != BTREE_SUCCESS) {
            fprintf(stderr, "삽입 실패: %s\n", btree_error_string(result));
            free(eng);
            free(kor);
            continue;
        }
        printf("삽입: %s -> %s\n", eng, kor);
    }
    
    printf("\n사전 크기: %zu\n", btree_string_string_size(dict));
    printf("사전 높이: %d\n", btree_string_string_height(dict));
    
    /* 단어 검색 */
    printf("\n=== 단어 검색 ===\n");
    const char *search_words[] = {"apple", "dog", "zebra", "house"};
    for (int i = 0; i < 4; i++) {
        char *search_key = strdup(search_words[i]);
        char **translation = btree_string_string_search(dict, search_key);
        if (translation && *translation) {
            printf("번역: %s -> %s\n", search_key, *translation);
        } else {
            printf("번역 실패: %s (단어를 찾을 수 없음)\n", search_key);
        }
        free(search_key);
    }
    
    /* 트리 구조 출력 */
    printf("\n=== 사전 구조 ===\n");
    btree_string_string_print(dict, stdout);
    
    /* 정리 */
    btree_string_string_destroy(dict);
    printf("\n문자열 B-Tree 정리 완료\n");
}

/**
 * @brief 성능 벤치마크
 */
void benchmark_performance() {
    printf("\n\n=== 성능 벤치마크 ===\n");
    
    const int test_size = 10000;
    btree_int_int_t *tree = btree_int_int_create(16);
    if (!tree) {
        fprintf(stderr, "벤치마크용 B-Tree 생성 실패\n");
        return;
    }
    
    /* 삽입 성능 측정 */
    clock_t start = clock();
    for (int i = 0; i < test_size; i++) {
        btree_int_int_insert(tree, i, i * 2);
    }
    clock_t end = clock();
    
    double insert_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("삽입 성능: %d개 항목, %.3f초 (%.0f ops/sec)\n", 
           test_size, insert_time, test_size / insert_time);
    
    /* 검색 성능 측정 */
    start = clock();
    int found_count = 0;
    for (int i = 0; i < test_size; i++) {
        if (btree_int_int_search(tree, i)) {
            found_count++;
        }
    }
    end = clock();
    
    double search_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("검색 성능: %d개 검색, %d개 발견, %.3f초 (%.0f ops/sec)\n", 
           test_size, found_count, search_time, test_size / search_time);
    
    /* 메모리 사용량 */
    printf("메모리 사용량: %zu bytes\n", btree_memory_get_usage());
    
    /* 트리 통계 */
    printf("최종 트리 크기: %zu\n", btree_int_int_size(tree));
    printf("최종 트리 높이: %d\n", btree_int_int_height(tree));
    
    btree_int_int_destroy(tree);
}

/**
 * @brief 사용자 정의 구조체 예제
 */
typedef struct {
    int id;
    char name[32];
    double score;
} student_t;

/* 학생 비교 함수 (ID 기준) */
static int student_compare(const void *a, const void *b) {
    const student_t *sa = (const student_t*)a;
    const student_t *sb = (const student_t*)b;
    return (sa->id > sb->id) - (sa->id < sb->id);
}

/* 학생 출력 함수 */
static void student_print(const void *ptr, FILE *output) {
    const student_t *s = (const student_t*)ptr;
    fprintf(output, "{id:%d, name:\"%s\", score:%.1f}", 
            s->id, s->name, s->score);
}

/* 학생 B-Tree 정의 */
BTREE_DEFINE_BASIC_OPS(student_t, student)
BTREE_DECLARE(student_t, student_t, student_student)

/* 타입 정보 설정 */
static btree_type_info_t student_type_info = {
    .key_size = sizeof(student_t),
    .value_size = sizeof(student_t),
    .alignment = _Alignof(student_t),
    .type_name = "student_t",
    .type_id = BTREE_TYPE_ID(student_t),
    .compare = student_compare,
    .copy = btree_copy_student,
    .move = btree_move_student,
    .destroy = NULL,
    .print = student_print
};

/* 학생 B-Tree 생성 함수 */
btree_student_student_t* btree_student_student_create(int degree) {
    btree_student_student_t *tree = malloc(sizeof(btree_student_student_t));
    if (!tree) return NULL;
    
    if (!btree_init(&tree->base, degree, &student_type_info, 
                   &student_type_info, NULL)) {
        free(tree);
        return NULL;
    }
    
    return tree;
}

/* 학생 B-Tree 소멸 함수 */
void btree_student_student_destroy(btree_student_student_t *tree) {
    if (tree) {
        btree_cleanup(&tree->base);
        free(tree);
    }
}

/* 학생 삽입 함수 */
btree_result_t btree_student_student_insert(btree_student_student_t *tree, 
                                           student_t key, student_t value) {
    return btree_insert(&tree->base, &key, &value);
}

/* 학생 검색 함수 */
student_t* btree_student_student_search(btree_student_student_t *tree, student_t key) {
    return (student_t*)btree_search(&tree->base, &key);
}

void example_custom_struct() {
    printf("\n\n=== 사용자 정의 구조체 예제 ===\n");
    
    btree_student_student_t *students = btree_student_student_create(5);
    if (!students) {
        fprintf(stderr, "학생 B-Tree 생성 실패\n");
        return;
    }
    
    /* 학생 데이터 */
    student_t student_data[] = {
        {1001, "김철수", 85.5},
        {1003, "이영희", 92.3},
        {1002, "박민수", 78.9},
        {1005, "최지은", 96.7},
        {1004, "정태현", 88.1}
    };
    
    size_t student_count = sizeof(student_data) / sizeof(student_data[0]);
    
    /* 학생 정보 삽입 */
    printf("학생 정보 삽입:\n");
    for (size_t i = 0; i < student_count; i++) {
        btree_result_t result = btree_student_student_insert(students, 
                                                           student_data[i], 
                                                           student_data[i]);
        if (result == BTREE_SUCCESS) {
            printf("  삽입: ");
            student_print(&student_data[i], stdout);
            printf("\n");
        }
    }
    
    /* 학생 검색 */
    printf("\n학생 검색:\n");
    student_t search_key = {1003, "", 0.0};  /* ID로만 검색 */
    student_t *found = btree_student_student_search(students, search_key);
    if (found) {
        printf("  발견: ");
        student_print(found, stdout);
        printf("\n");
    } else {
        printf("  학생을 찾을 수 없습니다 (ID: %d)\n", search_key.id);
    }
    
    btree_student_student_destroy(students);
    printf("사용자 정의 구조체 예제 완료\n");
}

/**
 * @brief 메인 함수
 */
int main() {
    printf("B-Tree 라이브러리 기본 사용 예제\n");
    printf("버전: %s\n\n", btree_version_string());
    
    /* 라이브러리 초기화 */
    if (btree_library_init() != BTREE_SUCCESS) {
        fprintf(stderr, "라이브러리 초기화 실패\n");
        return 1;
    }
    
    /* 예제 실행 */
    example_integer_btree();
    example_string_btree();
    example_custom_struct();
    benchmark_performance();
    
    /* 메모리 통계 출력 */
    printf("\n=== 최종 메모리 통계 ===\n");
    btree_memory_print_stats(stdout);
    
    /* 메모리 누수 검사 */
    if (btree_memory_check_leaks()) {
        fprintf(stderr, "경고: 메모리 누수가 감지되었습니다!\n");
    } else {
        printf("메모리 누수 없음\n");
    }
    
    /* 라이브러리 정리 */
    btree_library_cleanup();
    
    printf("\n모든 예제가 완료되었습니다.\n");
    return 0;
}