#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "logger.h"
#include "session.h"
#include "thread_pool.h"
#include "util.h"
#include "vec.h"

void sigint_handler(int signum) {
  (void)signum;

  atomic_store(&global_terminate, true);
}

_Atomic(bool) global_terminate;

int main(int argc, char *argv[]) {
  /*
   * create logger
   */
  char const *logger_file = "ftpd.log";  // TODO: logger file should be read from a config file
  struct logger *logger = logger_create(logger_file, SIGUSR1);
  if (!logger) {
    fprintf(stderr, "failed to create a logger\n");
    return 1;
  }

  /*
   * create a thread pool
   */
  uint8_t threads_count = 100;  // TODO: the thread count should be read from a config file
  struct thread_pool *tp = tp_create(threads_count);
  if (!tp) {
    LOG(logger, ERROR, "failed to create a thread pool with %hhd threads", threads_count);
    goto logger_cleanup;
  }

  /*
   * create a vec of sessions
   */
  struct vec sessions = vec_create(sizeof(struct session), session_destroy_wrapper);
  if (!sessions._data) {
    LOG(logger, ERROR, "%s\n", "failed to create sessions");
    goto thread_pool_cleanup;
  }

  if (sig_handler_install(SIGINT, sigint_handler)) {
    LOG(logger, ERROR, "failed to install a signal handler for signal: %d\n", SIGINT);
    goto logger_cleanup;
  }

  atomic_store(&global_terminate, false);  // global init
  // main event loop
  while (!atomic_load(&global_terminate)) {
    // TODO:
  }

sessions_cleanup:
  vec_destroy(&sessions);
thread_pool_cleanup:
  tp_destroy(tp);
logger_cleanup:
  logger_destroy(logger);

  return 0;
}
