#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ascii_str.h"

#define MIN_SSO_LEN (size_t)12 - 1

static void ascii_str_init_null_test(void) {
  // given
  // when
  struct ascii_str empty = ascii_str_from_str(NULL);

  // then
  assert(empty.is_sso);
  assert(ascii_str_len(&empty) == 0);

  // cleanup
  ascii_str_destroy(&empty);
}

static void ascii_str_init_empty_test(void) {
  // given
  // when
  struct ascii_str empty = ascii_str_from_str("");

  // then
  assert(empty.is_sso);
  assert(ascii_str_len(&empty) == 0);

  // cleanup
  ascii_str_destroy(&empty);
}

static void ascii_str_init_sso_test(char const *c_str) {
  // given
  assert(c_str);
  size_t len = strlen(c_str);
  assert(len <= MIN_SSO_LEN);

  // when
  struct ascii_str str = ascii_str_from_str(c_str);

  // then
  assert(str.is_sso);
  assert(ascii_str_len(&str) == len);
  assert(strcmp(ascii_str_c_str(&str), c_str) == 0);

  // cleanup
  ascii_str_destroy(&str);
}

static void ascii_str_init_non_sso_test(char const *c_str) {
  // given
  assert(c_str);
  size_t len = strlen(c_str);
  assert(len > MIN_SSO_LEN);

  // when
  struct ascii_str str = ascii_str_from_str(c_str);

  // then
  assert(!str.is_sso);
  assert(ascii_str_len(&str) == len);
  assert(strcmp(ascii_str_c_str(&str), c_str) == 0);

  // cleanup
  ascii_str_destroy(&str);
}

static void ascii_str_empty_test(void) {
  // given
  // when
  struct ascii_str empty = ascii_str_from_str("");

  // then
  assert(empty.is_sso);
  assert(ascii_str_len(&empty) == 0);
  assert(ascii_str_empty(&empty));

  // cleanup
  ascii_str_destroy(&empty);
}

static void ascii_str_non_empty_test(char const *c_str) {
  // given
  assert(c_str);
  size_t len = strlen(c_str);
  assert(len);

  // when
  struct ascii_str empty = ascii_str_from_str(c_str);

  // then
  assert(!ascii_str_empty(&empty));
  assert(ascii_str_len(&empty) == len);

  // cleanup
  ascii_str_destroy(&empty);
}

static void ascii_str_empty_after_clear_test(char const *c_str) {
  // given
  assert(c_str);
  size_t len = strlen(c_str);
  assert(len);
  struct ascii_str str = ascii_str_from_str(c_str);
  assert(!ascii_str_empty(&str));

  // when
  ascii_str_clear(&str);

  // then
  assert(ascii_str_empty(&str));

  // cleanup
  ascii_str_destroy(&str);
}

static void ascii_str_push_test(char const *c_str) {
  assert(c_str);
  size_t len = strlen(c_str);
  assert(len);

  // given
  struct ascii_str str = ascii_str_from_str(c_str);
  assert(!ascii_str_empty(&str));
  char c = 'a';

  // when
  ascii_str_push(&str, c);

  // then
  assert(!ascii_str_empty(&str));
  assert(ascii_str_len(&str) - 1 == len);
  assert(memcmp(ascii_str_c_str(&str), c_str, len) == 0);
  assert(ascii_str_pop(&str) == c);

  // cleanup
  ascii_str_destroy(&str);
}

static void ascii_str_push_force_resize_test(char const *c_str) {
  assert(c_str);
  size_t len = strlen(c_str);
  assert(len <= MIN_SSO_LEN);

  // given
  struct ascii_str str = ascii_str_from_str(c_str);

  // when
  for (int c = 'a'; c < 'z'; c++) {
    ascii_str_push(&str, c);
  }

  // then
  assert(!str.is_sso);  // max sso size on x64 is 23 chars
  assert(!ascii_str_empty(&str));
  assert(ascii_str_len(&str) == len + 'z' - 'a');

  // cleanup
  ascii_str_destroy(&str);
}

