#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "vec.h"

bool equals(const void *a, const void *b) {
  int i_a = *(int *)a;
  int i_b = *(int *)b;
  return i_a == i_b;
}

int cmpr(const void *a, const void *b) {
  const int *i_a = a;
  const int *i_b = b;
  return (*i_a > *i_b) - (*i_a < *i_b);
}

struct vec *before(int *arr, size_t arr_size) {
  struct vec *vect = vec_init(sizeof *arr, NULL);
  for (size_t i = 0; i < arr_size; i++) {
    vec_push(vect, arr + i);
  }
  return vect;
}

void after(struct vec *vect) {
  vec_destroy(vect);
}

void vec_push_sanity_test(int num) {
  // given
  struct vec *vect = vec_init(sizeof num, NULL);
  assert(vec_empty(vect));

  // when
  bool res = vec_push(vect, &num);

  // then
  assert(res);
  int *stored = vec_at(vect, 0);
  assert(!vec_empty(vect));
  assert(stored);
  assert(*stored == num);

  // cleanup
  after(vect);
}

void vec_pop_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  assert(vec_size(vect) == arr_size);

  // when
  const int *poped = vec_pop(vect);

  // then
  assert(vec_size(vect) == arr_size - 1);
  assert(poped);
  assert(*poped == arr[arr_size - 1]);

  // cleanup
  after(vect);
}

void vec_at_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  // when
  int *first = vec_at(vect, 0);
  int *last = vec_at(vect, arr_size - 1);
  int *middle = vec_at(vect, arr_size / 2);

  // then
  assert(first);
  assert(last);
  assert(middle);
  assert(*first == *arr);
  assert(*last == arr[arr_size - 1]);
  assert(*middle == arr[arr_size / 2]);

  // cleanup
  after(vect);
}

void vec_find_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  // when
  int original = arr[arr_size / 2];
  int *elem = vec_find(vect, &original, cmpr);

  // then
  assert(elem);
  assert(*elem == original);

  // cleanup
  after(vect);
}

void vec_reserve_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  size_t init_capacity = vec_capacity(vect);
  assert(init_capacity > 0);

  // when
  size_t new_capacity = vec_reserve(vect, init_capacity * 4);

  // then
  assert(new_capacity > init_capacity);
  assert(new_capacity == init_capacity * 4);

  // cleanup
  after(vect);
}

void vec_remove_at_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);
  assert(vec_size(vect) == arr_size);

  // when
  int *removed = vec_remove_at(vect, arr_size / 2);

  // then
  assert(removed);
  assert(*removed == arr[arr_size / 2]);
  assert(vec_find(vect, &arr[arr_size / 2], cmpr) == NULL);

  // cleanup
  free(removed);
  after(vect);
}

void vec_replace_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);
  int num = -1;

  // when
  int *replaced = vec_replace(vect, &num, arr_size / 2);

  // then
  assert(replaced);
  assert(*replaced == arr[arr_size / 2]);
  assert(vec_find(vect, arr + arr_size / 2, cmpr) == NULL);

  // cleanup
  free(replaced);
  after(vect);
}

void vec_shrink_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);
  size_t init_capacity = vec_capacity(vect);
  size_t vect_init_size = vec_size(vect);

  // force a resize
  if (vect_init_size == init_capacity) {
    vec_push(vect, &arr[0]);
    init_capacity = vec_capacity(vect);
  }

  // when
  size_t new_capacity = vec_shrink(vect);
  size_t vect_size = vec_size(vect);

  // then
  assert(init_capacity > new_capacity);
  assert(new_capacity == vect_size);

  // cleanup
  after(vect);
}

void vec_index_of_sanity_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  // when
  size_t index = vec_index_of(vect, &arr[arr_size / 2], cmpr);

  // then
  assert(index != GENERICS_EINVAL);
  assert(index == arr_size / 2);

  // cleanup
  after(vect);
}

void vec_sort_santiy_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  // when
  vec_sort(vect, cmpr);

  // then
  size_t vect_size = vec_size(vect);
  for (size_t i = 0; i < vect_size - 1; i++) {
    assert(*(int *)vec_at(vect, i) < *(int *)vec_at(vect, i + 1));
  }

  // cleanup
  after(vect);
}

void vec_iter_empty_vec_test(void) {
  // given
  struct vec *vect = vec_init(sizeof(int), NULL);

  // when
  void *begin = vec_iter_begin(vect);
  void *end = vec_iter_end(vect);

  // then
  assert(begin == end);
  assert(vec_iter_next(vect, begin) == end);

  // cleanup
  after(vect);
}

void vec_iter_forward_iteration_test(int *arr, size_t arr_size) {
  // given
  struct vec *vect = before(arr, arr_size);

  // when
  void *begin = vec_iter_begin(vect);
  void *end = vec_iter_begin(vect);

  // then
  for (size_t i = 0; i < arr_size && begin != end; i++, begin = vec_iter_next(vect, begin)) {
    int *elem = begin;
    assert(*elem == arr[i]);
  }

  // cleanup
  after(vect);
}

#define SIZE 20

void populate_array(int *arr, size_t arr_size) {
  for (size_t i = arr_size; i > 0; i--) {
    arr[SIZE - i] = i;
  }
}

int main(void) {
  int arr[SIZE];
  size_t arr_size = sizeof arr / sizeof *arr;
  populate_array(arr, arr_size);

  vec_push_sanity_test(1);
  vec_pop_sanity_test(arr, arr_size);
  vec_at_sanity_test(arr, arr_size);
  vec_find_sanity_test(arr, arr_size);
  vec_reserve_sanity_test(arr, arr_size);
  vec_remove_at_sanity_test(arr, arr_size);
  vec_replace_sanity_test(arr, arr_size);
  vec_shrink_sanity_test(arr, arr_size);
  vec_index_of_sanity_test(arr, arr_size);
  vec_sort_santiy_test(arr, arr_size);

  vec_iter_empty_vec_test();
  vec_iter_forward_iteration_test(arr, arr_size);
  return 0;
}
