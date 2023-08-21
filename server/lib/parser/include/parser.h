#pragma once

#include "hash_table.h"
#include "vec.h"

enum command_type {
  CMD_INVALID,
  CMD_UNSUPPORTED,
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
};

struct command {
  enum command_type command;
  struct vec args;  // vec<string>
};

/**
 * @brief generate a hash table of the supported identifiers
 * the hash table should never be written to
 *
 * @return struct hash_table
 * hash_table<string, NULL> - i.e. set<string>
 */
struct hash_table identifiers_create(void);

void identifiers_destroy(struct hash_table *identifiers);

struct command parser_parse(struct hash_table const *identifiers, struct vec *tokens, size_t tokens_count, ...);

void command_destroy(struct command *cmd);
