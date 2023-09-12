#pragma once

#include <threads.h>
#include "logger.h"
#include "parser.h"
#include "sqlite3.h"
#include "vec.h"

struct task_args;

struct task_args *task_args_create(mtx_t *restrict sessions_mtx,
                                   struct vec *restrict sessions,
                                   struct logger *restrict logger,
                                   sqlite3 *restrict db,
                                   struct command cmd);

void task_args_destroy(struct task_args *task_args);
