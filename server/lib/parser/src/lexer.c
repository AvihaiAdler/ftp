#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define SEED 1868

char const *keywords[TOKEN_MAPPING_SIZE] = {
  [TT_USER] = "user", [TT_PASS] = "pass", [TT_ACCT] = "acct", [TT_CWD] = "cwd",   [TT_CDUP] = "cdup",
  [TT_SMNT] = "smnt", [TT_REIN] = "rein", [TT_QUIT] = "quit", [TT_PORT] = "port", [TT_PASV] = "pasv",
  [TT_TYPE] = "type", [TT_STRU] = "stru", [TT_MODE] = "mode", [TT_RETR] = "retr", [TT_STOR] = "stor",
  [TT_STOU] = "stou", [TT_APPE] = "appe", [TT_ALLO] = "allo", [TT_REST] = "rest", [TT_RNFR] = "rnfr",
  [TT_RNTO] = "rnto", [TT_ABOR] = "abor", [TT_DELE] = "dele", [TT_RMD] = "rmd",   [TT_MKD] = "mkd",
  [TT_PWD] = "pwd",   [TT_LIST] = "list", [TT_NLST] = "nlst", [TT_SITE] = "site", [TT_SYST] = "syst",
  [TT_STAT] = "stat", [TT_HELP] = "help", [TT_NOOP] = "noop"};

/*
 * unlike ispunct '_' isn't considered a puncuation
 * returns true for !"#$%&'()*+,-./:;<=>?@[\]^`{|}~
 */
static bool is_punct(int c) {
  return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '^') || (c >= '{' && c <= '~') ||
         c == '`';
}

static void token_destroy(void *_token) {
  struct token *token = _token;

  if (token->type == TT_STRING) { ascii_str_destroy(&token->string); }
}

static struct token token_punc(char const *ptr) {
  enum token_type type = TT_PUNCT;
  if (*ptr == ',') type = TT_COMMA;

  return (struct token){.type = type, .punctuation = *ptr};
}

static struct token token_space(char const **ptr) {
  char const *curr = *ptr;
  if (*curr == '\r' && *(curr + 1) == '\n') {  // check for CRLF
    *ptr = curr + 1;
    return (struct token){.type = TT_CRLF};
  }

  for (; isspace(*curr) && *curr != '\r'; curr++) {
    continue;
  }

  *ptr = curr - 1;

  return (struct token){.type = TT_SPACE};
}

static struct token token_number(char const **ptr) {
  char const *start = *ptr;
  char *end;
  long number = strtol(start, &end, 10);

  /* compensating for strtol's behavior. walker is incremented by lexer_lex as it is*/
  *ptr = end - 1;

  return (struct token){.type = TT_INT, .number = number};
}

static struct ascii_str token_string(char const **ptr) {
  char const *start = *ptr;
  char const *tmp = *ptr;

  for (; *tmp && (isalpha(*tmp) || *tmp == '_'); tmp++) {
    continue;
  }

  *ptr = tmp - 1;
  struct ascii_str token_str = ascii_str_create(start, tmp - start);
  ascii_str_tolower(&token_str);

  return token_str;
}

static size_t hash(struct ascii_str *token_str, size_t seed, size_t limit) {
  if (ascii_str_len(token_str) < 2) return limit + 1;

  enum buff_size { local_buf_size = 4 };
  unsigned char buf[local_buf_size] = {0};

  // copy the first 2 chars and the last 2 chars of the string into buf
  memcpy(buf, ascii_str_c_str(token_str), 2);
  memcpy(buf + 2, ascii_str_c_str(token_str) + ascii_str_len(token_str) - 2, 2);

  size_t _hash = 0;
  for (size_t i = 0; i < sizeof buf; i++) {
    _hash = ((_hash << (i * 8)) | (size_t)buf[i]) * seed;
  }

  return _hash % limit;
}

struct vec lexer_lex(struct ascii_str *text) {
  struct vec tokens = vec_create(sizeof(struct token), token_destroy);

  if (ascii_str_empty(text)) {  // checks if !text as well
    vec_push(&tokens, &(struct token){.type = TT_EOF});
    return tokens;
  }

  for (char const *curr = ascii_str_c_str(text); *curr; curr++) {
    struct token token;
    if (is_punct(*curr)) {
      token = token_punc(curr);
    } else if (isspace(*curr)) {
      token = token_space(&curr);
    } else if (isdigit(*curr)) {
      token = token_number(&curr);
    } else {
      struct ascii_str token_str = token_string(&curr);
      size_t _hash = hash(&token_str, (size_t)SEED, (size_t)TOKEN_MAPPING_SIZE);

      if (_hash > (size_t)TOKEN_MAPPING_SIZE) {
        token = (struct token){.type = TT_STRING, .string = token_str};
      } else if (keywords[_hash] && strcmp(ascii_str_c_str(&token_str), keywords[_hash]) != 0) {
        token = (struct token){.type = TT_STRING, .string = token_str};
      } else {
        token = (struct token){.type = (enum token_type)_hash};
        ascii_str_destroy(&token_str);
      }
    }

    vec_push(&tokens, &token);
  }

  vec_push(&tokens, &(struct token){.type = TT_EOF});

  return tokens;
}
