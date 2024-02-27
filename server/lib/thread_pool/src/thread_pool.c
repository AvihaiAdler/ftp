#include "thread_pool.h"
#include <setjmp.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include "queue.h"
#include "vec.h"

#define INVALID_IDX -1

struct context {
  atomic_int id;
  uint8_t count;
  sigjmp_buf *buffers;
};

// all elements in this struct shall not be written to unless explicitly stated otherwise
struct thread_properties {
  uint8_t id;
  atomic_bool terminate;  // may be written to

  struct {
    mtx_t *mtx;
    cnd_t *cnd;

    struct queue *queue;  // may be written to if `tasks::mtx` is acquired
  } tasks;

  struct {
    mtx_t mtx;
    size_t task_id;

    enum {
      STATE_IDLE,
      STATE_BUSY,
    } value;
  } state;  // may be written to if `state::mtx` is acquired
};

struct thread {
  thrd_t id;

  struct thread_properties properties;
};

struct thread_pool {
  mtx_t _tasks_mtx;
  cnd_t _tasks_cnd;

  // the vec itself shall not be written to as long as the threads are running thus in this narrow context it can be
  // assumed to be thread safe. changing its underlying elements (the threads) must be done in a thread safe manner
  struct vec _threads;  // vec<thread>

  struct queue _tasks;  // queue<task>
};

static struct context global_context;  // IMPORTANT

static void sig_handler(int signum) {
  if (signum != SIGUSR1) return;

  int prev;
  int idx = atomic_load(&global_context.id);
  do {
    prev = idx;
  } while (!atomic_compare_exchange_weak(&global_context.id, &prev, INVALID_IDX));

  if (idx >= 0 && idx < global_context.count) siglongjmp(global_context.buffers[idx], 0);
}

static bool thread_block_signal(int signum) {
  sigset_t sig_to_block;
  if (sigemptyset(&sig_to_block) != 0) return false;
  if (sigaddset(&sig_to_block, signum) != 0) return false;

  return pthread_sigmask(SIG_BLOCK, &sig_to_block, NULL) == 0;
}

static bool thread_unblock_signal(int signum) {
  sigset_t sig_to_block;
  if (sigemptyset(&sig_to_block) != 0) return false;
  if (sigaddset(&sig_to_block, signum) != 0) return false;

  return pthread_sigmask(SIG_UNBLOCK, &sig_to_block, NULL) == 0;
}

static bool process_block_signal(int signum) {
  sigset_t sig_to_block;
  if (sigemptyset(&sig_to_block) != 0) return false;
  if (sigaddset(&sig_to_block, signum) != 0) return false;

  return sigprocmask(SIG_BLOCK, &sig_to_block, NULL) == 0;
}

static void update_state(struct thread_properties *properties, int state, size_t task_id) {
  while (mtx_lock(&properties->state.mtx) != thrd_success) {
    continue;
  }

  switch (state) {
    case STATE_BUSY:  // fallthrough
      properties->state.task_id = task_id;
    case STATE_IDLE:
      properties->state.value = state;
      break;
    default:
      break;
  }

  while (mtx_unlock(&properties->state.mtx) != thrd_success) {
    continue;
  }
}

static int thread_launch(void *arg) {
  thread_block_signal(SIGUSR1);
  if (!arg) return 1;

  volatile struct thread_properties *properties = arg;

  // as long as the thread shouldn't terminate
  while (!atomic_load(&properties->terminate)) {
    // try to get a task
    while (mtx_lock(properties->tasks.mtx) != thrd_success) {
      continue;
    }

    // there are no tasks. release the lock and wait
    while (queue_empty(properties->tasks.queue)) {
      while (cnd_wait(properties->tasks.cnd, properties->tasks.mtx) != thrd_success) {
        continue;
      }

      // woken up but should terminate - release lock & terminate
      if (atomic_load(&properties->terminate)) {
        while (mtx_unlock(properties->tasks.mtx) != thrd_success) {
          continue;
        }

        goto thread_exit;
      }
    }

    // get a task & release the lock
    struct task task = {0};
    enum ds_error ret = queue_dequeue(properties->tasks.queue, &task);

    while (mtx_unlock(properties->tasks.mtx) != thrd_success) {
      continue;
    }

    // if (!task) continue;
    if (ret != DS_VALUE_OK) continue;

    // update state
    update_state((struct thread_properties *)properties, STATE_BUSY, task.id);
    thread_unblock_signal(SIGUSR1);

    // handle the task
    if (sigsetjmp(global_context.buffers[properties->id], 1) == 0) {
      if (task.handle_task) task.handle_task(task.args);
    }

    if (task.destroy_task) task.destroy_task(&task);

    // update state
    update_state((struct thread_properties *)properties, STATE_IDLE, 0);
  }

thread_exit:
  return 0;
}

