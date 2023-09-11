#pragma once

#include "ascii_str.h"
#include "hash_table.h"
#include "list.h"

enum command_type {
  CMD_USER,
  CMD_PASS,
  CMD_CWD,
  CMD_CDUP,
  CMD_QUIT,
  CMD_PORT,
  CMD_PASV,
  CMD_RETR,
  CMD_STOR,
  CMD_RNFR,
  CMD_RNTO,
  CMD_DELE,
  CMD_RMD,
  CMD_MKD,
  CMD_PWD,
  CMD_LIST,
  CMD_ABOR,
  CMD_INVALID,
  CMD_UNSUPPORTED,
};

struct command {
  enum command_type command;
  struct ascii_str arg;
};

/**
 * @brief parses a list of tokens into a `struct command`. takes ownership over `tokens`
 *
 * @param[in] tokens
 * @return `struct command` the parsed command
 */
struct command parser_parse(struct list *tokens);

/**
 * @brief destroys a command
 *
 * @param[in] cmd the command to destroy
 */
void command_destroy(struct command *cmd);
