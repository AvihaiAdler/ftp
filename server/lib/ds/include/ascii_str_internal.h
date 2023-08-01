#pragma once

#include <stdbool.h>
#include <stddef.h>

struct ascii_str_long {
  size_t size;
  size_t capacity;
  char *data;
};

struct ascii_str_short {
  // room for 23 chars the 24th byte represent the number of available chars left
  char data[sizeof(struct ascii_str_long)];
};

struct ascii_str {
  union {
    struct ascii_str_long long_;
    struct ascii_str_short short_;
  } str_internal;
  bool is_sso;
};
