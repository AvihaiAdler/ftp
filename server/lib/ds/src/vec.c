#include "vec.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct vec {
  // both size and capacity can never exceed SIZE_MAX / 2
  size_t size;
  size_t capacity;
  size_t data_size;
  void *data;

  void (*destroy)(void *element);
};

struct vec *vec_init(size_t data_size, void (*destroy)(void *element)) {
  // limit check
  if (data_size == 0) return NULL;
  if ((SIZE_MAX >> 1) / data_size < VECT_INIT_CAPACITY) return NULL;

  struct vec *vec = calloc(1, sizeof *vec);
  if (!vec) return NULL;

  vec->data_size = data_size;
  vec->data = calloc(VECT_INIT_CAPACITY * vec->data_size, 1);
  if (!vec->data) {
    free(vec);
    return vec;
  }

  vec->capacity = VECT_INIT_CAPACITY;
  vec->size = 0;
  vec->destroy = destroy;
  return vec;
}

void vec_destroy(struct vec *vec) {
  if (!vec) return;
  if (vec->data) {
    for (size_t i = 0; i < vec->size * vec->data_size; i += vec->data_size) {
      if (vec->destroy) { vec->destroy((char *)vec->data + i); }
    }
    free(vec->data);
  }
  free(vec);
}

size_t vec_size(struct vec *vec) {
  return vec ? vec->size : 0;
}

size_t vec_struct_size(struct vec *vec) {
  return sizeof *vec;
}

size_t vec_capacity(struct vec *vec) {
  if (!vec) return 0;
  return vec->capacity;
}

void *vec_data(struct vec *vec) {
  return vec ? vec->data : NULL;
}

bool vec_empty(struct vec *vec) {
  return vec ? vec->size == 0 : true;
}

void *vec_at(struct vec *vec, size_t pos) {
  if (!vec) return NULL;
  if (!vec->data) return NULL;
  if (pos >= vec->size) return NULL;

  return (unsigned char *)vec->data + (pos * vec->data_size);
}

void *vec_find(struct vec *vec, const void *element, int (*cmpr)(const void *, const void *)) {
  if (!vec) return NULL;
  if (!vec->data) return NULL;
  if (!cmpr) return NULL;

  vec_sort(vec, cmpr);
  void *elem = bsearch(element, vec->data, vec->size, vec->data_size, cmpr);
  if (!elem) return NULL;
  return elem;
}

/* used internally to resize the vec by GROWTH_FACTOR */
static bool vec_resize_internal(struct vec *vec) {
  // limit check. vec:capacity cannot exceeds (SIZE_MAX >> 1)
  if ((SIZE_MAX >> 1) >> GROWTH_FACTOR < vec->capacity) return false;
  size_t new_capacity = vec->capacity << GROWTH_FACTOR;

  // limit check. vec::capacity * vec::data_size (the max number of
  // element the vec can hold) cannot exceeds (SIZE_MAX >> 1) / vec::data_size
  // (the number of elements (SIZE_MAX >> 1) can hold)
  if ((SIZE_MAX >> 1) / vec->data_size < new_capacity) return false;

  void *tmp = realloc(vec->data, new_capacity * vec->data_size);
  if (!tmp) return false;

  memset((char *)tmp + vec->size * vec->data_size, 0, new_capacity * vec->data_size - vec->size * vec->data_size);

  vec->capacity = new_capacity;
  vec->data = tmp;
  return true;
}

size_t vec_reserve(struct vec *vec, size_t size) {
  if (!vec) return 0;
  if (size > (SIZE_MAX >> 1)) return vec->capacity;
  if (size <= vec->capacity) return vec->capacity;

  void *tmp = realloc(vec->data, size * vec->data_size);
  if (!tmp) return vec->capacity;

  memset((char *)tmp + vec->size * vec->data_size, 0, size * vec->data_size - vec->size * vec->data_size);

  vec->capacity = size;
  vec->data = tmp;
  return vec->capacity;
}

size_t vec_resize(struct vec *vec, size_t size) {
  if (!vec) return 0;

  if (size >= vec->size && size <= vec->capacity) {
    memset((unsigned char *)vec->data + vec->size * vec->data_size,
           0,
           size * vec->data_size - vec->size * vec->data_size);
  } else if (size > vec->capacity) {
    size_t prev_capacity = vec_capacity(vec);
    size_t new_capacity = vec_reserve(vec, size);

    // vec_reserve failure
    if (prev_capacity == new_capacity) { return vec->size; }
  }

  vec->size = size;
  return vec->size;
}

bool vec_push(struct vec *vec, const void *element) {
  if (!vec) return false;
  if (!vec->data) return false;
  if (vec->size == vec->capacity) {
    if (!vec_resize_internal(vec)) return false;
  }

  memcpy((unsigned char *)vec->data + (vec->size * vec->data_size), element, vec->data_size);
  vec->size++;
  return true;
}

const void *vec_pop(struct vec *vec) {
  if (!vec) return NULL;
  if (!vec->data) return NULL;
  return (unsigned char *)vec->data + (--vec->size * vec->data_size);
}

void *vec_remove_at(struct vec *vec, size_t pos) {
  void *tmp = vec_at(vec, pos);
  if (!tmp) return NULL;

  void *old = calloc(1, vec->data_size);
  if (!old) return NULL;

  memcpy(old, tmp, vec->data_size);

  size_t factored_pos = pos * vec->data_size;
  memmove((unsigned char *)vec->data + factored_pos,
          (unsigned char *)vec->data + factored_pos + 1 * vec->data_size,
          (vec->size - pos - 1) * vec->data_size);
  vec->size--;
  return old;
}

void *vec_replace(struct vec *vec, const void *element, size_t pos) {
  void *tmp = vec_at(vec, pos);
  if (!tmp) return NULL;

  void *old = calloc(vec->data_size, 1);
  if (!old) return NULL;

  memcpy(old, tmp, vec->data_size);

  memcpy((unsigned char *)vec->data + (pos * vec->data_size), element, vec->data_size);
  return old;
}

size_t vec_shrink(struct vec *vec) {
  if (!vec) return 0;
  if (!vec->data) return 0;

  size_t new_capacity = vec->size;
  void *tmp = realloc(vec->data, new_capacity * vec->data_size);
  if (!tmp) return vec->capacity;

  vec->capacity = new_capacity;
  vec->data = tmp;
  return vec->capacity;
}

size_t vec_index_of(struct vec *vec, const void *element, int (*cmpr)(const void *, const void *)) {
  if (!vec) return -1;
  if (!vec->data) return -1;

  for (size_t i = 0; i < vec->size * vec->data_size; i += vec->data_size) {
    if (cmpr(element, (char *)vec->data + i) == 0) return i / vec->data_size;
  }

  return DS_EINVAL;
}

void vec_sort(struct vec *vec, int (*cmpr)(const void *, const void *)) {
  if (!vec) return;
  if (!vec->data) return;

  qsort(vec->data, vec->size, vec->data_size, cmpr);
}

void *vec_iter_begin(struct vec *vec) {
  if (!vec) return NULL;

  return (vec_empty(vec)) ? vec_iter_end(vec) : vec->data;
}

void *vec_iter_end(struct vec *vec) {
  if (!vec) return NULL;

  return (char *)vec->data + vec->size * vec->data_size;
}

void *vec_iter_next(struct vec *vec, void *iter) {
  if (!vec || !iter) return NULL;

  return (vec_empty(vec)) ? vec_iter_end(vec) : (char *)iter + vec->data_size;
}
