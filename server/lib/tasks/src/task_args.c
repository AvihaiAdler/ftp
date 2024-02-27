#include "task_args.h"
#include <stdlib.h>

struct task_args *task_args_create(struct ascii_str id,
                                   mtx_t *restrict sessions_mtx,
                                   struct hash_table *restrict sessions,
                                   struct logger *restrict logger,
                                   sqlite3 *restrict db,
                                   struct command cmd) {
  if (!sessions_mtx || !sessions || !logger || !db) return NULL;
  if (cmd.command == CMD_INVALID || cmd.command == CMD_UNSUPPORTED) return NULL;

  struct task_args *args = malloc(sizeof *args);
  if (!args) return NULL;

  *args = (struct task_args){.id = id,
                             .sessions_mtx = sessions_mtx,
                             .db = db,
                             .logger = logger,
                             .sessions = sessions,
                             .cmd = cmd};
  return args;
}

void task_args_destroy(struct task_args *task_args) {
  if (!task_args) return;

  command_destroy(&task_args->cmd);
  free(task_args);
}
