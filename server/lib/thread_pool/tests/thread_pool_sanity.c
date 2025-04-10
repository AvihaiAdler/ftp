#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include "ascii_str.h"
#include "logger.h"
#include "thread_pool.h"

struct task_args_simple {
  struct logger *logger;
  struct ascii_str *string;
};

struct task_args_long {
  unsigned sec;
  struct logger *logger;
  struct ascii_str string;
};

static int generate_random(int min, int max) {
  int range = max - min;
  double rand_val = rand() / (1.0 + RAND_MAX);
  return (int)(rand_val * range + min);
}

static struct ascii_str rand_string(size_t len) {
  char const *alphabet = "abcdefghijklmnopqrstuvwxyz";
  size_t alphabet_len = strlen(alphabet);

  struct ascii_str str = ascii_str_create(NULL, 0);
  for (size_t i = 0; i < len; i++) {
    int rand_idx = generate_random(0, alphabet_len);
    ascii_str_push(&str, alphabet[rand_idx]);
  }

  return str;
}

static void string_destroy(void *_str) {
  struct ascii_str *str = _str;
  ascii_str_destroy(str);
}

static struct thread_pool *before(int thread_count) {
  struct thread_pool *tp = tp_create(thread_count);
  return tp;
}

static void after(struct thread_pool *tp) {
  tp_destroy(tp);
}

static void simple_task_handler(void *_args) {
  struct task_args_simple *args = _args;
  LOG(args->logger, INFO, "\n\tworker %ld: %s\n", thrd_current(), ascii_str_c_str(args->string));
}

static void simple_task_destroyer(void *_task) {
  struct task *task = _task;
  struct task_args_simple *args = task->args;

  ascii_str_destroy(args->string);
}

static void simple_task_destroyer_heap(void *_task) {
  struct task *task = _task;
  struct task_args_simple *args = task->args;

  free(args);
}

static void long_task_handler(void *_args) {
  struct task_args_long *args = _args;
  LOG(args->logger, INFO, "\n\tworker %ld starts a computional heavy task\n", thrd_current());

  struct timespec reminaing = {0};
  nanosleep(&(struct timespec){.tv_sec = args->sec}, &reminaing);

  LOG(args->logger, INFO, "\n\tworker %ld return the result: %s\n", thrd_current(), ascii_str_c_str(&args->string));
}

static void long_task_destroyer(void *_task) {
  struct task *task = _task;
  struct task_args_long *args = task->args;

  ascii_str_destroy(&args->string);
}

static void tp_create_invalid_test(struct logger *restrict logger) {
  LOG(logger, INFO, "\n\ttesting %d threads\n", 0);
  // given
  // when
  struct thread_pool *tp = before(0);

  // then
  assert(!tp);
}

static void tp_add_task_test(struct logger *restrict logger, int thread_count) {
  LOG(logger, INFO, "\n\ttesting %d threads\n", thread_count);

  // given
  struct thread_pool *tp = before(thread_count);
  assert(tp);

  struct ascii_str str = ascii_str_create("hello, world!", STR_C_STR);
  struct task_args_simple args = {.logger = logger, .string = &str};

  // when
  bool ret = tp_add_task(
    tp,
    &(struct task){.args = &args, .destroy_task = simple_task_destroyer, .handle_task = simple_task_handler});

  // then
  assert(ret);

  struct timespec remaining = {0};
  nanosleep(&(struct timespec){.tv_sec = 2}, &remaining);

  // cleanup
  after(tp);
}

