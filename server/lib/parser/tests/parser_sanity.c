#include <stdbool.h>
#include <stdio.h>
#include "ascii_str.h"
#include "assert.h"
#include "lexer.h"
#include "parser.h"

#define LOG(stream, fmt, ...)                                                       \
  do {                                                                              \
    fprintf(stream, "%s %s:%d\n\t" fmt, __FILE__, __func__, __LINE__, __VA_ARGS__); \
  } while (0);

static char const *cmd_type_name(enum command_type cmd) {
  switch (cmd) {
    case CMD_USER:
      return "USER";
    case CMD_PASS:
      return "PASS";
    case CMD_CWD:
      return "CWD";
    case CMD_CDUP:
      return "CDUP";
    case CMD_QUIT:
      return "QUIT";
    case CMD_PORT:
      return "PORT";
    case CMD_PASV:
      return "PASV";
    case CMD_RETR:
      return "RETR";
    case CMD_STOR:
      return "STOR";
    case CMD_RNFR:
      return "RNFR";
    case CMD_RNTO:
      return "RNTO";
    case CMD_DELE:
      return "DELE";
    case CMD_RMD:
      return "RMD";
    case CMD_MKD:
      return "MKD";
    case CMD_PWD:
      return "PWD";
    case CMD_LIST:
      return "LIST";
    case CMD_INVALID:
      return "INVALID";
    case CMD_UNSUPPORTED:
      return "UNSUPPORTED";
    default:
      return "UNKNOWN";
  }
}

static void parse_valid_command_test(struct ascii_str *text) {
  assert(text);

  LOG(stderr, "attempt to parse: %s\n", ascii_str_c_str(text));

  // given
  struct list tokens = lexer_lex(text);

  // when
  struct command cmd = parser_parse(&tokens);

  LOG(stderr, "command: {%s, %s}\n", cmd_type_name(cmd.command), ascii_str_c_str(&cmd.arg));

  // then
  assert(cmd.command != CMD_INVALID);
  assert(cmd.command != CMD_UNSUPPORTED);

  // cleanup
  command_destroy(&cmd);
}

static void parse_invalid_command_test(struct ascii_str *text) {
  assert(text);

  LOG(stderr, "attempt to parse: %s\n", ascii_str_c_str(text));

  // given
  struct list tokens = lexer_lex(text);

  // when
  struct command cmd = parser_parse(&tokens);

  // then
  assert(cmd.command == CMD_INVALID);

  // cleanup
  command_destroy(&cmd);
}

static void parse_unsupported_command_test(struct ascii_str *text) {
  assert(text);

  LOG(stderr, "attempt to parse: %s\n", ascii_str_c_str(text));

  // given
  struct list tokens = lexer_lex(text);

  // when
  struct command cmd = parser_parse(&tokens);

  // then
  assert(cmd.command == CMD_UNSUPPORTED);

  // cleanup
  command_destroy(&cmd);
}

enum test {
  TEST_VALID,
  TEST_UNSUPPORTED,
  TEST_INVALID,
};

static void open_file_and_test(char const *file_name, enum test test_mode) {
  FILE *fp = fopen(file_name, "r");
  assert(fp);

  struct ascii_str text = ascii_str_create(NULL, 0);
  while (true) {
    int c = fgetc(fp);
    if (c == '\n' || c == EOF) {
      ascii_str_append(&text, "\r\n");

      switch (test_mode) {
        case TEST_VALID:
          parse_valid_command_test(&text);
          break;
        case TEST_UNSUPPORTED:
          parse_unsupported_command_test(&text);
          break;
        default:
          parse_invalid_command_test(&text);
          break;
      }

      ascii_str_clear(&text);

      if (c == EOF) break;

      continue;
    }

    ascii_str_push(&text, (char)c);
  }

  ascii_str_destroy(&text);
  fclose(fp);
}

int main(void) {
  open_file_and_test("commands/valid_commands", TEST_VALID);
  open_file_and_test("commands/unsupported_commands", TEST_UNSUPPORTED);
  open_file_and_test("commands/invalid_commands", TEST_INVALID);
}
