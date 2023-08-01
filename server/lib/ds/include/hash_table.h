#pragma once

#include <stddef.h>

#include "vec.h"

/**
 * @file hash_table.h
 * @brief the definition of a hash table
 */

struct hash_table;

/**
 * @brief creates a hash table object. returns a pointer to a hash table object on success or `NULL` on failure
 *
 * the hash table will be created with `INIT_CAPACITY` capacity. the function expects a cmpr function to compare between
 * 2 keys. a cmpr function should return a negative int if `key < other`, 0 if both are equal or a positive int if `key
 * > other`
 * the function also expects 2 destroy functions (which may be `NULL`). if they're not `NULL`, `table_destory` will call
 * them for evey key-value pair in the table. one should only pass in a destroy function if `key` or `value` *contains*
 * a pointer to a heap allocated memory
 *
 * @param[in] cmpr a compare function to compare 2 keys
 * @param[in] destroy_key a destroy function. used to destroy any heap allocated data *within* `key`
 * @param[in] destroy_value a destroy function. used to destroy any heap allocated data *within* `value`
 *
 * @return `struct hash_table *` - a pointer to a hash table object on success or `NULL` on failure
 */
struct hash_table *table_init(int (*cmpr)(void const *, void const *),
                              void (*destroy_key)(void *),
                              void (*destroy_value)(void *));

/**
 * @brief destroys a hash table object
 *
 * @param[in] table a hash table object
 */
void table_destroy(struct hash_table *table);

/**
 * @brief checks whether or not a hash table contains any key-value pairs
 *
 * @param[in] table a hash table object
 *
 * @return `true` - if the hash table contains no key-value pairs
 * @return `false` - if the hash table contains at least one key-value pair
 */
bool table_empty(struct hash_table *table);

/**
 * @brief returns the number of key-value pairs in the table
 *
 * @param[in] table a hash table object
 *
 * @return `size_t` - the number of key-value pairs the table holds
 */
size_t table_size(struct hash_table *table);

/**
 * @brief returns the number of entries the table can hold
 *
 * @param[in] table a hash table object
 * @return `size_t` - the number of entries the table can currently hold
 */
size_t table_capacity(struct hash_table *table);

/**
 * @brief
 *
 * @param[in] table creates a copy of the data passed in - and store it in the table. returns the previous value for
 * that key (which has to be free'd) or NULL if there was no mapping for that key
 * @param[in] key the `key` associated with the data. `key` must not contain any padding bits. in case one not sure -
 * one shouldn't use structs as keys
 * @param[in] key_size the size of the `key` in bytes
 * @param[in] value the value
 * @param[in] value_size the size of the `value` in bytes
 *
 * @return `void *` - a pointer to a copy of the old `value` which has to be free'd intependently. if no such `key`
 * exists - `NULL` will be returned
 */
void *table_put(struct hash_table *table, void const *key, size_t key_size, void const *value, size_t value_size);

/**
 * @brief removes the mapping for a specific `key` if present. returns a copy of the previous `value`
 *
 * @param[in] table a hash table object
 * @param[in] key the desired `key` to remove
 * @param[in] key_size the size of the `key` in bytes
 *
 * @return `void *` - a pointer to a copy of the old `value` which has to be free'd intependently if no such `key`
 * exists - `NULL` will be returned
 */
void *table_remove(struct hash_table *table, void const *key, size_t key_size);

/* returns the mapping for a specific key if present or NULL if there was no
 * mapping for that key. the value must not be free'd. key_size - the size of
 * key in bytes */

/**
 * @brief returns the mapping for a specific `key`
 *
 * this function should be used with care as any changes to `value` will change its data. moreover, certain operation on
 * the returned pointer might corrupt other key-values' data.
 *
 * @param[in] table a hash table object
 * @param[in] key the desired `key`
 * @param[in] key_size the size of the `key` in bytes
 *
 * @return `void *` - a pointer the `value` associated with key `key`. the pointer must not be free'd
 */
void *table_get(struct hash_table *table, void const *key, size_t key_size);
