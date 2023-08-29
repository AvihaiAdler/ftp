#pragma once
/**
 * @file thread_pool.h
 */
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>
#include "list.h"
#include "vec.h"

struct thread_pool;

/**
 * @brief task object
 */
struct task {
  void *args;
  int (*handle_task)(void *arg);
  void (*destroy_task)(void *arg);
};

/**
 * @brief creates a thread pool with `threads_count` threads
 *
 * @param[in] threads_count the number of threads to spawn
 * @return `struct thread_pool`
 */
struct thread_pool *thread_pool_create(uint8_t threads_count);

/**
 * @brief terminates all threads gracefully and destroys a thread pool
 *
 * @param[in] thread_pool
 */
void thread_pool_destroy(struct thread_pool *thread_pool);

/**
 * @brief adds a task to be executes asynchronously
 *
 * @param[in] thread_pool
 * @param[in] task the task to execute
 * @return `true` if the operation succeded
 * @return `false` if the operation failed
 */
bool thread_pool_add_task(struct thread_pool *restrict thread_pool, struct task const *restrict task);
