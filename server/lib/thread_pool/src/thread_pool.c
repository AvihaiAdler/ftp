#include "thread_pool.h"
#include <stdlib.h>

struct args {
  atomic_bool stop;
  mtx_t *tasks_mtx;
  cnd_t *tasks_cnd;
  struct list *tasks;
};

struct thread {
  thrd_t id;
  struct args args;
};

struct thread_pool {
  mtx_t _tasks_mtx;
  cnd_t _tasks_cnd;

  struct vec _threads;  // vec<thread>
  struct list _tasks;   // list<task>
};

static int start(void *arg) {
  struct args *thread_args = arg;

  // as long as the thread shouldn't terminate
  while (!atomic_load(&thread_args->stop)) {
    // try to get a task
    while (mtx_lock(thread_args->tasks_mtx) != thrd_success) {
      continue;
    }

    // there're no tasks - wait
    while (list_empty(thread_args->tasks)) {

      while (cnd_wait(thread_args->tasks_cnd, thread_args->tasks_mtx) != thrd_success) {
        continue;
      }

      // if should stop - break
      if (atomic_load(&thread_args->stop)) {
        while (mtx_unlock(thread_args->tasks_mtx) != thrd_success) {
          continue;
        }

        goto thread_stop;
      }
    }

    struct task *task = list_remove_first(thread_args->tasks);

    while (mtx_unlock(thread_args->tasks_mtx) != thrd_success) {
      continue;
    }

    if (!task) continue;
    // handle the task
    // as long as task is valid the thread shouldn't stop
    if (task->handle_task) task->handle_task(task->args);
    if (task->destroy_task) task->destroy_task(task);
    free(task);
  }

thread_stop:
  return 0;
}

static void terminate(struct vec *threads, cnd_t *cnd) {
  if (!threads) return;

  for (size_t i = 0; i < vec_size(threads); i++) {
    struct thread *curr = vec_at(threads, i);
    atomic_store(&curr->args.stop, true);
  }

  while (cnd_broadcast(cnd) != thrd_success) {
    continue;
  }

  for (size_t i = 0; i < vec_size(threads); i++) {
    struct thread *curr = vec_at(threads, i);
    thrd_join(curr->id, NULL);
  }
}

static void destroy_task_internal(void *_task) {
  struct task *task = _task;

  if (task->destroy_task) task->destroy_task(task);
}

struct thread_pool *thread_pool_create(uint8_t threads_count) {
  if (!threads_count) goto invalid_thread_pool;

  struct thread_pool *tp = calloc(1, sizeof *tp);
  if (!tp) goto invalid_thread_pool;

  if (mtx_init(&tp->_tasks_mtx, mtx_plain) != thrd_success) { goto invalid_thread_pool; }
  if (cnd_init(&tp->_tasks_cnd) != thrd_success) { goto mtx_cleanup; }

  tp->_threads = vec_create(sizeof(struct thread), NULL);
  vec_reserve(&tp->_threads, threads_count);

  tp->_tasks = list_create(sizeof(struct task), destroy_task_internal);

  for (uint8_t i = 0; i < threads_count; i++) {
    struct args args =
      (struct args){.stop = false, .tasks = &tp->_tasks, .tasks_cnd = &tp->_tasks_cnd, .tasks_mtx = &tp->_tasks_mtx};
    vec_push(&tp->_threads, &(struct thread){.id = 0, .args = args});
  }

  for (size_t i = 0; i < vec_size(&tp->_threads); i++) {
    struct thread *curr_thrd = vec_at(&tp->_threads, i);
    if (curr_thrd) thrd_create(&curr_thrd->id, start, &curr_thrd->args);
  }

  return tp;

mtx_cleanup:
  mtx_destroy(&tp->_tasks_mtx);
invalid_thread_pool:
  return NULL;
}

void thread_pool_destroy(struct thread_pool *thread_pool) {
  if (!thread_pool) return;

  terminate(&thread_pool->_threads, &thread_pool->_tasks_cnd);
  vec_destroy(&thread_pool->_threads);
  list_destroy(&thread_pool->_tasks);
  mtx_destroy(&thread_pool->_tasks_mtx);
  cnd_destroy(&thread_pool->_tasks_cnd);
  free(thread_pool);
}

bool thread_pool_add_task(struct thread_pool *restrict thread_pool, struct task const *restrict task) {
  if (!thread_pool) return false;
  if (!task) return false;

  while (mtx_lock(&thread_pool->_tasks_mtx) != thrd_success) {
    continue;
  }

  bool ret = list_append(&thread_pool->_tasks, task);

  while (mtx_unlock(&thread_pool->_tasks_mtx) != thrd_success) {
    continue;
  }

  if (ret) {
    while (cnd_broadcast(&thread_pool->_tasks_cnd) != thrd_success) {
      continue;
    }
  }

  return ret;
}