static void ascii_str_pop_test(char const *str) {
  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);
  size_t original_len = strlen(str);

  // when
  char popped = ascii_str_pop(&ascii_str);

  // then
  assert(popped == str[strlen(str) - 1]);
  assert(ascii_str_len(&ascii_str) == original_len - 1);

  // cleanup
  ascii_str_destroy(&ascii_str);
}

static void ascii_str_append_test(char const *str, char const *other) {
  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);
  char *combined = calloc(strlen(str) + strlen(other) + 1, 1);
  if (!combined) perror("");

  sprintf(combined, "%s%s", str, other);

  // when
  ascii_str_append(&ascii_str, other);

  // then
  assert(ascii_str_len(&ascii_str) == strlen(str) + strlen(other));
  assert(strcmp(ascii_str_c_str(&ascii_str), combined) == 0);

  // cleanup
  free(combined);
  ascii_str_destroy(&ascii_str);
}

static void ascii_str_erase_test(char const *str, size_t from, size_t count) {
  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);
  struct ascii_str erased = ascii_str_from_arr(str, from);
  if (strlen(str) > from + count) ascii_str_append(&erased, str + from + count);

  // when
  ascii_str_erase(&ascii_str, from, count);

  // then
  assert(strcmp(ascii_str_c_str(&ascii_str), ascii_str_c_str(&erased)) == 0);

  // cleanup
  ascii_str_destroy(&ascii_str);
  ascii_str_destroy(&erased);
}

static void ascii_str_insert_test(char const *str, size_t pos, char const *other) {
  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);
  struct ascii_str inserted = ascii_str_from_arr(str, pos);
  ascii_str_append(&inserted, other);
  ascii_str_append(&inserted, str + pos);

  // when
  ascii_str_insert(&ascii_str, pos, other);

  // then
  assert(ascii_str_len(&ascii_str) == ascii_str_len(&inserted));
  assert(strcmp(ascii_str_c_str(&ascii_str), ascii_str_c_str(&inserted)) == 0);

  // cleanup
  ascii_str_destroy(&ascii_str);
  ascii_str_destroy(&inserted);
}

static void ascii_str_contains_test(char const *str, char const *other) {
  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);

  // when
  bool contains_within_ascii_str = ascii_str_contains(&ascii_str, other);
  bool contains = strstr(str, other) != NULL;

  // then
  assert(contains_within_ascii_str == contains);

  // cleanup
  ascii_str_destroy(&ascii_str);
}

static void ascii_str_index_of_existing_char_test(char const *str, char const c) {
  assert(strchr(str, (int)c));

  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);
  size_t idx = (size_t)(strchr(str, (int)c) - str);

  // when
  size_t ret = ascii_str_index_of(&ascii_str, c);

  // then
  assert(ret == idx);

  // cleanup
  ascii_str_destroy(&ascii_str);
}

static void ascii_str_index_of_non_existing_char_test(char const *str, char const c) {
  assert(!strchr(str, (int)c));

  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);

  // when
  size_t ret = ascii_str_index_of(&ascii_str, c);

  // then
  assert(ret == DS_EINVAL);

  // cleanup
  ascii_str_destroy(&ascii_str);
}

static void ascii_str_substr_test(char const *str, size_t from, size_t count) {
  // given
  struct ascii_str ascii_str = ascii_str_from_str(str);

  char const *arr = from >= strlen(str) ? NULL : str + from;
  struct ascii_str slice = ascii_str_from_arr(arr, count);

  // when
  struct ascii_str substr = ascii_str_substr(&ascii_str, from, count);

  // then
  assert(strcmp(ascii_str_c_str(&substr), ascii_str_c_str(&slice)) == 0);

  // cleanup
  ascii_str_destroy(&ascii_str);
  ascii_str_destroy(&slice);
  ascii_str_destroy(&substr);
}

