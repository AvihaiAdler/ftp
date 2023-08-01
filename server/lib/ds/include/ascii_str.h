#pragma once

/**
 * @file ascii_str.h
 * @brief an implementation of fbstring https://www.youtube.com/watch?v=kPR8h4-qZdk. all functions recieve a pointer as
 * their first argument to maintain consistency.
 */

#include <stddef.h>
#include "ascii_str_internal.h"
#include "defines.h"
#include "vec.h"

/**
 * @brief constructs and initializes a string object from a c-string
 *
 * @param[in] c_str a c-string or NULL
 *
 * @return the string object initialized with the content of c_str. if c_str is a NULL pointer the object will be
 * an empty string will be returned
 */
struct ascii_str ascii_str_from_str(char const *c_str);

/**
 * @brief constructs a string from an array
 *
 * @param[in] arr a char array
 * @param[in] len the length of the array in bytes
 * @return struct ascii_str - the string object initialized with the content of arr. if arr is a NULL pointer or len is
 * 0 an empty string will be returned
 */
struct ascii_str ascii_str_from_arr(char const *arr, size_t len);

/**
 * @brief destroys a string object
 *
 * @param[in] acsii_str the string object to be destroyed
 */
void ascii_str_destroy(struct ascii_str *ascii_str);

/**
 * @brief checks whether an ascii_str object is empty
 *
 * @param[in] ascii_str an scii_str object
 * @return true if the object is an empty string
 * @return false otherwise
 */
bool ascii_str_empty(struct ascii_str *ascii_str);

/**
 * @brief returns the length of an ascii_str
 *
 * @param[in] ascii_str an secii_str object
 * @return the number of bytes in the string. for ascii string that would be equal to the number of characters in the
 * string *excluding* the null terminator
 */
size_t ascii_str_len(struct ascii_str *ascii_str);

/**
 * @brief returns a const pointer to the underlying data
 *
 * @param[in] ascii_str an ascii_str object
 * @return char const * - a pointer to the underlying data of the string. the ptr behaves as a c-string and guarantee to
 * be null terminated
 */
char const *ascii_str_c_str(struct ascii_str *ascii_str);

/**
 * @brief clears the string (effectively any ascii_str object to hold an empty string)
 *
 * @param[in] ascii_str an ascii_str object
 */
void ascii_str_clear(struct ascii_str *ascii_str);

/**
 * @brief pushes one chr to the end of the string
 *
 * @param[in] ascii_str an ascii_str object
 * @param[in] c a char to append to the end of the string
 */
void ascii_str_push(struct ascii_str *ascii_str, char const c);

/**
 * @brief pops (removes) the last char from an ascii_str object
 *
 * @param[in] ascii_str an ascii_str object
 * @return the removed char.
 *
 * if ascii_str is a NULL pointer or the string is empty a (char)-1 will be returned
 */
char ascii_str_pop(struct ascii_str *ascii_str);

/**
 * @brief appends a c-string to the end of an ascii_str object
 *
 * @param[in] ascii_str an ascii_str object
 * @param[in] c_str a null terminated c-string. passing a NULL pointer will result in the function silently failing
 */
void ascii_str_append(struct ascii_str *restrict ascii_str, char const *restrict c_str);

/**
 * @brief erases @count bytes (chars) from the string from position @pos up to @pos + @count - 1 [pos, min(count,
 * ascii_str::len))
 *
 * if @count exceeds the string size - all the chars from @from_pos up to the string length will be removed
 *
 * @param[in] ascii_str an ascii_str object
 * @param[in] from_pos the position to start deleting the chars from
 * @param[in] count the number of chars to delete
 */
void ascii_str_erase(struct ascii_str *ascii_str, size_t from_pos, size_t count);

/**
 * @brief inserts a string an position @pos
 *
 * @param[in] ascii_str an ascii_str object
 * @param[in] pos the position to insert the string in. is @pos exceeds ascii_str's length - the result will be as if
 * ascii_str_append was called
 * @param[in] c_str a null terminated c-string to be inserted into the ascii_str object. passing a NULL pointer will
 * result in the function silently failing
 */
void ascii_str_insert(struct ascii_str *restrict ascii_str, size_t pos, char const *restrict c_str);

/**
 * @brief checks whether an ascii_str object contains the substring @needle
 *
 * @param[in] haystack an ascii_str object
 * @param[in] needle a null terminated c-string
 * @return bool - true if @haystack contains the substring, false otherwise
 */
bool ascii_str_contains(struct ascii_str *haystack, const char *needle);

/**
 * @brief returns the index of the *first occurence* of a char in the string. if no such char is found -
 * `GENERICS_EINVAL` will be returned
 *
 * @param[in] ascii_str an ascii_str object
 * @param[in] c the char to look for
 * @return size_t the index of said char or `GENERICS_EINVAL` if no such char is found. if @ascii_str is a `NULL`
 * pointer `GENERIC_EINVAL` will be returned
 */
size_t ascii_str_index_of(struct ascii_str *ascii_str, char const c);

/**
 * @brief returns a 'slice' from an ascii_str object. the slice will hold the contents of the substring starting at
 * position @from_pos up to @count - 1 [pos, pos + count).
 * if @from_pos isn't a valid index - an empty slice will be returned.
 * if @count exceeds the string length - the returned slice will be truncated
 * if @count is 0 - an empty slice will be returned
 *
 * @param[in] ascii_str an ascii_str object
 * @param[in] from_pos the start position the slice
 * @param[in] count the number of bytes (chars) the slice will hold
 * @return struct ascii_str - an ascii_str object holding the content of the specified 'slice'
 */
struct ascii_str ascii_str_substr(struct ascii_str *ascii_str, size_t from_pos, size_t count);

/**
 * @brief splits an ascii string on each char in @pattern. returns a struct vec containing each sub-string
 *
 * @param[in] ascii_str the string to stlit. this string may not change after this function call
 * @param[in] pattern a valid c-string
 * @return struct vec - a vec containing all substrings. this vec might be empty if @ascii_str doesn't contain any
 * chars which match the @pattern
 */
struct vec *ascii_str_split(struct ascii_str *restrict ascii_str, char const *restrict pattern);
