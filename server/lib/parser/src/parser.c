#include "parser.h"
#include <stdarg.h>
#include <string.h>
#include "ascii_str.h"
#include "lexer.h"

char const *const identifiers[] =
  {"user", "pass", "cwd", "cdup", "quit", "port", "pasv", "retr", "stor", "rnfr", "rnto", "dele", "rmd", "pwd", "list"};

static int cmpr(void const *_a, void const *_b) {
  // necessary evil
  struct ascii_str *a = (void *)_a;
  struct ascii_str *b = (void *)_b;

  return strcmp(ascii_str_c_str(a), ascii_str_c_str(b));
}

static void destroy(void *_key) {
  struct ascii_str *key = _key;
  ascii_str_destroy(key);
}

static size_t hash(void const *_key, size_t _key_size) {
  (void)_key_size;

  struct ascii_str *key = (void *)_key;  // necessary evil
  char const *c_str = ascii_str_c_str(key);

  // djd2 by Dan Bernstein
  size_t _hash = 5381;
  for (size_t i = 0; i < ascii_str_len(key); i++) {
    _hash = _hash * 33 + c_str[i];
  }
  return _hash;
}

struct hash_table identifiers_create(void) {
  struct hash_table ht = table_create(sizeof(struct ascii_str), 0, cmpr, hash, destroy, NULL);

  size_t identifiers_count = sizeof identifiers / sizeof *identifiers;
  for (size_t i = 0; i < identifiers_count; i++) {
    struct ascii_str identifier = ascii_str_create(identifiers[i], STR_C_STR);
    (void)table_put(&ht, &identifier, NULL, NULL);
  }

  if (table_size(&ht) != identifiers_count) goto identifiers_create_empty;

  return ht;

identifiers_create_empty:
  identifiers_destroy(&ht);
  return table_create(sizeof(struct ascii_str), 0, cmpr, NULL, destroy, NULL);
}

void identifiers_destroy(struct hash_table *identifiers) {
  if (!identifiers) return;

  table_destroy(identifiers);
}

/**
 * @brief matched the desired tokens `tokens_type` against the list of tokens. in order
 *
 * @param tokens the list of the tokens to parse
 * @param tokens_type list of desired tokens types
 * @return true
 * @return false
 */
static bool verify_expr(struct vec *tokens, struct vec *tokens_type) {
  if (vec_size(tokens) < vec_size(tokens_type)) return false;

  for (size_t i = 0; i < vec_size(tokens_type); i++) {
    struct token *token = vec_at(tokens, i);
    enum token_type *token_type = vec_at(tokens_type, i);

    if (token->type != *token_type) return false;
  }

  return true;
}

// TODO: parse the list of tokens to a command
static struct command parse(struct hash_table const *identifiers, struct vec *tokens, struct vec *tokens_type) {
  for (size_t i = 0; i < vec_size(tokens_type); i++) {}
}

struct command parser_parse(struct hash_table const *identifiers, struct vec *tokens, size_t tokens_count, ...) {
  if (!tokens || vec_empty(tokens)) goto invalid_command;
  if (!tokens_count) goto invalid_command;

  struct vec tokens_type = vec_create(sizeof(enum token_type), NULL);

  va_list tokens_type_list;
  va_start(tokens_type_list, tokens_count);
  for (size_t i = 0; i < tokens_count; i++) {
    enum token_type tt = va_arg(tokens_type_list, enum token_type);
    (void)vec_push(&tokens_type, &tt);
  }
  va_end(tokens_type_list);

  if (vec_size(&tokens_type) != tokens_count) {
    vec_destroy(&tokens_type);
    goto invalid_command;  // questionable. better than assuming never fails
  }

  if (!verify_expr(tokens, &tokens_type)) {
    vec_destroy(&tokens_type);
    goto invalid_command;
  }

  struct command cmd = parse(identifiers, tokens, &tokens_type);
  vec_destroy(&tokens_type);
  return cmd;

invalid_command:
  return (struct command){.command = CMD_INVALID};
}

void command_destroy(struct command *cmd) {
  if (!cmd) return;

  vec_destroy(&cmd->args);
}