static void terminate(struct vec *threads, cnd_t *cnd) {
  if (!threads) return;

  for (size_t i = 0; i < vec_size(threads); i++) {
    struct thread *curr = vec_at(threads, i);
    atomic_store_explicit(&curr->properties.terminate, true, memory_order_seq_cst);
  }

  while (cnd_broadcast(cnd) != thrd_success) {
    continue;
  }
  for (size_t i = 0; i < vec_size(threads); i++) {
    struct thread *curr = vec_at(threads, i);
    thrd_join(curr->id, NULL);
  }
}

static void tp_destroy_internal(struct thread_pool *tp) {
  queue_destroy(&tp->_tasks);
  vec_destroy(&tp->_threads);
  cnd_destroy(&tp->_tasks_cnd);
  mtx_destroy(&tp->_tasks_mtx);
  free(tp);
}

void tp_destroy(struct thread_pool *thread_pool) {
  if (!thread_pool) return;

  terminate(&thread_pool->_threads, &thread_pool->_tasks_cnd);
  tp_destroy_internal(thread_pool);

  free(global_context.buffers);

  (void)thread_unblock_signal(SIGUSR1);
}

static void task_destroy(void *_task) {
  struct task *task = _task;

  if (task->destroy_task) task->destroy_task(task);
}

static void thread_destroy(void *_thread) {
  struct thread *thread = _thread;

  mtx_destroy(&thread->properties.state.mtx);
}

static bool thread_create(struct thread *restrict thread,
                          uint8_t id,
                          struct queue *restrict tasks,
                          mtx_t *restrict tasks_mtx,
                          cnd_t *restrict tasks_cnd) {
  if (!thread) return false;
  if (!tasks || !tasks_mtx || !tasks_cnd) return false;

  *thread = (struct thread){.id = 0,
                            .properties = {.id = id,
                                           .tasks = {.cnd = tasks_cnd, .mtx = tasks_mtx, .queue = tasks},
                                           .state = {.task_id = 0, .value = STATE_IDLE}}};

  atomic_init(&thread->properties.terminate, false);
  return mtx_init(&thread->properties.state.mtx, mtx_plain) == thrd_success;
}

static bool install_handler(int signum) {
  struct sigaction act = {.sa_handler = sig_handler};

  return sigaction(signum, &act, NULL) == 0;
}

// global init
static bool context_init(uint8_t count) {
  atomic_init(&global_context.id, INVALID_IDX);

  global_context.count = count;
  global_context.buffers = malloc(count * sizeof *global_context.buffers);
  return global_context.buffers != NULL;
}

struct thread_pool *tp_create(uint8_t threads_count) {
  if (!threads_count) goto invalid_thread_pool;
  if (!context_init(threads_count)) goto invalid_thread_pool;

  // constructing the mask for all threads. all threads shall block SIGINT and install a handler for SIGUSR1
  // the pool uses SIGUSR1 to signal threads to 'abort' their current task
  if (!install_handler(SIGUSR1)) goto context_cleanup;
  if (!process_block_signal(SIGINT)) goto context_cleanup;

  struct thread_pool *tp = calloc(1, sizeof *tp);
  if (!tp) goto context_cleanup;