static void tp_terminate_early_test(struct logger *restrict logger, int threads_count, size_t tasks_count) {
  LOG(logger, INFO, "\n\ttesting %zu tasks with %d threads\n", tasks_count, threads_count);

  // given
  struct thread_pool *tp = before(threads_count);
  assert(tp);
  struct vec strings = vec_create(sizeof(struct ascii_str), string_destroy);

  // when
  for (size_t i = 0; i < tasks_count; i++) {
    struct ascii_str str = rand_string(100);
    assert(vec_push(&strings, &str));
  }

  for (size_t i = 0; i < tasks_count; i++) {
    struct task_args_simple *args = malloc(sizeof *args);
    assert(args);
    *args = (struct task_args_simple){.logger = logger, .string = vec_at(&strings, i)};

    bool ret = tp_add_task(tp,
                           &(struct task){.args = args,
                                          .destroy_task = simple_task_destroyer_heap,
                                          .handle_task = simple_task_handler,
                                          .id = i});

    // then
    assert(ret);
  }

  LOG(logger, INFO, "\n\tworker %ld (main) terminating early\n", thrd_current());

  // cleanup
  after(tp);
  vec_destroy(&strings);
}

static void tp_add_multiple_tasks_test(struct logger *restrict logger, size_t count, int threads_count) {
  LOG(logger, INFO, "\n\ttesting %zu tasks with %d threads\n", count, threads_count);

  // given
  struct thread_pool *tp = before(threads_count);
  assert(tp);
  struct vec strings = vec_create(sizeof(struct ascii_str), string_destroy);

  // when
  for (size_t i = 0; i < count; i++) {
    struct ascii_str str = rand_string(100);
    assert(vec_push(&strings, &str));
  }

  for (size_t i = 0; i < count; i++) {
    struct task_args_simple *args = malloc(sizeof *args);
    assert(args);
    *args = (struct task_args_simple){.logger = logger, .string = vec_at(&strings, i)};

    bool ret = tp_add_task(tp,
                           &(struct task){.args = args,
                                          .destroy_task = simple_task_destroyer_heap,
                                          .handle_task = simple_task_handler,
                                          .id = i});

    // then
    assert(ret);
  }

  LOG(logger, INFO, "\n\tworker %ld (main) entering sleep\n", thrd_current());
  // might terminate early depends on `count`. to checks early termination pass a high `count`
  struct timespec remaining = {0};
  nanosleep(&(struct timespec){.tv_sec = 5}, &remaining);

  LOG(logger, INFO, "\n\tworker %ld (main) woken up destroying the pool\n", thrd_current());

  // cleanup
  after(tp);
  vec_destroy(&strings);
}

static void tp_add_task_and_abort_test(struct logger *restrict logger, unsigned worker_delay, unsigned manager_delay) {
  // given
  struct thread_pool *tp = before(1);
  assert(tp);

  // when
  struct ascii_str str = rand_string(100);
  struct task_args_long args = {.sec = worker_delay, .logger = logger, .string = str};
  bool ret = tp_add_task(tp,
                         &(struct task){.args = &args,
                                        .destroy_task = long_task_destroyer,
                                        .handle_task = long_task_handler,
                                        .id = INT16_MAX});
  assert(ret);

  LOG(logger, INFO, "\n\tworker %ld (main) entering sleep\n", thrd_current());
  // pass low `manager_delay` to trigger the abort
  struct timespec remaining = {0};
  nanosleep(&(struct timespec){.tv_sec = manager_delay}, &remaining);

  LOG(logger, INFO, "\n\tworker %ld (main) woken up trying to abort (id: %zu)\n", thrd_current(), (size_t)INT16_MAX);

  assert(tp_abort_task(tp, INT16_MAX));

  LOG(logger, INFO, "\n\tworker %ld (main) successfully aborted (id: %zu)\n", thrd_current(), (size_t)INT16_MAX);

  // cleanup
  after(tp);
}

