#pragma once

#include <threads.h>
#include "ascii_str.h"
#include "hash_table.h"
#include "logger.h"
#include "parser.h"
#include "sqlite3.h"

struct task_args {
  struct ascii_str id; /**< peer_ip*/

  mtx_t *sessions_mtx;
  struct hash_table *sessions;

  struct logger *logger;
  sqlite3 *db;

  struct command cmd;
};

/**
 * @brief
 * NOTE: takes ownership of `id` & `command`
 *
 * @param id
 * @param sessions_mtx
 * @param sessions
 * @param logger
 * @param db
 * @param cmd
 * @return struct task_args*
 */
struct task_args *task_args_create(struct ascii_str id,
                                   mtx_t *restrict sessions_mtx,
                                   struct hash_table *restrict sessions,
                                   struct logger *restrict logger,
                                   sqlite3 *restrict db,
                                   struct command cmd);

void task_args_destroy(struct task_args *task_args);