  if (mtx_init(&tp->_tasks_mtx, mtx_plain) != thrd_success) { goto context_cleanup; }
  if (cnd_init(&tp->_tasks_cnd) != thrd_success) { goto mtx_cleanup; }

  tp->_threads = vec_create(sizeof(struct thread), thread_destroy);
  vec_reserve(&tp->_threads, threads_count);

  tp->_tasks = queue_create(sizeof(struct task), task_destroy);

  for (uint8_t i = 0; i < threads_count; i++) {
    struct thread thread;
    if (!thread_create(&thread, i, &tp->_tasks, &tp->_tasks_mtx, &tp->_tasks_cnd)) {
      tp_destroy_internal(tp);
      goto invalid_thread_pool;
    }
    vec_push(&tp->_threads, &thread);
  }

  // constructing the mask for the main thread. the main thread shall not handle SIGUSR1
  if (!thread_block_signal(SIGUSR1)) {
    tp_destroy_internal(tp);
    goto invalid_thread_pool;
  }

  // restoring the mask for the main thread. the main thread is no longer blocking SIGINT
  if (!thread_unblock_signal(SIGINT)) {
    tp_destroy_internal(tp);
    goto invalid_thread_pool;
  }

  for (size_t i = 0; i < vec_size(&tp->_threads); i++) {
    struct thread *curr_thrd = vec_at(&tp->_threads, i);
    if (curr_thrd) thrd_create(&curr_thrd->id, thread_launch, &curr_thrd->properties);
  }

  return tp;

mtx_cleanup:
  mtx_destroy(&tp->_tasks_mtx);
  free(tp);
context_cleanup:
  free(global_context.buffers);
invalid_thread_pool:
  return NULL;
}

bool tp_add_task(struct thread_pool *restrict thread_pool, struct task const *restrict task) {
  if (!thread_pool) return false;
  if (!task) return false;

  if (!thread_block_signal(SIGUSR1)) return false;

  while (mtx_lock(&thread_pool->_tasks_mtx) != thrd_success) {
    continue;
  }

  enum ds_error ret = queue_enqueue(&thread_pool->_tasks, task);

  while (mtx_unlock(&thread_pool->_tasks_mtx) != thrd_success) {
    continue;
  }

  if (ret == DS_OK) {
    while (cnd_broadcast(&thread_pool->_tasks_cnd) != thrd_success) {
      continue;
    }
  }

  (void)thread_unblock_signal(SIGUSR1);
  return ret == DS_OK;
}

// guaratnee to abort only threads marked as BUSY. i.e. the thread shouldn't block / unblock SIGUSR1 before / after
// handling a task
bool tp_abort_task(struct thread_pool *restrict thread_pool, size_t task_id) {
  if (!thread_block_signal(SIGUSR1)) return false;

  // nothing fancy. loop over all the threads and look for the one who's BUSY executing `task_id`
  for (size_t i = 0; i < vec_size(&thread_pool->_threads); i++) {
    struct thread *curr = vec_at(&thread_pool->_threads, i);

    if (curr->id == thrd_current()) {  // `curr` is this current self - don't take any action
      continue;
    }

    while (mtx_lock(&curr->properties.state.mtx) != thrd_success) {
      continue;
    }

    if (curr->properties.state.value == STATE_BUSY && curr->properties.state.task_id == task_id) {

      // signal the thread to abort:
      int idx;
      do {
        idx = INVALID_IDX;
      } while (!atomic_compare_exchange_weak(&global_context.id, &idx, i));

      while (mtx_unlock(&curr->properties.state.mtx) != thrd_success) {
        continue;
      }

      bool ret = pthread_kill(curr->id, SIGUSR1) == 0;
      (void)thread_unblock_signal(SIGUSR1);

      return ret;
    }

    while (mtx_unlock(&curr->properties.state.mtx) != thrd_success) {
      continue;
    }
  }

  (void)thread_unblock_signal(SIGUSR1);
  return false;
}

bool tp_critical_section_begin(void) {
  return thread_block_signal(SIGUSR1);
}

bool tp_critical_section_end(void) {
  return thread_unblock_signal(SIGUSR1);
}
