/**
 * @file btree_core.c
 * @brief B-Tree 핵심 알고리즘 구현
 */

#include "../include/btree.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 전역 오류 상태 - C99에서는 thread_local 대신 static 사용 */
static btree_result_t g_last_error = BTREE_SUCCESS;

/* 오류 설정 함수 */
static void btree_set_error(btree_result_t error) {
    g_last_error = error;
}

/* 키 포인터 계산 */
static inline void* btree_get_key_ptr(const btree_node_t *node, int index,
                                     const btree_type_info_t *key_type) {
    return (char*)node->keys + (index * key_type->key_size);
}

/* 값 포인터 계산 */
static inline void* btree_get_value_ptr(const btree_node_t *node, int index,
                                       const btree_type_info_t *value_type) {
    return (char*)node->values + (index * value_type->value_size);
}

/**
 * @brief B-Tree 초기화
 */
btree_result_t btree_init(btree_t *tree, int degree,
                         const btree_type_info_t *key_type,
                         const btree_type_info_t *value_type,
                         btree_allocator_t *allocator) {
    if (!tree || !key_type || !value_type) {
        return btree_set_error(BTREE_ERROR_NULL_POINTER), BTREE_ERROR_NULL_POINTER;
    }

    if (degree < BTREE_MIN_DEGREE || degree > BTREE_MAX_DEGREE) {
        return btree_set_error(BTREE_ERROR_INVALID_DEGREE), BTREE_ERROR_INVALID_DEGREE;
    }

    /* 기본값 초기화 */
    memset(tree, 0, sizeof(btree_t));
    
    tree->degree = degree;
    tree->max_keys = 2 * degree - 1;
    tree->min_keys = degree - 1;
    tree->height = 0;
    
    /* 타입 정보 복사 */
    memcpy(&tree->key_type, key_type, sizeof(btree_type_info_t));
    memcpy(&tree->value_type, value_type, sizeof(btree_type_info_t));
    
    /* 할당자 설정 */
    tree->allocator = allocator ? allocator : btree_default_allocator();
    
    /* 통계 초기화 */
    tree->node_count = 0;
    tree->key_count = 0;
    tree->total_memory = sizeof(btree_t);
    
    return BTREE_SUCCESS;
}

/**
 * @brief B-Tree 정리
 */
void btree_cleanup(btree_t *tree) {
    if (!tree) return;
    
    btree_clear(tree);
    
    /* 통계 리셋 */
    tree->node_count = 0;
    tree->key_count = 0;
    tree->total_memory = 0;
}

/**
 * @brief 노드 생성
 */
