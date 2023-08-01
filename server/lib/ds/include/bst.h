#pragma once
#include <stdbool.h>
#include <stddef.h>

/**
 * @file bst.h
 * @brief the definition of a binary search tree
 */

struct bst;

/**
 * @brief creates a binary search tree object. returns a pointer to the tree on success or `NULL` on failure
 *
 * the function require a compare function between 2 keys which returns a positive int if `key > other`, 0 if `key ==
 * other` or a negative int if `key < other`.
 *
 * the function might recieve 2 optional destroy functions one for the key and one for the value. if one of them isn't
 * heap allocated - its corresponding destroy function should be `NULL`.
 *
 * @param[in] cmpr a compare function to compare between `key`s
 * @param[in] destroy_key a destory function to destroy a `key`
 * @param[in] destroy_value a destroy function to destroy a `value`
 *
 * @return `struct bst *` - a pointer to a binary search tree object on success, or `NULL` on failure
 */
struct bst *bst_create(int (*cmpr)(void *key, void *other),
                       void (*destroy_key)(void *key),
                       void (*destroy_value)(void *value));

/**
 * @brief associates a `value` with a `key`
 *
 * if `key` doesn't exists - creates a new node in the tree with `key` and `value`. if `key` exists - destructs the old
 * value and associate the `key` with the new `value`.
 *
 * @param[in] bst a binary search tree object
 * @param[in] key the `key`
 * @param[in] key_size the `key` size in bytes
 * @param[in] value the `value`
 * @param[in] value_size the `value` size in bytes
 *
 * @return `true` - on success
 * @return `false` - on failure
 */
bool bst_upsert(struct bst *bst, const void *const key, size_t key_size, const void *const value, size_t value_size);

/**
 * @brief finds the value associated with the key `key`. returns a pointer to the `value` associated with `key`
 *
 * this function should be used with care as any changes to `value` will change its data. moreover, certain operation on
 * the returned pointer might corrupt other data
 *
 * @param[in] bst a binary search tree object
 * @param[in] key the `key`
 *
 * @return `void *` - a pointer to the `value` associated with the key `key` on success, or `NULL` if no such `key`
 * exists
 */
void *bst_find(struct bst *bst, void *key);

/**
 * @brief deletes a key-value pair from the tree
 *
 * @param[in] bst a binary search tree object
 * @param[in] key the key
 *
 * @return `true` - on success
 * @return `false` - on failure
 */
bool bst_delete(struct bst *bst, void *key);

/**
 * @brief prints the tree using an 'in order' traversal
 *
 * @param[in] bst a binary search tree object
 * @param[in] print_key a function to print the key `key`
 * @param[in] print_value a fucntion o print the value `value`
 */
void bst_print(struct bst *bst, void (*print_key)(void *key), void (*print_value)(void *value));

/**
 * @brief destroys the tree
 *
 * @param[in] bst a binary search tree object
 */
void bst_destroy(struct bst *bst);