static void ascii_str_split_test(char const *str, char const *pattern) {
  // given
  size_t len = strlen(str);
  char *str_cpy = calloc(len + 1, 1);
  assert(str_cpy);
  memcpy(str_cpy, str, len);

  struct ascii_str ascii_str = ascii_str_from_str(str);

  // when
  struct vec *splitted = ascii_str_split(&ascii_str, pattern);

  // then
  assert(splitted);
  struct ascii_str *itr = vec_iter_begin(splitted);
  for (char *token = strtok(str_cpy, pattern); token;
       token = strtok(NULL, pattern), itr = vec_iter_next(splitted, itr)) {
    assert(strcmp(ascii_str_c_str(itr), token) == 0);
  }

  // cleanup
  vec_destroy(splitted);
  ascii_str_destroy(&ascii_str);
  free(str_cpy);
}

#define SHORT_STR "hello world"
#define LONG_STR "the quick brown fox jumps over the lazy dog"

int main(void) {
  ascii_str_init_null_test();

  ascii_str_init_empty_test();

  ascii_str_init_sso_test(SHORT_STR);

  ascii_str_init_non_sso_test(LONG_STR);

  ascii_str_empty_test();

  ascii_str_non_empty_test(SHORT_STR);
  ascii_str_non_empty_test(LONG_STR);

  ascii_str_empty_after_clear_test(SHORT_STR);
  ascii_str_empty_after_clear_test(LONG_STR);

  ascii_str_push_test(SHORT_STR);
  ascii_str_push_test(LONG_STR);

  ascii_str_push_force_resize_test(SHORT_STR);

  ascii_str_pop_test(SHORT_STR);
  ascii_str_pop_test(LONG_STR);

  ascii_str_append_test("hello", " world");
  ascii_str_append_test(SHORT_STR, LONG_STR);
  ascii_str_append_test(LONG_STR, LONG_STR);

  ascii_str_erase_test(SHORT_STR, 0, strlen(SHORT_STR));
  ascii_str_erase_test(SHORT_STR, 1, 2);
  ascii_str_erase_test(SHORT_STR, strlen(SHORT_STR) - 2, strlen(SHORT_STR));

  ascii_str_erase_test(LONG_STR, 0, strlen(LONG_STR));
  ascii_str_erase_test(LONG_STR, 1, 2);
  ascii_str_erase_test(LONG_STR, strlen(LONG_STR) - 2, strlen(LONG_STR));

  ascii_str_insert_test(SHORT_STR, 0, LONG_STR);
  ascii_str_insert_test(SHORT_STR, strlen(SHORT_STR) / 2, LONG_STR);
  ascii_str_insert_test(SHORT_STR, strlen(SHORT_STR), LONG_STR);

  ascii_str_contains_test(SHORT_STR, "ell");
  ascii_str_contains_test(SHORT_STR, "world_");
  ascii_str_contains_test(LONG_STR, "og");

  ascii_str_index_of_existing_char_test(SHORT_STR, 'h');
  ascii_str_index_of_existing_char_test(SHORT_STR, ' ');
  ascii_str_index_of_existing_char_test(LONG_STR, ' ');

  ascii_str_index_of_non_existing_char_test(SHORT_STR, '#');
  ascii_str_index_of_non_existing_char_test(LONG_STR, '!');

  ascii_str_substr_test(SHORT_STR, 0, strlen(SHORT_STR));
  ascii_str_substr_test(SHORT_STR, strlen(SHORT_STR), 1);
  ascii_str_substr_test(SHORT_STR, strlen(SHORT_STR) / 2, 4);

  ascii_str_substr_test(LONG_STR, 0, strlen(LONG_STR));
  ascii_str_substr_test(LONG_STR, strlen(LONG_STR), 1);
  ascii_str_substr_test(LONG_STR, strlen(LONG_STR) / 2, 4);

  ascii_str_split_test(SHORT_STR, " ");
  ascii_str_split_test(SHORT_STR, " ,!");

  ascii_str_split_test(LONG_STR, "o");
}
