#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ascii_str.h"
#include "lexer.h"

static struct ascii_str long_to_str(long number) {
  struct ascii_str str = ascii_str_create(NULL, 0);

  enum ltos_size { LTOS_SIZE = 128 };
  char buf[LTOS_SIZE];

  if (snprintf(NULL, 0, "%ld", number) + 1 >= LTOS_SIZE) return str;
  if (sprintf(buf, "%ld", number) < 0) return str;

  ascii_str_append(&str, buf);
  return str;
}

static bool parser_consume(struct list *restrict tokens, enum token_type type, struct ascii_str *restrict token) {
  if (!tokens) return false;

  // necessary evil
  struct token *curr = list_remove_first((struct list *)tokens);
  if (!curr) return false;

  if (curr->type != type) {
    free(curr);
    return false;
  }

  if (token) {
    switch (type) {
      case TT_STRING:
        *token = curr->string;
        break;
      case TT_INT:
        *token = long_to_str(curr->number);
        break;
      default:
        break;
    }
  }

  free(curr);
  return true;
}

// USER SPACE STRING CRLF EOF
static struct command user(struct list *tokens) {
  if (!tokens) { goto user_invalid; }
  if (!parser_consume(tokens, TT_USER, NULL)) { goto user_invalid; }
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

static void token_destroy(struct token *token) {
  if (!token) return;

  if (token->type == TT_STRING) { ascii_str_destroy(&token->string); }
  free(token);
}

static struct ascii_str parse_password(struct list *restrict tokens) {
  struct ascii_str pass = ascii_str_create(NULL, 0);
  do {
    struct token *first = list_peek_first(tokens);
    if (!first) return pass;

    switch (first->type) {
      case TT_INT: {
        struct ascii_str long_as_str = long_to_str(first->number);
        ascii_str_append(&pass, ascii_str_c_str(&long_as_str));
        ascii_str_destroy(&long_as_str);
      } break;
      case TT_STRING:
        ascii_str_append(&pass, ascii_str_c_str(&first->string));
        break;
      default:
        goto parse_password_end;
    }

    token_destroy(list_remove_first(tokens));
  } while (true);

parse_password_end:
  return pass;
}

// PASS SPACE STRING CRLF EOF
static struct command pass(struct list *tokens) {
  if (!tokens) { goto pass_invalid; }
  if (!parser_consume(tokens, TT_PASS, NULL)) { goto pass_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto pass_invalid; }

  struct ascii_str password = parse_password(tokens);
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto pass_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto pass_cleanup; }

  return (struct command){.command = CMD_PASS, .arg = password};

pass_cleanup:
  ascii_str_destroy(&password);
pass_invalid:
  return (struct command){.command = CMD_INVALID};
}

// CWD SPACE STRING CRLF EOF
static struct command cwd(struct list *tokens) {
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
static struct command cdup(struct list *tokens) {
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
static struct command quit(struct list *tokens) {
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

static bool parse_ip(struct list *restrict tokens, struct ascii_str *restrict ip) {
  enum ipv4_octets { IPV4_OCTETS = 4 };

  struct ascii_str _ip = ascii_str_create(NULL, 0);

  for (size_t i = 0; i < (size_t)IPV4_OCTETS; i++) {
    struct ascii_str tmp;

    struct token *token = list_peek_first(tokens);
    if (!token || token->type != TT_INT) { goto parse_ip_cleanup; }

    // invalid octet
    if (token->number > UINT8_MAX) { goto parse_ip_cleanup; }

    if (!parser_consume(tokens, TT_INT, &tmp)) { goto parse_ip_cleanup; }
    ascii_str_append(&_ip, ascii_str_c_str(&tmp));
    ascii_str_destroy(&tmp);

    // don't try to consume a non existant trailing comma. ip should look like octet,octet,octet,octet and not
    // octet,octet,octet,octet,
    if (i == (size_t)IPV4_OCTETS - 1) break;
    if (!parser_consume(tokens, TT_COMMA, NULL)) { goto parse_ip_cleanup; }
    ascii_str_push(&_ip, '.');
  }

  *ip = _ip;
  return true;
parse_ip_cleanup:
  ascii_str_destroy(&_ip);
  return false;
}

static bool parse_port(struct list *restrict tokens, struct ascii_str *restrict port) {
  enum port_octets { PORT_OCTETS = 2 };

  struct ascii_str _port = ascii_str_create(NULL, 0);

  for (size_t i = 0; i < (size_t)PORT_OCTETS; i++) {
    struct ascii_str tmp;

    if (!parser_consume(tokens, TT_INT, &tmp)) { goto parse_port_cleanup; }
    ascii_str_append(&_port, ascii_str_c_str(&tmp));
    ascii_str_destroy(&tmp);

    if (i == (size_t)PORT_OCTETS - 1) break;
    if (!parser_consume(tokens, TT_COMMA, NULL)) { goto parse_port_cleanup; }
  }

  // invalid port
  // guarantee to contain a long as str
  if (strtol(ascii_str_c_str(&_port), NULL, 10) > UINT16_MAX) { goto parse_port_cleanup; }

  *port = _port;
  return true;
parse_port_cleanup:
  ascii_str_destroy(&_port);
  return false;
}

// PORT SPACE INT COMMA INT COMMA INT COMMA INT COMMA INT COMMA INT CRLF EOF
// PORT 127,0,0,0,p1,p2 where p1 & p2 specify the port
// the resulting tokens shall be h1.h2.h3.h4:p1p2 (e.g. 127.0.0.1:2020)
static struct command port(struct list *tokens) {
  if (!tokens) { goto port_invalid; }
  if (!parser_consume(tokens, TT_PORT, NULL)) { goto port_invalid; }
  if (!parser_consume(tokens, TT_SPACE, NULL)) { goto port_invalid; }

