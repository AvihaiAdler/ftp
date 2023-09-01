#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include "logger.h"
#include "thread_pool.h"

#define SIZE 128
#define TASKS_COUNT 100

struct args {
  int i;
  struct logger *logger;
};

void destroy_task(void *_task) {
  struct task *task = _task;
  if (task) free(task->args);
}

// simulates a long task
int handle_task(void *arg) {
  if (!arg) return 1;

  struct args *args = arg;

  struct timespec delay = {.tv_sec = 1, .tv_nsec = 0};
  struct timespec remains = {0};

  if (args->logger) { logger_log(args->logger, INFO, "thread %lu executing task %d\n", thrd_current(), args->i); }
  nanosleep(&delay, &remains);
  return 0;
}

int main(void) {
  struct logger *logger = logger_create("threads_pool_test.log", SIG_NONE);
  assert(logger);

  struct thread_pool *thread_pool = tp_create(20);
  assert(thread_pool);

  logger_log(logger, INFO, "test start\n");
  for (uint8_t i = 0; i < (uint8_t)TASKS_COUNT; i++) {
    struct args *args = malloc(sizeof *args);
    args->i = i;
    args->logger = logger;

    struct task task = {.args = args, .destroy_task = destroy_task, .handle_task = handle_task};
    assert(tp_add_task(thread_pool, &task));
  }

  struct timespec wait = {.tv_sec = 2, .tv_nsec = 0};
  struct timespec remains;
  nanosleep(&wait, &remains);

  tp_destroy(thread_pool);
  logger_log(logger, INFO, "test end\n");
  logger_destroy(logger);
}
