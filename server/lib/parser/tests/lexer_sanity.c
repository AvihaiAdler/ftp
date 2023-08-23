#include <assert.h>
#include <stdarg.h>
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
    case TT_USER:
      return "USER";
    case TT_PASS:
      return "PASS";
    case TT_ACCT:
      return "ACCT";
    case TT_CWD:
      return "CWD";
    case TT_CDUP:
      return "CDUP";
    case TT_SMNT:
      return "SMNT";
    case TT_REIN:
      return "REIN";
    case TT_QUIT:
      return "QUIT";
    case TT_PORT:
      return "PORT";
    case TT_PASV:
      return "PASV";
    case TT_TYPE:
      return "TYPE";
    case TT_STRU:
      return "STRU";
    case TT_MODE:
      return "MODE";
    case TT_RETR:
      return "RETR";
    case TT_STOR:
      return "STOR";
    case TT_STOU:
      return "STOU";
    case TT_APPE:
      return "APPE";
    case TT_ALLO:
      return "ALLO";
    case TT_REST:
      return "REST";
    case TT_RNFR:
      return "RNFR";
    case TT_RNTO:
      return "RNTO";
    case TT_ABOR:
      return "ABOR";
    case TT_DELE:
      return "DELE";
    case TT_RMD:
      return "RMD";
    case TT_MKD:
      return "MKD";
    case TT_PWD:
      return "PWD";
    case TT_LIST:
      return "LIST";
    case TT_NLST:
      return "NLST";
    case TT_SITE:
      return "SITE";
    case TT_SYST:
      return "SYST";
    case TT_STAT:
      return "STAT";
    case TT_HELP:
      return "HELP";
    case TT_NOOP:
      return "NOOP";
    default:
      return "UNKNOWN";
  }
}

static int cmpr_tokens(void const *_a, void const *_b) {
  struct token const *a = _a;
  struct token const *b = _b;

  return a->type == b->type ? 0 : 1;
}

static void lexer_empty_string_test(struct logger *logger) {
  // given
  struct ascii_str text = ascii_str_create("", STR_C_STR);
  LOG(logger, INFO, "%s", ascii_str_c_str(&text));

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  assert(vec_size(&tokens) == 1);

  struct token *t = vec_at(&tokens, 0);
  assert(t->type == TT_EOF);

  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_all_chars_string_test(struct logger *logger, char const *_text, size_t expected_tokens) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);
  LOG(logger, INFO, "%s", ascii_str_c_str(&text));

  // when
  struct vec tokens = lexer_lex(&text);
  PRINT_TOKENS(logger, &tokens);

  // then
  assert(vec_size(&tokens) == expected_tokens + 1);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_string_with_punctuations_test(struct logger *logger,
                                                char const *_text,
                                                size_t expected_tokens,
                                                size_t punct_count) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);
  LOG(logger, INFO, "%s", ascii_str_c_str(&text));

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
  LOG(logger, INFO, "%s", ascii_str_c_str(&text));

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
  LOG(logger, INFO, "%s", ascii_str_c_str(&text));

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

  PRINT_TOKENS(logger, &tokens);

  vec_destroy(&tokens);
  ascii_str_destroy(&text);
}

static void lexer_string_with_keywords_test(struct logger *logger, char *_text, size_t keywords_count, ...) {
  // given
  struct ascii_str text = ascii_str_create(_text, STR_C_STR);
  LOG(logger, INFO, "%s", ascii_str_c_str(&text));

  // when
  struct vec tokens = lexer_lex(&text);

  // then
  size_t expected_keywords = 0;

  va_list args;
  va_start(args, keywords_count);
  for (size_t i = 0; i < keywords_count; i++) {
    enum token_type tt = va_arg(args, enum token_type);
    assert(vec_find(&tokens, &(struct token){.type = tt}, cmpr_tokens));
    expected_keywords++;
  }
  va_end(args);

  assert(expected_keywords == keywords_count);

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
  lexer_string_with_keywords_test(logger, "PORT 127,0,0,1", 1, TT_PORT);
  lexer_string_with_keywords_test(logger, "USER some_user PASS 1234", 2, TT_USER, TT_PASS);
  lexer_string_with_keywords_test(logger, "The quick brown fox jumps over the lazy dog", 0);
  lexer_string_with_keywords_test(logger, "PASV", 1, TT_PASV);

  logger_destroy(logger);
}
