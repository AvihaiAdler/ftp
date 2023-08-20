#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>

static void token_destroy(void *_token) {
  struct token *token = _token;

  if (token->type == TT_STRING) { ascii_str_destroy(&token->string); }
}

static struct token token_punc(char const *ptr) {
  return (struct token){.type = TT_PUNC, .punctuation = *ptr};
}

static struct token token_space(char const **ptr) {
  char const *curr = *ptr;
  if (*curr == '\r' && *(curr + 1) == '\n') {  // check of CRLF
    *ptr = curr + 1;
    return (struct token){.type = TT_CRLF};
  }

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

static struct token token_string(char const **ptr) {
  char const *start = *ptr;
  char const *tmp = *ptr;

  for (; *tmp && isalpha(*tmp); tmp++) {
    continue;
  }

  *ptr = tmp - 1;
  return (struct token){.type = TT_STRING, .string = ascii_str_create(start, tmp - start)};
}

struct vec lexer_lex(struct ascii_str *text) {
  struct vec tokens = vec_create(sizeof(struct token), token_destroy);

  if (ascii_str_empty(text)) {  // checks if !text as well
    vec_push(&tokens, &(struct token){.type = TT_EOF});
    return tokens;
  }

  for (char const *curr = ascii_str_c_str(text); *curr; curr++) {
    struct token token;
    if (ispunct(*curr)) {
      token = token_punc(curr);
    } else if (isspace(*curr)) {
      token = token_space(&curr);
    } else if (isdigit(*curr)) {
      token = token_number(&curr);
    } else {
      token = token_string(&curr);
    }

    vec_push(&tokens, &token);
  }

  vec_push(&tokens, &(struct token){.type = TT_EOF});

  return tokens;
}
