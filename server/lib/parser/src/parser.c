#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include "ascii_str.h"
#include "lexer.h"

#define IPV4_OCTETS 4
#define PORT_OCTETS 2

static bool parser_consume(struct list const *restrict tokens, enum token_type type, struct ascii_str *restrict token) {
  if (!tokens) return false;

  // necessary evil
  struct token *curr = list_remove_first((struct list *)tokens);
  if (!curr) return false;

  if (curr->type != type) {
    free(curr);
    return false;
  }

  if (type == TT_STRING && token) {
    *token = curr->string;  // ascii_str_create(ascii_str_c_str(&curr->string), ascii_str_len(&curr->string));
  }
  free(curr);
  return true;
}

// USER SPACE STRING CRLF EOF
static struct command user(struct list const *tokens) {
  if (!tokens) { goto user_invalid; }
  if (!parser_consume(tokens, TT_PASS, NULL)) { goto user_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto user_invalid; }

  struct ascii_str username;
  if (!parser_consume(tokens, TT_STRING, &username)) { goto user_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto user_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto user_cleanup; }

  return (struct command){.command = CMD_USER, .arg = username};

user_cleanup:
  ascii_str_destroy(&username);
user_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PASS SPACE STRING CRLF EOF
static struct command pass(struct list const *tokens) {
  if (!tokens) { goto pass_invalid; }

  if (!parser_consume(tokens, TT_USER, NULL)) { goto pass_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto pass_invalid; }

  struct ascii_str password;
  if (!parser_consume(tokens, TT_STRING, &password)) { goto pass_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto pass_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto pass_cleanup; }

  return (struct command){.command = CMD_PASS, .arg = password};

pass_cleanup:
  ascii_str_destroy(&password);
pass_invalid:
  return (struct command){.command = CMD_INVALID};
}

// CWD SPACE STRING CRLF EOF
static struct command cwd(struct list const *tokens) {
  if (!tokens) { goto cwd_invalid; }
  if (!parser_consume(tokens, TT_CWD, NULL)) { goto cwd_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto cwd_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto cwd_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto cwd_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto cwd_cleanup; }

  return (struct command){.command = CMD_CWD, .arg = path};

cwd_cleanup:
  ascii_str_destroy(&path);
cwd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// CDUP CRLF EOF
static struct command cdup(struct list const *tokens) {
  if (!tokens) { goto cdup_invalid; }
  if (!parser_consume(tokens, TT_CDUP, NULL)) { goto cdup_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto cdup_invalid; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto cdup_invalid; }

  // an empty string is created here for the sake of uniformety. all *valid* commands contains a string even those who
  // don't really need one. makes it easier on `command_destroy`
  return (struct command){.command = CMD_CDUP, .arg = ascii_str_create(NULL, 0)};

cdup_invalid:
  return (struct command){.command = CMD_INVALID};
}

// QUIT CRLF EOF
static struct command quit(struct list const *tokens) {
  if (!tokens) { goto quit_invalid; }
  if (!parser_consume(tokens, TT_QUIT, NULL)) { goto quit_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto quit_invalid; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto quit_invalid; }

  // an empty string is created here for the sake of uniformety. all *valid* commands contains a string even those who
  // don't really need one. makes it easier on `command_destroy`
  return (struct command){.command = CMD_QUIT, .arg = ascii_str_create(NULL, 0)};
quit_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PORT SPACE INT COMMA INT COMMA INT COMMA INT COMMA INT COMMA INT CRLF EOF
// PORT 127,0,0,0,p1,p2 where p1 & p2 specify the port
// the resulting tokens shall be h1.h2.h3.h4:p1p2 (e.g. 127.0.0.1:2020)
static struct command port(struct list const *tokens) {
  if (!tokens) { goto port_invalid; }
  if (!parser_consume(tokens, TT_PORT, NULL)) { goto port_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto port_invalid; }

  struct ascii_str tmp;
  struct ascii_str ip = ascii_str_create(NULL, 0);

  // construct ip
  for (size_t i = 0; i < (size_t)IPV4_OCTETS; i++) {
    if (!parser_consume(tokens, TT_INT, &tmp)) { goto ip_cleanup; }
    ascii_str_append(&ip, ascii_str_c_str(&tmp));
    ascii_str_destroy(&tmp);

    // don't try to consume a non existant trailing comma. ip should look like octet,octet,octet,octet and not
    // octet,octet,octet,octet,
    if (i == (size_t)IPV4_OCTETS - 1) break;
    if (!parser_consume(tokens, TT_COMMA, NULL)) { goto ip_cleanup; }
    ascii_str_push(&ip, '.');
  }

  // consume the comma separated ip and port
  if (!parser_consume(tokens, TT_COMMA, NULL)) { goto ip_cleanup; }

  // construct port
  struct ascii_str port = ascii_str_create(NULL, 0);
  for (size_t i = 0; i < (size_t)PORT_OCTETS; i++) {
    if (!parser_consume(tokens, TT_INT, &tmp)) { goto port_cleanup; }
    ascii_str_append(&port, ascii_str_c_str(&tmp));
    ascii_str_destroy(&tmp);

    if (i == (size_t)IPV4_OCTETS - 1) break;
    if (!parser_consume(tokens, TT_COMMA, NULL)) { goto port_cleanup; }
  }