btree_node_t* btree_node_create(btree_t *tree, bool is_leaf) {
    if (!tree || !tree->allocator) return NULL;
    
    btree_node_t *node = tree->allocator->alloc(sizeof(btree_node_t));
    if (!node) {
        btree_set_error(BTREE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    /* 노드 초기화 */
    memset(node, 0, sizeof(btree_node_t));
    node->is_leaf = is_leaf ? 1 : 0;
    node->num_keys = 0;
    node->capacity = tree->max_keys;
    node->ref_count = 1;
    
    /* 키 배열 할당 */
    size_t key_array_size = tree->max_keys * tree->key_type.key_size;
    node->keys = tree->allocator->alloc(key_array_size);
    if (!node->keys) {
        tree->allocator->free(node);
        btree_set_error(BTREE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    /* 값 배열 할당 (표준 B-Tree에서는 모든 노드에서 값 저장) */
    size_t value_array_size = tree->max_keys * tree->value_type.value_size;
    node->values = tree->allocator->alloc(value_array_size);
    if (!node->values) {
        tree->allocator->free(node->keys);
        tree->allocator->free(node);
        btree_set_error(BTREE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    /* 자식 배열 할당 (내부 노드만) */
    if (!is_leaf) {
        size_t children_array_size = (tree->max_keys + 1) * sizeof(btree_node_t*);
        node->children = tree->allocator->alloc(children_array_size);
        if (!node->children) {
            tree->allocator->free(node->keys);
            tree->allocator->free(node);
            btree_set_error(BTREE_ERROR_MEMORY_ALLOCATION);
            return NULL;
        }
        memset(node->children, 0, children_array_size);
    }
    
    /* 통계 업데이트 */
    tree->node_count++;
    tree->total_memory += sizeof(btree_node_t) + key_array_size;
    if (is_leaf) {
        tree->total_memory += tree->max_keys * tree->value_type.value_size;
    } else {
        tree->total_memory += (tree->max_keys + 1) * sizeof(btree_node_t*);
    }
    
    return node;
}

/**
 * @brief 노드 소멸
 */
void btree_node_destroy(btree_t *tree, btree_node_t *node) {
    if (!tree || !node || !tree->allocator) return;
    
    /* 참조 카운트 감소 */
    if (--node->ref_count > 0) return;
    
    /* 키 및 값 소멸자 호출 */
    if (tree->key_type.destroy) {
        tree->key_type.destroy(node->keys, node->num_keys);
    }
    if (node->values && tree->value_type.destroy) {
        tree->value_type.destroy(node->values, node->num_keys);
    }
    
    /* 자식 노드들 재귀적 소멸 */
    if (!node->is_leaf && node->children) {
        for (int i = 0; i <= node->num_keys; i++) {
            if (node->children[i]) {
                btree_node_destroy(tree, node->children[i]);
            }
        }
        tree->allocator->free(node->children);
    }
    
    /* 메모리 해제 */
    if (node->keys) tree->allocator->free(node->keys);
    if (node->values) tree->allocator->free(node->values);
    tree->allocator->free(node);
    
    /* 통계 업데이트 */
    tree->node_count--;
}

/**
 * @brief 노드에서 키 검색 (이진 검색)
 */
int btree_node_find_key(const btree_node_t *node, const void *key,
                       const btree_type_info_t *key_type) {
    if (!node || !key || !key_type || node->num_keys == 0) {
        return -1;
    }
    
    int left = 0;
    int right = node->num_keys - 1;
    
    while (left <= right) {
        int mid = (left + right) / 2;
        void *mid_key = btree_get_key_ptr(node, mid, key_type);
        
        int cmp = key_type->compare(key, mid_key);
        if (cmp == 0) {
            return mid;  /* 정확한 일치 */
        } else if (cmp < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    
    return -(left + 1);  /* 삽입 위치 반환 */
}

/**
 * @brief 노드에 키-값 쌍 삽입
 */
btree_result_t btree_node_insert_key(btree_node_t *node, int index,
                                    const void *key, const void *value,
                                    const btree_type_info_t *key_type,
                                    const btree_type_info_t *value_type) {
    if (!node || !key || !key_type || index < 0 || index > node->num_keys) {
        return BTREE_ERROR_NULL_POINTER;
    }
    
    if (node->num_keys >= node->capacity) {
        return BTREE_ERROR_INVALID_OPERATION;
    }
    
    /* 키들을 뒤로 이동 */
    if (index < node->num_keys) {
        size_t move_size = (node->num_keys - index) * key_type->key_size;
        void *src = btree_get_key_ptr(node, index, key_type);
        void *dst = btree_get_key_ptr(node, index + 1, key_type);
        
        if (key_type->move) {
            key_type->move(dst, src, node->num_keys - index);
        } else {
            memmove(dst, src, move_size);
        }
        
        /* 값들도 이동 (리프 노드의 경우) */
        if (node->is_leaf && node->values && value_type) {
            size_t value_move_size = (node->num_keys - index) * value_type->value_size;
            void *value_src = btree_get_value_ptr(node, index, value_type);
            void *value_dst = btree_get_value_ptr(node, index + 1, value_type);
            
            if (value_type->move) {
                value_type->move(value_dst, value_src, node->num_keys - index);
            } else {
                memmove(value_dst, value_src, value_move_size);
            }
        }
        
        /* 자식 포인터들 이동 (내부 노드의 경우) */
        if (!node->is_leaf && node->children) {
            memmove(&node->children[index + 2], &node->children[index + 1],
                   (node->num_keys - index) * sizeof(btree_node_t*));
        }
    }
    
    /* 새 키 복사 */
    void *key_slot = btree_get_key_ptr(node, index, key_type);
    if (key_type->copy) {
        key_type->copy(key_slot, key, 1);
    } else {
        memcpy(key_slot, key, key_type->key_size);
    }
    
    /* 새 값 복사 (표준 B-Tree에서는 내부 노드와 리프 노드 모두에서 값 저장) */
    if (node->values && value && value_type) {
        void *value_slot = btree_get_value_ptr(node, index, value_type);
        if (value_type->copy) {
            value_type->copy(value_slot, value, 1);
        } else {
            memcpy(value_slot, value, value_type->value_size);
        }
    }
    
    node->num_keys++;
    return BTREE_SUCCESS;
}

/**
 * @brief 노드에서 키 제거
 */
btree_result_t btree_node_remove_key(btree_node_t *node, int index,
                                    const btree_type_info_t *key_type,
                                    const btree_type_info_t *value_type) {
    if (!node || index < 0 || index >= node->num_keys) {
        return BTREE_ERROR_INVALID_OPERATION;
    }
    
    /* 키 소멸자 호출 */
    if (key_type->destroy) {
        void *key_slot = btree_get_key_ptr(node, index, key_type);
        key_type->destroy(key_slot, 1);
    }
    
    /* 값 소멸자 호출 (리프 노드의 경우) */
    if (node->is_leaf && node->values && value_type && value_type->destroy) {
        void *value_slot = btree_get_value_ptr(node, index, value_type);
        value_type->destroy(value_slot, 1);
    }
    
    /* 뒤의 키들을 앞으로 이동 */
    if (index < node->num_keys - 1) {
        size_t move_size = (node->num_keys - index - 1) * key_type->key_size;
        void *dst = btree_get_key_ptr(node, index, key_type);
        void *src = btree_get_key_ptr(node, index + 1, key_type);
        
        if (key_type->move) {
            key_type->move(dst, src, node->num_keys - index - 1);
        } else {
            memmove(dst, src, move_size);
        }
        
        /* 값들도 이동 (리프 노드의 경우) */
        if (node->is_leaf && node->values && value_type) {
            size_t value_move_size = (node->num_keys - index - 1) * value_type->value_size;
            void *value_dst = btree_get_value_ptr(node, index, value_type);
            void *value_src = btree_get_value_ptr(node, index + 1, value_type);
            
            if (value_type->move) {
                value_type->move(value_dst, value_src, node->num_keys - index - 1);
            } else {
                memmove(value_dst, value_src, value_move_size);
            }
        }
        
        /* 자식 포인터들 이동 (내부 노드의 경우) */
        if (!node->is_leaf && node->children) {
            memmove(&node->children[index + 1], &node->children[index + 2],
                   (node->num_keys - index - 1) * sizeof(btree_node_t*));
        }
    }
    
    node->num_keys--;
    return BTREE_SUCCESS;
}

/**
 * @brief B-Tree에서 검색
 */
void* btree_search(btree_t *tree, const void *key) {
    if (!tree || !key) {
        btree_set_error(BTREE_ERROR_NULL_POINTER);
        return NULL;
    }
    
    btree_node_t *node = tree->root;
    
    while (node) {
        int pos = btree_node_find_key(node, key, &tree->key_type);
        
        if (pos >= 0) {
            /* 키를 찾았음 - 표준 B-Tree에서는 내부 노드와 리프 노드 모두에서 값 반환 */
            return btree_get_value_ptr(node, pos, &tree->value_type);
        } else {
            if (node->is_leaf) {
                /* 리프 노드에서 못 찾았음 */
                btree_set_error(BTREE_ERROR_KEY_NOT_FOUND);
                return NULL;
            }
            
            /* 적절한 자식으로 이동 */
            int child_index = -(pos + 1);
            node = node->children[child_index];
        }
    }
    
    btree_set_error(BTREE_ERROR_KEY_NOT_FOUND);
    return NULL;
}

/**
 * @brief 노드 분할
 */
btree_result_t btree_split_node(btree_t *tree, btree_node_t *node,
                               btree_node_t **new_node, void **split_key) {
    if (!tree || !node || !new_node || !split_key) {
        return BTREE_ERROR_NULL_POINTER;
    }
    
    if (node->num_keys < tree->max_keys) {
        return BTREE_ERROR_INVALID_OPERATION;
    }
    
    /* 새 노드 생성 */
    *new_node = btree_node_create(tree, node->is_leaf);
    if (!*new_node) {
        return BTREE_ERROR_MEMORY_ALLOCATION;
    }
    
    int mid = tree->min_keys;  /* 분할 지점 */
    int keys_to_move = node->num_keys - mid - 1;
    
    /* 분할 키 할당 및 복사 */
    *split_key = tree->allocator->alloc(tree->key_type.key_size + tree->value_type.value_size);
    if (!*split_key) {
        btree_node_destroy(tree, *new_node);
        return BTREE_ERROR_MEMORY_ALLOCATION;
    }
    
    void *mid_key = btree_get_key_ptr(node, mid, &tree->key_type);
    if (tree->key_type.copy) {
        tree->key_type.copy(*split_key, mid_key, 1);
    } else {
        memcpy(*split_key, mid_key, tree->key_type.key_size);
    }
    
    /* 분할 키의 값도 함께 복사 (표준 B-Tree에서는 내부 노드에도 값 저장) */
    if (node->values) {
        void *mid_value = btree_get_value_ptr(node, mid, &tree->value_type);
        void *split_value = (char*)*split_key + tree->key_type.key_size;
        if (tree->value_type.copy) {
            tree->value_type.copy(split_value, mid_value, 1);
        } else {
            memcpy(split_value, mid_value, tree->value_type.value_size);
        }
    }
    
    /* 오른쪽 절반의 키들을 새 노드로 이동 */
    if (keys_to_move > 0) {
        void *src_keys = btree_get_key_ptr(node, mid + 1, &tree->key_type);
        size_t keys_size = keys_to_move * tree->key_type.key_size;
        
        if (tree->key_type.move) {
            tree->key_type.move((*new_node)->keys, src_keys, keys_to_move);
        } else {
            memcpy((*new_node)->keys, src_keys, keys_size);
        }
        
        /* 값들도 이동 (리프 노드의 경우) */
        if (node->is_leaf && node->values) {
            void *src_values = btree_get_value_ptr(node, mid + 1, &tree->value_type);
            size_t values_size = keys_to_move * tree->value_type.value_size;
            
            if (tree->value_type.move) {
                tree->value_type.move((*new_node)->values, src_values, keys_to_move);
            } else {
                memcpy((*new_node)->values, src_values, values_size);
            }
        }
        
        /* 자식 포인터들 이동 (내부 노드의 경우) */
        if (!node->is_leaf && node->children) {
            memcpy((*new_node)->children, &node->children[mid + 1],
                  (keys_to_move + 1) * sizeof(btree_node_t*));
            
            /* 부모 포인터 업데이트 */
            for (int i = 0; i <= keys_to_move; i++) {
                if ((*new_node)->children[i]) {
                    (*new_node)->children[i]->parent = *new_node;
                }
            }
        }
    }
    
    /* 새 노드의 키 개수 설정 */
    (*new_node)->num_keys = keys_to_move;
    (*new_node)->parent = node->parent;
    
    /* 기존 노드의 키 개수 조정 */
    node->num_keys = mid;
    
    /* B+Tree의 경우 리프 노드 연결 */
    if (node->is_leaf) {
        (*new_node)->next_leaf = node->next_leaf;
        if (node->next_leaf) {
            node->next_leaf->prev_leaf = *new_node;
        }
        node->next_leaf = *new_node;
        (*new_node)->prev_leaf = node;
    }
    
    return BTREE_SUCCESS;
}

/**
 * @brief B-Tree에 삽입 (재귀 헬퍼)
 */
static btree_result_t btree_insert_recursive(btree_t *tree, btree_node_t *node,
                                            const void *key, const void *value,
                                            void **split_key, btree_node_t **new_child) {
    int pos = btree_node_find_key(node, key, &tree->key_type);
    
    if (pos >= 0) {
        /* 중복 키 */
        if (!(tree->flags & BTREE_FLAG_ALLOW_DUPLICATES)) {
            return BTREE_ERROR_DUPLICATE_KEY;
        }
    }
    
    if (node->is_leaf) {
        /* 리프 노드에 직접 삽입 */
        int insert_pos = (pos >= 0) ? pos : -(pos + 1);
        
        if (node->num_keys >= tree->max_keys) {
            /* 노드가 가득 참 - 분할 필요 */
            btree_result_t result = btree_split_node(tree, node, new_child, split_key);
            if (result != BTREE_SUCCESS) return result;
            
            /* 분할 키와 비교하여 적절한 노드에 삽입 */
            int cmp = tree->key_type.compare(key, *split_key);
            btree_node_t *target_node = (cmp < 0) ? node : *new_child;
            
            if (cmp >= 0) {
                /* 새 노드에 삽입하는 경우 인덱스 조정 */
                insert_pos -= (tree->min_keys + 1);
            }
            
            return btree_node_insert_key(target_node, insert_pos, key, value,
                                       &tree->key_type, &tree->value_type);
        } else {
            /* 노드에 여유가 있음 */
            return btree_node_insert_key(node, insert_pos, key, value,
                                       &tree->key_type, &tree->value_type);
        }
    } else {
        /* 내부 노드 - 적절한 자식으로 재귀 */
        int child_index = (pos >= 0) ? pos + 1 : -(pos + 1);
        btree_node_t *child = node->children[child_index];
        
        void *child_split_key = NULL;
        btree_node_t *child_new_node = NULL;
        
        btree_result_t result = btree_insert_recursive(tree, child, key, value,
                                                      &child_split_key, &child_new_node);
        
        if (result != BTREE_SUCCESS) return result;
        
        if (child_new_node) {
            /* 자식이 분할되었음 - 분할 키를 이 노드에 삽입 */
            if (node->num_keys >= tree->max_keys) {
                /* 이 노드도 가득 참 - 분할 필요 */
                result = btree_split_node(tree, node, new_child, split_key);
                if (result != BTREE_SUCCESS) {
                    tree->allocator->free(child_split_key);
                    return result;
                }
                
                /* 분할 키와 비교하여 적절한 노드에 삽입 */
                int cmp = tree->key_type.compare(child_split_key, *split_key);
                btree_node_t *target_node = (cmp < 0) ? node : *new_child;
                
                int insert_pos = btree_node_find_key(target_node, child_split_key, &tree->key_type);
                insert_pos = (insert_pos >= 0) ? insert_pos : -(insert_pos + 1);
                
                /* 분할 키에서 키와 값 추출 */
                void *split_value = (char*)child_split_key + tree->key_type.key_size;
                result = btree_node_insert_key(target_node, insert_pos, child_split_key, split_value,
                                             &tree->key_type, &tree->value_type);
                if (result == BTREE_SUCCESS) {
                    /* 자식 포인터 설정 */
                    target_node->children[insert_pos + 1] = child_new_node;
                    child_new_node->parent = target_node;
                }
            } else {
                /* 노드에 여유가 있음 */
                int insert_pos = btree_node_find_key(node, child_split_key, &tree->key_type);
                insert_pos = (insert_pos >= 0) ? insert_pos : -(insert_pos + 1);
                
                /* 분할 키에서 키와 값 추출 */
                void *split_value = (char*)child_split_key + tree->key_type.key_size;
                result = btree_node_insert_key(node, insert_pos, child_split_key, split_value,
                                             &tree->key_type, &tree->value_type);
                if (result == BTREE_SUCCESS) {
                    /* 자식 포인터 설정 */
                    node->children[insert_pos + 1] = child_new_node;
                    child_new_node->parent = node;
                }
            }
            
            tree->allocator->free(child_split_key);
        }
        
        return result;
    }
}

/**
 * @brief B-Tree에 삽입
 */
btree_result_t btree_insert(btree_t *tree, const void *key, const void *value) {
    if (!tree || !key) {
        return btree_set_error(BTREE_ERROR_NULL_POINTER), BTREE_ERROR_NULL_POINTER;
    }
    
    if (!tree->root) {
        /* 첫 번째 노드 생성 */
        tree->root = btree_node_create(tree, true);
        if (!tree->root) {
            return BTREE_ERROR_MEMORY_ALLOCATION;
        }
        tree->height = 1;
        
        btree_result_t result = btree_node_insert_key(tree->root, 0, key, value,
                                                     &tree->key_type, &tree->value_type);
        if (result == BTREE_SUCCESS) {
            tree->key_count++;
        }
        return result;
    }
    
    void *split_key = NULL;
    btree_node_t *new_child = NULL;
    
    btree_result_t result = btree_insert_recursive(tree, tree->root, key, value,
                                                  &split_key, &new_child);
    
    if (result == BTREE_SUCCESS) {
        tree->key_count++;
        
        if (new_child) {
            /* 루트가 분할되었음 - 새 루트 생성 */
            btree_node_t *new_root = btree_node_create(tree, false);
            if (!new_root) {
                tree->allocator->free(split_key);
                return BTREE_ERROR_MEMORY_ALLOCATION;
            }
            
            /* 분할 키를 새 루트에 삽입 */
            void *split_value = (char*)split_key + tree->key_type.key_size;
            btree_node_insert_key(new_root, 0, split_key, split_value,
                                 &tree->key_type, &tree->value_type);
            
            /* 자식 포인터 설정 */
            new_root->children[0] = tree->root;
            new_root->children[1] = new_child;
            tree->root->parent = new_root;
            new_child->parent = new_root;
            
            tree->root = new_root;
            tree->height++;
            
            tree->allocator->free(split_key);
        }
    }
    
    return result;
}

/**
 * @brief B-Tree에서 삭제
 */
btree_result_t btree_delete(btree_t *tree, const void *key) {
    if (!tree || !key) {
        return btree_set_error(BTREE_ERROR_NULL_POINTER), BTREE_ERROR_NULL_POINTER;
    }
    
    if (!tree->root) {
        return btree_set_error(BTREE_ERROR_KEY_NOT_FOUND), BTREE_ERROR_KEY_NOT_FOUND;
    }
    
    /* TODO: 삭제 알고리즘 구현 */
    /* 복잡한 삭제 로직은 다음 구현에서... */
    
    return BTREE_ERROR_INVALID_OPERATION;
}

/**
 * @brief 키 포함 여부 확인
 */
bool btree_contains(btree_t *tree, const void *key) {
    return btree_search(tree, key) != NULL;
}

/**
 * @brief 트리 크기 반환
 */
size_t btree_size(const btree_t *tree) {
    return tree ? tree->key_count : 0;
}

/**
 * @brief 트리 높이 반환
 */
int btree_height(const btree_t *tree) {
    return tree ? tree->height : 0;
}

/**
 * @brief 트리가 비어있는지 확인
 */
bool btree_is_empty(const btree_t *tree) {
    return !tree || tree->root == NULL;
}

/**
 * @brief 트리 비우기
 */
void btree_clear(btree_t *tree) {
    if (!tree) return;
    
    if (tree->root) {
        btree_node_destroy(tree, tree->root);
        tree->root = NULL;
    }
    
    tree->key_count = 0;
    tree->height = 0;
}

/**
 * @brief 마지막 오류 반환
 */
btree_result_t btree_get_last_error(void) {
    return g_last_error;
}

/**
 * @brief 오류 문자열 반환
 */
const char* btree_error_string(btree_result_t error) {
    switch (error) {
        case BTREE_SUCCESS: return "Success";
        case BTREE_ERROR_NULL_POINTER: return "Null pointer";
        case BTREE_ERROR_INVALID_DEGREE: return "Invalid degree";
        case BTREE_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case BTREE_ERROR_KEY_NOT_FOUND: return "Key not found";
        case BTREE_ERROR_DUPLICATE_KEY: return "Duplicate key";
        case BTREE_ERROR_INVALID_OPERATION: return "Invalid operation";
        case BTREE_ERROR_TYPE_MISMATCH: return "Type mismatch";
        case BTREE_ERROR_INVALID_SIZE: return "Invalid size";
        case BTREE_ERROR_ALIGNMENT_ERROR: return "Alignment error";
        default: return "Unknown error";
    }
}

/**
 * @brief 라이브러리 버전 정보
 */
const char* btree_version_string(void) {
    return BTREE_VERSION_STRING;
}

int btree_version_major(void) {
    return BTREE_VERSION_MAJOR;
}

int btree_version_minor(void) {
    return BTREE_VERSION_MINOR;
}

int btree_version_patch(void) {
    return BTREE_VERSION_PATCH;
}

/**
 * @brief 라이브러리 초기화
 */
btree_result_t btree_library_init(void) {
    /* 전역 초기화 로직 */
    return BTREE_SUCCESS;
}

/**
 * @brief 라이브러리 정리
 */
void btree_library_cleanup(void) {
    /* 전역 정리 로직 */
}