#include "algorithm.h"
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct tuple {
  size_t a;
  size_t b;
};

enum context {
  REGULAR,
  INVERSE,
};

static struct tuple max_suffix(char const *restrict pattern, size_t len, enum context context) {
  size_t period = 1;
  size_t last_occur = 1;
  size_t pos = 0;
  size_t k = 1;

  while (last_occur + k < len) {
    char first = pattern[pos + k];
    char last = pattern[last_occur + k];

    if (last == first) {
      if (k == period) {
        last_occur += period;
        k = 1;
      } else {
        k++;
      }
      continue;
    }

    switch (context) {
      case INVERSE:
        if (last > first) {
          last_occur += k;
          k = 1;
          period = last_occur - pos;
        } else {
          pos = last_occur;
          last_occur = pos + 1;
          k = period = 1;
        }
        break;
      case REGULAR:
      default:  // fallthrough
        if (last < first) {
          last_occur += k;
          k = 1;
          period = last_occur - pos;
        } else {
          pos = last_occur;
          last_occur = pos + 1;
          k = period = 1;
        }
        break;
    }
  }

  return (struct tuple){.a = pos, .b = period};
}

/**
 * https://igm.univ-mlv.fr/~mac/Articles-PDF/CP-1991-jacm.pdf
 * http://www-igm.univ-mlv.fr/~lecroq/string/node26.html
 *
 * if its not broken - don't fix it...
 */
char *match(char *restrict haystack, size_t haystack_len, char const *restrict needle, size_t needle_len) {
  if (!haystack || !haystack_len) return NULL;
  if (!needle || !needle_len) return NULL;

  struct tuple regular = max_suffix(needle, needle_len, REGULAR);
  struct tuple inverse = max_suffix(needle, needle_len, INVERSE);

  size_t local_period = regular.a;
  size_t period = regular.b;

  if (regular.a < inverse.a) {
    local_period = inverse.a;
    period = inverse.b;
  }

  if (memcmp(needle, needle + period, local_period + 1) == 0) {
    size_t pos = 0;
    size_t memory = 0;
    while (pos + needle_len <= haystack_len) {
      size_t i = MAX(local_period, memory) + 1;
      while (i < needle_len && needle[i] == haystack[i + pos]) { i++; }

      if (i >= needle_len) {
        i = local_period;
        while (i > memory && needle[i] == haystack[pos + i]) { i--; }

        if (i <= memory) return haystack + pos;
        pos += period;
        memory = needle_len - period - 1;
      } else {
        pos += (i - local_period);
        memory = 0;
      }
    }
  } else {
    period = MAX(local_period + 1, needle_len - local_period - 1) + 1;
    size_t pos = 0;
    while (pos + needle_len <= haystack_len) {
      size_t i = local_period + 1;
      while (i < needle_len && needle[i] == haystack[i + pos]) { i++; }

      if (i >= needle_len) {
        i = local_period;
        while ((long)i >= 0 && needle[i] == haystack[pos + i]) { i--; }

        if ((long)i < 0) return haystack + pos;
        pos += period;
      } else {
        pos += (i - local_period);
      }
    }
  }
  return NULL;
}