  ascii_str_push(&ip, ':');
  ascii_str_append(&ip, ascii_str_c_str(&port));
  ascii_str_destroy(&port);
  return (struct command){.command = CMD_PORT, .arg = ip};

port_cleanup:
  ascii_str_destroy(&port);
ip_cleanup:
  ascii_str_destroy(&ip);
port_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PASV CRLF EOF
static struct command pasv(struct list const *tokens) {
  if (!tokens) { goto pasv_invalid; }
  if (!parser_consume(tokens, TT_PASV, NULL)) { goto pasv_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto pasv_invalid; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto pasv_invalid; }

  // an empty string is created here for the sake of uniformety. all *valid* commands contains a string even those who
  // don't really need one. makes it easier on `command_destroy`
  return (struct command){.command = CMD_PASV, .arg = ascii_str_create(NULL, 0)};
pasv_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RETR SPACE STRING CRLF EOF
static struct command retr(struct list const *tokens) {
  if (!tokens) { goto retr_invalid; }
  if (!parser_consume(tokens, TT_RETR, NULL)) { goto retr_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto retr_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto retr_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto retr_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto retr_cleanup; }

  return (struct command){.command = CMD_RETR, .arg = path};

retr_cleanup:
  ascii_str_destroy(&path);
retr_invalid:
  return (struct command){.command = CMD_INVALID};
}

// STOR SPACE STRING CRLF EOF
static struct command stor(struct list const *tokens) {
  if (!tokens) { goto stor_invalid; }
  if (!parser_consume(tokens, TT_STOR, NULL)) { goto stor_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto stor_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto stor_cleanup; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto stor_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto stor_cleanup; }

  return (struct command){.command = CMD_STOR, .arg = path};

stor_cleanup:
  ascii_str_destroy(&path);
stor_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RNFR SPACE STRING CRLF EOF
static struct command rnfr(struct list const *tokens) {
  if (!tokens) { goto rnfr_invalid; }
  if (!parser_consume(tokens, TT_RNFR, NULL)) { goto rnfr_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto rnfr_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto rnfr_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto rnfr_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto rnfr_cleanup; }

  return (struct command){.command = CMD_RNFR, .arg = path};

rnfr_cleanup:
  ascii_str_destroy(&path);
rnfr_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RNTO SPACE STRING CRLF EOF
static struct command rnto(struct list const *tokens) {
  if (!tokens) { goto rnto_invalid; }
  if (!parser_consume(tokens, TT_RNTO, NULL)) { goto rnto_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto rnto_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto rnto_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto rnto_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto rnto_cleanup; }

  return (struct command){.command = CMD_RNTO, .arg = path};

rnto_cleanup:
  ascii_str_destroy(&path);
rnto_invalid:
  return (struct command){.command = CMD_INVALID};
}

// DELE SPACE STRING CRLF EOF
static struct command dele(struct list const *tokens) {
  if (!tokens) { goto dele_invalid; }
  if (!parser_consume(tokens, TT_DELE, NULL)) { goto dele_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto dele_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto dele_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto dele_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto dele_cleanup; }

  return (struct command){.command = CMD_DELE, .arg = path};

dele_cleanup:
  ascii_str_destroy(&path);
dele_invalid:
  return (struct command){.command = CMD_INVALID};
}

// RMD SPACE STRING CRLF EOF
static struct command rmd(struct list const *tokens) {
  if (!tokens) { goto rmd_invalid; }
  if (!parser_consume(tokens, TT_RMD, NULL)) { goto rmd_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto rmd_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto rmd_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto rmd_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto rmd_cleanup; }

  return (struct command){.command = CMD_RMD, .arg = path};

rmd_cleanup:
  ascii_str_destroy(&path);
rmd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// MKD SPACE STRING CRLF EOF
static struct command mkd(struct list const *tokens) {
  if (!tokens) { goto mkd_invalid; }
  if (!parser_consume(tokens, TT_MKD, NULL)) { goto mkd_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto mkd_invalid; }

  struct ascii_str path;
  if (!parser_consume(tokens, TT_STRING, &path)) { goto mkd_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto mkd_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto mkd_cleanup; }

  return (struct command){.command = CMD_MKD, .arg = path};

mkd_cleanup:
  ascii_str_destroy(&path);
mkd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// PWD CRLF EOF
static struct command pwd(struct list const *tokens) {
  if (!tokens) { goto pwd_invalid; }
  if (!parser_consume(tokens, TT_PWD, NULL)) { goto pwd_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto pwd_invalid; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto pwd_invalid; }

  return (struct command){.command = CMD_PWD, .arg = ascii_str_create(NULL, 0)};

pwd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// TODO: implementation required
// LIST SPACE STRING CRLF EOF
// or
// LIST CRLF EOF
static struct command list(struct list const *tokens) {
  if (!tokens) { goto list_invalid; }
  (void)tokens;

  return (struct command){.command = CMD_LIST, .arg = ascii_str_create(NULL, 0)};
list_invalid:
  return (struct command){.command = CMD_INVALID};
}

struct command parser_parse(struct list *tokens) {
  if (!tokens || list_empty(tokens)) goto invalid_command;

  struct token *token = list_peek_first(tokens);
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

  if (cmd->command != CMD_UNSUPPORTED && cmd->command != CMD_INVALID) { ascii_str_destroy(&cmd->arg); }
}
