#include "util.h"
#include <signal.h>
#include <stddef.h>
#include "session.h"

static bool process_block_signal(int signum) {
  sigset_t sig_to_block;
  if (sigemptyset(&sig_to_block) != 0) return false;
  if (sigaddset(&sig_to_block, signum) != 0) return false;

  return sigprocmask(SIG_BLOCK, &sig_to_block, NULL) == 0;
}

static bool process_unblock_signal(int signum) {
  sigset_t sig_to_block;
  if (sigemptyset(&sig_to_block) != 0) return false;
  if (sigaddset(&sig_to_block, signum) != 0) return false;

  return sigprocmask(SIG_UNBLOCK, &sig_to_block, NULL) == 0;
}

bool sig_handler_install(int signum, void (*sig_handler)(int)) {
  if (!sig_handler) return false;

  if (!process_block_signal(signum)) return false;
  struct sigaction act = {.sa_handler = sig_handler};

  bool ret = sigaction(signum, &act, NULL) == 0;

  if (!process_unblock_signal(signum)) return false;
  return ret;
}

void session_destroy_wrapper(void *session) {
  session_destroy(session);
}