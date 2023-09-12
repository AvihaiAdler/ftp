#pragma once
/**
 * @file thread_pool.h
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @struct a task object
 */
struct task {
  size_t id;

  void *args; /**< the arguments require to execute the task casted to a `void *`. the argument must live long enough
                 for the task to use it. prefer having the task own `arg` with heap allocation if possible */

  void (*handle_task)(void *arg); /**< the task to execute. `args` will be passed into it. i.e. the thread will execute
                                    the task in this manner: `task::handle_task(task::args)`*/
  void (*destroy_task)(void *task); /**< the destructor of a task. this destructor is intended to cleanup `task::args`
                                       only! one must not try to `free` the `task` itself in any way. the thread will
                                       call the destructor in this manner: `task::destroy_task(task)` */
};

/**
 * @brief
 * NOTE: the pool uses `SIGUSR1` to signal threads to 'abort' their current task. one must not raise said signal
 */
struct thread_pool;

/**
 * @brief creates a thread pool with `threads_count` threads
 *
 * @param[in] threads_count the number of threads to spawn
 * @return `struct thread_pool`
 */
struct thread_pool *tp_create(uint8_t threads_count);

/**
 * @brief terminates all threads gracefully and destroys a thread pool
 *
 * @param[in] thread_pool
 */
void tp_destroy(struct thread_pool *thread_pool);

/**
 * @brief adds a task to be executes asynchronously
 *
 * @param[in] thread_pool
 * @param[in] task the task to execute
 * @return `true` if the operation succeded
 * @return `false` if the operation failed
 */
bool tp_add_task(struct thread_pool *restrict thread_pool, struct task const *restrict task);

/**
 * @brief aborts a task if said task is currently being executed
 *
 * NOTE: this function is expensive. the calling thread might block for a long time
 * NOTE: the use of this function impose the following restrictions on the task one wish to abort:
 * 1. any critical section within `task::handle_task` must first call `tp_critical_section_begin` then aquire the mutex
 * / semaphore. `tp_critical_section_end` must be called after exiting said section. failure to comply with this
 * restriction might cause a deadlock
 * 2. any heap allocation within `task::handle_task` which isn't saved into `task::args` in some way - will not be
 * cleaned up.
 * 3. using non signal-safe functions without blocking `SIGUSR1` might cause weird or even undefined behavior. for a
 * list of signal safe functions for linux please refer to https://man7.org/linux/man-pages/man7/signal-safety.7.html.
 * one can call `tp_critical_section_begin` & `tp_critical_section_end` before and after said function call to remedy
 * the issue.
 * 4. `tp_abort_task` is signal-safe & thread safe
 *
 * @param[in] thread_pool
 * @param[in] task_id
 * @return true
 * @return false
 */
bool tp_abort_task(struct thread_pool *restrict thread_pool, size_t task_id);

/**
 * @brief does the necessary work to make a critical section signal safe in regard to `SIGUSR1`
 * NOTE: must be accompanied with a `tp_critical_section_end` call
 *
 * @return `true` on success
 * @return `false` otherwise
 */
bool tp_critical_section_begin(void);

/**
 * @brief restore previous state after a `tp_critical_section_begin` call
 *
 * @return `true` on success
 * @return `false` otherwise
 */
bool tp_critical_section_end(void);
