#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bst.h"

struct pair {
  int id;
  const char *str;
};

static int cmpr(void *key, void *other) {
  int *k = key;
  int *o = other;
  return (*k > *o) - (*k < *o);
}

static void print_key(void *key) {
  int *k = key;
  printf("%d: ", *k);
}

static void print_value(void *value) {
  const char *v = value;
  printf("%s\n", v);
}

static struct bst *before(struct pair *pairs, size_t size) {
  struct bst *bst = bst_create(cmpr, NULL, NULL);

  for (size_t i = 0; i < size; i++) {
    assert(bst_upsert(bst, &pairs[i].id, sizeof pairs[i].id, pairs[i].str, strlen(pairs[i].str) + 1));
  }

  return bst;
}

static void after(struct bst *bst) {
  bst_destroy(bst);
}

static void bst_insert_sanity(struct pair *pairs, size_t size) {
  // given
  // when
  struct bst *bst = before(pairs, size);

  // then
  assert(bst);
  bst_print(bst, print_key, print_value);

  // cleanup
  after(bst);
}

static void bst_find_sanity(struct pair *pairs, size_t size) {
  // given
  struct bst *bst = before(pairs, size);
  // and
  size_t i = 4;

  assert(bst);
  bst_print(bst, print_key, print_value);

  // when
  char *val = bst_find(bst, &pairs[i].id);
  // then
  assert(strcmp(val, pairs[i].str) == 0);

  // cleanup
  after(bst);
}

static void bst_replace_sanity(struct pair *pairs, size_t size) {
  // given
  struct bst *bst = before(pairs, size);
  // and
  size_t i = 4;
  const char *new_str = "hello, world";

  // validation
  assert(bst);

  bst_print(bst, print_key, print_value);
  char *val = bst_find(bst, &pairs[i].id);

  assert(strcmp(val, pairs[i].str) == 0);

  // when
  bool ret = bst_upsert(bst, &pairs[i].id, sizeof pairs[i].id, new_str, strlen(new_str) + 1);
  assert(ret);

  // and
  const char *value = bst_find(bst, &pairs[i].id);
  bst_print(bst, print_key, print_value);

  // then
  assert(strcmp(value, new_str) == 0);

  // cleanup
  after(bst);
}

static void bst_delete_sanity(struct pair *pairs, size_t size) {
  // given
  struct bst *bst = before(pairs, size);
  // and
  size_t i = 4;

  assert(bst);
  bst_print(bst, print_key, print_value);

  // when
  bool ret = bst_delete(bst, &pairs[i].id);

  // then
  assert(ret);

  char *val = bst_find(bst, &pairs[i].id);
  assert(!val);
  bst_print(bst, print_key, print_value);

  // cleanup
  after(bst);
}

int main(void) {
  struct pair pairs[] = {{.id = 5, .str = "five"},
                         {.id = 2, .str = "two"},
                         {.id = 8, .str = "eight"},
                         {.id = 4, .str = "four"},
                         {.id = 3, .str = "three"},
                         {.id = 7, .str = "seven"},
                         {.id = 1, .str = "one"}};
  size_t size = sizeof pairs / sizeof *pairs;

  bst_insert_sanity(pairs, size);
  printf("--------------------\n");
  bst_find_sanity(pairs, size);
  printf("--------------------\n");
  bst_replace_sanity(pairs, size);
  printf("--------------------\n");
  bst_delete_sanity(pairs, size);
}
