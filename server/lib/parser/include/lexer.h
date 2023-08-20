#pragma once

#include "ascii_str.h"
#include "vec.h"

enum token_type {
  TT_INT,
  TT_PUNC,
  TT_STRING,
  TT_SPACE,
  TT_CRLF,
  TT_EOF,
};

struct token {
  enum token_type type;
  union {
    long number;
    char punctuation;
    struct ascii_str string;
  };
};

struct vec lexer_lex(struct ascii_str *text);