  // construct ip
  struct ascii_str ip;
  if (!parse_ip(tokens, &ip)) { goto port_invalid; }

  // consume the comma separated ip and port
  if (!parser_consume(tokens, TT_COMMA, NULL)) { goto port_invalid; }

  // construct port
  struct ascii_str port;
  if (!parse_port(tokens, &port)) { goto ip_cleanup; }

  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto port_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto port_cleanup; }

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
static struct command pasv(struct list *tokens) {
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
static struct command retr(struct list *tokens) {
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
static struct command stor(struct list *tokens) {
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
static struct command rnfr(struct list *tokens) {
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
static struct command rnto(struct list *tokens) {
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
static struct command dele(struct list *tokens) {
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
static struct command rmd(struct list *tokens) {
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
static struct command mkd(struct list *tokens) {
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
static struct command pwd(struct list *tokens) {
  if (!tokens) { goto pwd_invalid; }
  if (!parser_consume(tokens, TT_PWD, NULL)) { goto pwd_invalid; }
  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto pwd_invalid; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto pwd_invalid; }

  return (struct command){.command = CMD_PWD, .arg = ascii_str_create(NULL, 0)};

pwd_invalid:
  return (struct command){.command = CMD_INVALID};
}

// LIST SPACE STRING CRLF EOF
// or
// LIST CRLF EOF
static struct command list(struct list *tokens) {
  if (!tokens) { goto list_invalid; }
  if (!parser_consume(tokens, TT_LIST, NULL)) { goto list_invalid; }

  struct token *t = list_peek_first(tokens);
  if (!t) { goto list_invalid; }

  struct ascii_str path;
  if (t->type == TT_SPACE) {
    parser_consume(tokens, TT_SPACE, NULL);
    if (!parser_consume(tokens, TT_STRING, &path)) { goto list_invalid; }
  } else {
    path = ascii_str_create(NULL, 0);
  }

  if (!parser_consume(tokens, TT_CRLF, NULL)) { goto list_cleanup; }
  if (!parser_consume(tokens, TT_EOF, NULL)) { goto list_cleanup; }

  return (struct command){.command = CMD_LIST, .arg = path};
list_cleanup:
  ascii_str_destroy(&path);
list_invalid:
  return (struct command){.command = CMD_INVALID};
}

struct command parser_parse(struct list *tokens) {
  struct command cmd = {.command = CMD_INVALID};

  struct token *token = list_peek_first(tokens);
  if (!token) goto parser_cleanup;  // list is empty or NULL

  switch (token->type) {
    case TT_USER:
      cmd = user(tokens);
      break;
    case TT_PASS:
      cmd = pass(tokens);
      break;
    case TT_CWD:
      cmd = cwd(tokens);
      break;
    case TT_CDUP:
      cmd = cdup(tokens);
      break;
    case TT_QUIT:
      cmd = quit(tokens);
      break;
    case TT_PORT:
      cmd = port(tokens);
      break;
    case TT_PASV:
      cmd = pasv(tokens);
      break;
    case TT_RETR:
      cmd = retr(tokens);
      break;
    case TT_STOR:
      cmd = stor(tokens);
      break;
    case TT_RNFR:
      cmd = rnfr(tokens);
      break;
    case TT_RNTO:
      cmd = rnto(tokens);
      break;
    case TT_DELE:
      cmd = dele(tokens);
      break;
    case TT_RMD:
      cmd = rmd(tokens);
      break;
    case TT_MKD:
      cmd = mkd(tokens);
      break;
    case TT_PWD:
      cmd = pwd(tokens);
      break;
    case TT_LIST:
      cmd = list(tokens);
      break;
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
      cmd = (struct command){.command = CMD_UNSUPPORTED};
      break;
    default:
      break;
  }

parser_cleanup:
  list_destroy(tokens);
  return cmd;
}

void command_destroy(struct command *cmd) {
  if (!cmd) return;

  if (cmd->command != CMD_UNSUPPORTED && cmd->command != CMD_INVALID) { ascii_str_destroy(&cmd->arg); }
}
