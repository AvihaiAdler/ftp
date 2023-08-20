#include <assert.h>
#include "ascii_str.h"
#include "lexer.h"
#include "logger.h"

#define PRINT_TOKENS(logger, tokens_ptr)                                            \
  do {                                                                              \
    for (void *iter = vec_iter_begin(tokens_ptr); iter != vec_iter_end(tokens_ptr); \
         iter = vec_iter_next(tokens_ptr, iter)) {                                  \
      struct token *t = iter;                                                       \
      LOG(logger, INFO, "token type: %s", token_type_name(t->type));                \
    }                                                                               \
  } while (0);

static char const *token_type_name(enum token_type type) {
  switch (type) {
    case TT_CRLF:
      return "CRLF";
    case TT_EOF:
      return "EOF";
    case TT_INT:
      return "INT";
    case TT_COMMA:
      return "COMMA";
    case TT_PUNCT:
      return "PUNCTUATION";
    case TT_SPACE:
      return "SPACE";
    case TT_STRING:
      return "STRING";
    default:
      return "UNKNOWN";
  }
}

static void lexer_empty_string_test(struct logger *logger) {
  // given
  struct ascii_str text = ascii_str_create("", STR_C_STR);

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  assert(vec_size(&tokens) == 1);

  struct token *t = vec_at(&tokens, 0);
  assert(t->type == TT_EOF);

  LOG(logger, INFO, "%s", ascii_str_c_str(&text));
  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_all_chars_string_test(struct logger *logger, char const *_text, size_t expected_tokens) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  assert(vec_size(&tokens) == expected_tokens + 1);

  LOG(logger, INFO, "%s", ascii_str_c_str(&text));
  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_string_with_punctuations_test(struct logger *logger,
                                                char const *_text,
                                                size_t expected_tokens,
                                                size_t punct_count) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  assert(vec_size(&tokens) == expected_tokens + 1);

  size_t _punct_count = 0;
  for (void *iter = vec_iter_begin(&tokens); iter != vec_iter_end(&tokens); iter = vec_iter_next(&tokens, iter)) {
    struct token *t = iter;
    if (t->type == TT_PUNCT) _punct_count++;
  }

  assert(punct_count == _punct_count);

  LOG(logger, INFO, "%s", ascii_str_c_str(&text));
  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_string_crlf_test(struct logger *logger,
                                   char const *_text,
                                   size_t expected_tokens,
                                   size_t crlf_count) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  assert(vec_size(&tokens) == expected_tokens + 1);

  size_t _crlf_count = 0;
  for (void *iter = vec_iter_begin(&tokens); iter != vec_iter_end(&tokens); iter = vec_iter_next(&tokens, iter)) {
    struct token *t = iter;
    if (t->type == TT_CRLF) _crlf_count++;
  }

  assert(crlf_count == _crlf_count);

  LOG(logger, INFO, "%s", ascii_str_c_str(&text));
  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_string_with_digits_test(struct logger *logger,
                                          char *_text,
                                          size_t expected_tokens,
                                          size_t numbers_count) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  assert(vec_size(&tokens) == expected_tokens + 1);

  size_t _numbers_count = 0;
  for (void *iter = vec_iter_begin(&tokens); iter != vec_iter_end(&tokens); iter = vec_iter_next(&tokens, iter)) {
    struct token *t = iter;
    if (t->type == TT_INT) _numbers_count++;
  }

  assert(numbers_count == _numbers_count);

  LOG(logger, INFO, "%s", ascii_str_c_str(&text));
  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

int main(void) {
  struct logger *logger = logger_init(NULL);

  lexer_empty_string_test(logger);
  lexer_all_chars_string_test(logger, "The quick brown fox jumps over the lazy dog", 17);
  lexer_all_chars_string_test(logger, "  The quick brown fox \t jumps over the lazy dog     ", 19);
  lexer_all_chars_string_test(logger, "  \r\nThe quick brown fox \t \r\njumps over the lazy dog   \r\n  ", 23);
  lexer_all_chars_string_test(logger, " The\tquick brown\nfox jumps over\nthe lazy dog", 18);
  lexer_string_with_punctuations_test(logger, "The quick brown fox - jumps over the lazy dog.", 20, 2);
  lexer_string_with_punctuations_test(logger, "The quick, brown fox - jumps over, the lazy dog.", 22, 2);
  lexer_string_crlf_test(logger, "\r\nThe quick brown fox\r\njumps over the lazy dog\r\n", 19, 3);
  lexer_string_with_digits_test(logger, "The quick brown 1 jumps over the lazy 99", 17, 2);
  lexer_string_with_digits_test(logger, " 1 quick brown 99-jumps over the lazy 256.", 19, 3);
  lexer_string_with_digits_test(logger, "PORT 127,0,0,1", 9, 4);

  logger_destroy(logger);
}
