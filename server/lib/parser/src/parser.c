#include "parser.h"
#include <string.h>
#include "ascii_str.h"
#include "lexer.h"

#define IPV4_OCTETS 4

// a better name if required since we don't really 'consume' here
static bool parser_consume(struct vec const *restrict tokens,
                           size_t pos,
                           enum token_type type,
                           struct ascii_str *restrict token) {
  if (!tokens) return false;

  // necessary evil
  struct token *curr = vec_at((struct vec *)tokens, pos);
  if (!curr) return false;

  if (curr->type != type) return false;

  if (type == TT_SPACE && token) {
    *token = ascii_str_create(ascii_str_c_str(&curr->string), ascii_str_len(&curr->string));
  }
  return true;
}

static void string_destroy(void *elem) {
  struct ascii_str *string = elem;
  ascii_str_destroy(string);
}

// USER SPACE STRING CRLF EOF
static struct command user(struct vec const *tokens) {
  if (!tokens) { goto user_invalid; }
  if (!parser_consume(tokens, 0, TT_PASS, NULL)) { goto user_invalid; }
  if (!parser_consume(tokens, 1, TT_SPACE, NULL)) { goto user_invalid; }

  struct ascii_str username;
  if (!parser_consume(tokens, 2, TT_STRING, &username)) { goto user_invalid; }
  if (!parser_consume(tokens, 3, TT_CRLF, NULL)) { goto user_cleanup; }
  if (!parser_consume(tokens, 4, TT_EOF, NULL)) { goto user_cleanup; }

  return (struct command){.command = CMD_USER, .arg = username};

user_cleanup:
  ascii_str_destroy(&username);
user_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PASS SPACE STRING CRLF EOF
static struct command pass(struct vec const *tokens) {
  if (!tokens) { goto pass_invalid; }

  if (!parser_consume(tokens, 0, TT_USER, NULL)) { goto pass_invalid; }
  if (!parser_consume(tokens, 1, TT_SPACE, NULL)) { goto pass_invalid; }

  struct ascii_str password;
  if (!parser_consume(tokens, 2, TT_STRING, &password)) { goto pass_invalid; }
  if (!parser_consume(tokens, 3, TT_CRLF, NULL)) { goto pass_cleanup; }
  if (!parser_consume(tokens, 4, TT_EOF, NULL)) { goto pass_cleanup; }

  return (struct command){.command = CMD_PASS, .arg = password};

pass_cleanup:
  ascii_str_destroy(&password);
pass_invalid:
  return (struct command){.command = CMD_INVALID};
}

// CWD SPACE STRING CRLF EOF
static struct command cwd(struct vec const *tokens) {
  if (!tokens) { goto cwd_invalid; }
  if (!parser_consume(tokens, 0, TT_CWD, NULL)) { goto cwd_invalid; }
  if (!parser_consume(tokens, 1, TT_SPACE, NULL)) { goto cwd_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, 2, TT_STRING, &path)) { goto cwd_invalid; }
  if (!parser_consume(tokens, 3, TT_CRLF, NULL)) { goto cwd_cleanup; }
  if (!parser_consume(tokens, 4, TT_EOF, NULL)) { goto cwd_cleanup; }

  return (struct command){.command = CMD_CWD, .arg = path};

cwd_cleanup:
  ascii_str_destroy(&path);
cwd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// CDUP CRLF EOF
static struct command cdup(struct vec const *tokens) {
  if (!tokens) { goto cdup_invalid; }
  if (!parser_consume(tokens, 0, TT_CDUP, NULL)) { goto cdup_invalid; }
  if (!parser_consume(tokens, 1, TT_CRLF, NULL)) { goto cdup_invalid; }
  if (!parser_consume(tokens, 2, TT_EOF, NULL)) { goto cdup_invalid; }

  // an empty string is created here for the sake of uniformety. all *valid* commands contains a string even those who
  // don't really need one. makes it easier on `command_destroy`
  return (struct command){.command = CMD_CDUP, .arg = ascii_str_create(NULL, 0)};

cdup_invalid:
  return (struct command){.command = CMD_INVALID};
}

// QUIT CRLF EOF
static struct command quit(struct vec const *tokens) {
  if (!tokens) { goto quit_invalid; }
  if (!parser_consume(tokens, 0, TT_QUIT, NULL)) { goto quit_invalid; }
  if (!parser_consume(tokens, 1, TT_CRLF, NULL)) { goto quit_invalid; }
  if (!parser_consume(tokens, 2, TT_EOF, NULL)) { goto quit_invalid; }

  // an empty string is created here for the sake of uniformety. all *valid* commands contains a string even those who
  // don't really need one. makes it easier on `command_destroy`
  return (struct command){.command = CMD_QUIT, .arg = ascii_str_create(NULL, 0)};
quit_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PORT SPACE INT COMMA INT COMMA INT COMMA INT CRLF EOF
static struct command port(struct vec const *tokens) {
  if (!tokens) { goto port_invalid; }
  if (!parser_consume(tokens, 0, TT_PORT, NULL)) { goto port_invalid; }
  if (!parser_consume(tokens, 1, TT_SPACE, NULL)) { goto port_invalid; }

  struct ascii_str tmp;
  struct ascii_str ip = ascii_str_create(NULL, 0);

