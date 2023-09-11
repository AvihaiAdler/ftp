#pragma once

#include "ascii_str.h"
#include "list.h"

#define TOKEN_MAPPING_SIZE 61

/* the mapping for all the commands were generated in such way to avoid collisions */
enum token_type {
  TT_INT = TOKEN_MAPPING_SIZE,
  TT_PUNCT,
  TT_COMMA, /* the only punctuation we care about is a ',' the rest are generalized*/
  TT_STRING,
  TT_SPACE,
  TT_CRLF,
  TT_EOF,
  TT_USER = 48,
  TT_PASS = 60,
  TT_ACCT = 29,
  TT_CWD = 8,
  TT_CDUP = 22,
  TT_SMNT = 50,
  TT_REIN = 23,
  TT_QUIT = 15,
  TT_PORT = 0,
  TT_PASV = 52,
  TT_TYPE = 39,
  TT_STRU = 16,
  TT_MODE = 21,
  TT_RETR = 45,
  TT_STOR = 4,
  TT_STOU = 57,
  TT_APPE = 28,
  TT_ALLO = 24,
  TT_REST = 33,
  TT_RNFR = 36,
  TT_RNTO = 56,
  TT_ABOR = 17,
  TT_DELE = 30,
  TT_RMD = 59,
  TT_MKD = 18,
  TT_PWD = 2,
  TT_LIST = 51,
  TT_NLST = 31,
  TT_SITE = 53,
  TT_SYST = 37,
  TT_STAT = 7,
  TT_HELP = 55,
  TT_NOOP = 47,
};

struct token {
  enum token_type type;
  union {
    long number;
    char punctuation;
    struct ascii_str string;
  };
};

struct list lexer_lex(struct ascii_str *text);