static void tp_add_task_abort_then_add_another_test(struct logger *restrict logger,
                                                    unsigned worker_delay,
                                                    unsigned manager_delay,
                                                    uint8_t threads_count) {
  LOG(logger,
      INFO,
      "\n\t testing %hhd threads (threads delay: %u, manager delay: %u)\n",
      threads_count,
      worker_delay,
      manager_delay);

  // given
  struct thread_pool *tp = before(threads_count);
  assert(tp);

  struct vec args = vec_create(sizeof(struct task_args_long), NULL);
  assert(vec_resize(&args, threads_count * 2) >= (size_t)threads_count);

  // when

  for (size_t i = 0; i < threads_count; i++) {
    struct ascii_str str = rand_string(100);
    vec_push(&args, &(struct task_args_long){.logger = logger, .sec = worker_delay, .string = str});
  }
  // struct task_args_long args = {.sec = worker_delay, .logger = logger, .string = str};

  for (size_t i = 0; i < threads_count; i++) {
    void *task_args = vec_at(&args, i);
    bool ret = tp_add_task(tp,
                           &(struct task){.args = task_args,
                                          .destroy_task = long_task_destroyer,
                                          .handle_task = long_task_handler,
                                          .id = i});
    assert(ret);
  }

  LOG(logger, INFO, "\n\tworker %ld (main) entering sleep\n", thrd_current());
  // pass low `manager_delay` to trigger the abort
  struct timespec remaining = {0};
  nanosleep(&(struct timespec){.tv_sec = manager_delay}, &remaining);

  LOG(logger, INFO, "\n\tworker %ld (main) woken up trying to abort tasks\n", thrd_current());
  for (size_t i = 0; i < threads_count; i++) { assert(tp_abort_task(tp, i)); }
  LOG(logger, INFO, "\n\tworker %ld (main) woken up successfully aborted all tasks\n", thrd_current());

  // adding one more task without aborting to esure the thread pool still works
  LOG(logger, INFO, "\n\tworker %ld (main) adding new tasks\n", thrd_current());
  for (size_t i = 0; i < threads_count; i++) {
    struct ascii_str str = rand_string(100);
    vec_push(&args, &(struct task_args_long){.logger = logger, .sec = worker_delay, .string = str});
  }

  for (size_t i = 0; i < threads_count; i++) {
    void *task_args = vec_at(&args, i + threads_count);
    assert(task_args);
    bool ret = tp_add_task(tp,
                           &(struct task){.args = task_args,
                                          .destroy_task = long_task_destroyer,
                                          .handle_task = long_task_handler,
                                          .id = i});
    assert(ret);
  }

  // sleep for a tiny while to ensure all threads started working on a task. this is performed since the data inside the
  // vector isn't going to be cleaned up (i.e. the vec & the thread pool shares the ownership of the data - which is bad
  // in all cases - however for this test - it should be fine)
  remaining = (struct timespec){0};
  nanosleep(&(struct timespec){.tv_sec = worker_delay / 2}, &remaining);
  LOG(logger, INFO, "\n\tworker %ld (main) terminating early\n", thrd_current());
  // cleanup
  after(tp);
  vec_destroy(&args);
}

int main(void) {
  srand((unsigned)time(NULL));
  struct logger *logger = logger_create("tp_sanity.log", SIGUSR1);
  assert(logger);

  tp_create_invalid_test(logger);
  tp_add_task_test(logger, 10);
  tp_add_task_test(logger, 20);
  tp_add_task_test(logger, 50);
  tp_add_task_test(logger, 100);

  tp_terminate_early_test(logger, 1, 1000);

  tp_add_multiple_tasks_test(logger, 10, 10);
  tp_add_multiple_tasks_test(logger, 10, 50);
  tp_add_multiple_tasks_test(logger, 100, 10);
  tp_add_multiple_tasks_test(logger, 100, 50);

  tp_add_task_and_abort_test(logger, 5, 1);
  tp_add_task_abort_then_add_another_test(logger, 5, 1, 1);
  tp_add_task_abort_then_add_another_test(logger, 5, 1, 2);
  tp_add_task_abort_then_add_another_test(logger, 5, 1, 10);
  tp_add_task_abort_then_add_another_test(logger, 5, 1, 50);

  logger_destroy(logger);
}
