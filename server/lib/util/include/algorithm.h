#pragma once
#include <stddef.h>

/**
 * @brief returns the first index of pattern within text
 *
 * @param[in] haystack - the array to search in
 * @param[in] haystack_len - number of characters in `text`
 * @param[in] needle - the array to search for
 * @param[in] needle_len - number of characters in `pattern`
 * @return `char*` - a pointer to the first occurrence of `pattern` within `text`. if `text` doesn't contain `pattern` -
 * returns `NULL`
 */
char *match(char *restrict haystack, size_t haystack_len, char const *restrict needle, size_t needle_len);