  for (size_t i = 0; i < (size_t)IPV4_OCTETS; i++) {
    if (!parser_consume(tokens, i * 2 + 2, TT_INT, &tmp)) { goto port_cleanup; }
    ascii_str_append(&ip, ascii_str_c_str(&tmp));
    ascii_str_destroy(&tmp);

    // don't try to consume a non existant trailing comma. ip should look like octet,octet,octet,octet and not
    // octet,octet,octet,octet,
    if (i * 2 + 2 == (size_t)IPV4_OCTETS * 2) break;

    if (!parser_consume(tokens, i * 2 + 3, TT_COMMA, NULL)) { goto port_cleanup; }
    ascii_str_push(&ip, '.');
  }

  return (struct command){.command = CMD_PORT, .arg = ip};

port_cleanup:
  ascii_str_destroy(&ip);
port_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PASV CRLF EOF
static struct command pasv(struct vec const *tokens) {
  if (!tokens) { goto pasv_invalid; }
  if (!parser_consume(tokens, 0, TT_PASV, NULL)) { goto pasv_invalid; }
  if (!parser_consume(tokens, 1, TT_CRLF, NULL)) { goto pasv_invalid; }
  if (!parser_consume(tokens, 2, TT_EOF, NULL)) { goto pasv_invalid; }

  // an empty string is created here for the sake of uniformety. all *valid* commands contains a string even those who
  // don't really need one. makes it easier on `command_destroy`
  return (struct command){.command = CMD_PASV, .arg = ascii_str_create(NULL, 0)};
pasv_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RETR SPACE STRING CRLF EOF
static struct command retr(struct vec const *tokens) {
  if (!tokens) { goto retr_invalid; }
  if (!parser_consume(tokens, 0, TT_RETR, NULL)) { goto retr_invalid; }
  if (!parser_consume(tokens, 1, TT_SPACE, NULL)) { goto retr_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, 2, TT_STRING, &path)) { goto retr_invalid; }
  if (!parser_consume(tokens, 3, TT_CRLF, NULL)) { goto retr_cleanup; }
  if (!parser_consume(tokens, 4, TT_EOF, NULL)) { goto retr_cleanup; }

  struct vec args = vec_create(sizeof(struct ascii_str), string_destroy);
  (void)vec_push(&args, &path);
  return (struct command){.command = CMD_RETR, .arg = path};

retr_cleanup:
  ascii_str_destroy(&path);
retr_invalid:
  return (struct command){.command = CMD_INVALID};
}

// STOR SPACE STRING CRLF EOF
static struct command stor(struct vec const *tokens) {
stor_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RNFR SPACE STRING CRLF EOF
static struct command rnfr(struct vec const *tokens) {
rnfr_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RNTO SPACE STRING CRLF EOF
static struct command rnto(struct vec const *tokens) {
rnto_invalid:
  return (struct command){.command = CMD_INVALID};
}

// DELE SPACE STRING CRLF EOF
static struct command dele(struct vec const *tokens) {
dele_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RMD SPACE STRING CRLF EOF
static struct command rmd(struct vec const *tokens) {
rmd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// MKD SPACE STRING CRLF EOF
static struct command mkd(struct vec const *tokens) {
mkd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PWD CRLF EOF
static struct command pwd(struct vec const *tokens) {
pwd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// LIST SPACE STRING CRLF EOF
// or
// LIST CRLF EOF
static struct command list(struct vec const *tokens) {
list_invalid:
  return (struct command){.command = CMD_INVALID};
}

struct command parser_parse(struct vec *tokens) {
  if (!tokens || vec_empty(tokens)) goto invalid_command;

  struct token *token = vec_at(tokens, 0);
  switch (token->type) {
    case TT_USER:
      return user(tokens);
    case TT_PASS:
      return pass(tokens);
    case TT_CWD:
      return cwd(tokens);
    case TT_CDUP:
      return cdup(tokens);
    case TT_QUIT:
      return quit(tokens);
    case TT_PORT:
      return port(tokens);
    case TT_PASV:
      return pasv(tokens);
    case TT_RETR:
      return retr(tokens);
    case TT_STOR:
      return stor(tokens);
    case TT_RNFR:
      return rnfr(tokens);
    case TT_RNTO:
      return rnto(tokens);
    case TT_DELE:
      return dele(tokens);
    case TT_RMD:
      return rmd(tokens);
    case TT_MKD:
      return mkd(tokens);
    case TT_PWD:
      return pwd(tokens);
    case TT_LIST:
      return list(tokens);
    case TT_ACCT:  // start of fallthrough
    case TT_SMNT:
    case TT_REIN:
    case TT_TYPE:
    case TT_STRU:
    case TT_MODE:
    case TT_STOU:
    case TT_APPE:
    case TT_ALLO:
    case TT_REST:
    case TT_ABOR:
    case TT_NLST:
    case TT_SITE:
    case TT_SYST:
    case TT_STAT:
    case TT_HELP:
    case TT_NOOP:  // end of fallthrough
      return (struct command){.command = CMD_UNSUPPORTED};
    default:
      goto invalid_command;
  }

invalid_command:
  return (struct command){.command = CMD_INVALID};
}

void command_destroy(struct command *cmd) {
  if (!cmd) return;

  switch (cmd->command) {
    case CMD_USER:  // start of fallthrough
    case CMD_PASS:
    case CMD_CWD:
    case CMD_CDUP:
    case CMD_QUIT:
    case CMD_PORT:
    case CMD_PASV:
    case CMD_RETR:
    case CMD_STOR:
    case CMD_RNFR:
    case CMD_RNTO:
    case CMD_DELE:
    case CMD_RMD:
    case CMD_MKD:
    case CMD_PWD:
    case CMD_LIST:
      ascii_str_destroy(&cmd->arg);
      break;
    default:
      break;
  }
}
