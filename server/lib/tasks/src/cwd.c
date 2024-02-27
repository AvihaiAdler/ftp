#include "cwd.h"
#include <dirent.h>
#include <stdlib.h>
#include <threads.h>
#include "hash_table.h"
#include "logger.h"
#include "session.h"
#include "task_args.h"
#include "thread_pool.h"

void task_cwd(void *_arg) {
  if (!_arg) return;

  struct task_args *arg = _arg;
  if (arg->cmd.command != CMD_CWD) {
    LOG(arg->logger, ERROR, "expected command type: %d but recieved %d\n", CMD_CWD, arg->cmd.command);
    goto cwd_cleanup;
  }

  if (!tp_critical_section_begin()) {
    LOG(arg->logger, ERROR, "%s\n", "failed to start a critical section block");
    goto cwd_cleanup;
  }

  struct session session;
  while (mtx_lock(arg->sessions_mtx) != thrd_success) {
    continue;
  }

  int err = table_get(arg->sessions, &arg->id, &session);

  while (mtx_unlock(arg->sessions_mtx) != thrd_success) {
    continue;
  }

  if (!tp_critical_section_end()) {  // the thread will no longer be cancellable
    LOG(arg->logger, ERROR, "%s\n", "failed to end a critical section block");
    goto cwd_cleanup;
  }

  if (err != DS_VALUE_OK) {
    LOG(arg->logger, ERROR, "failed to find a session for key %s\n", ascii_str_c_str(&arg->id));
    goto cwd_cleanup;
  }

  /* TODO:
   * try to get a handle to the directory in session::working_directory/arg->command::arg
   * if such directory exists:
   *    destroy session::curr_directory
   *    set session::curr_directroy to arg->command::arg
   *    insert the new modified session into sessions
   *    send confirmation
   * else:
   *    sends error code
   */

cwd_cleanup:
  ascii_str_destroy(&arg->id);
  command_destroy(&arg->cmd);
  free(arg);
}
