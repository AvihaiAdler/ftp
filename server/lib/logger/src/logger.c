#include "logger.h"

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define SIZE 128

struct logger {
  enum { TYPE_SYSTEM = 0, TYPE_FILE } stream_type;
  int signal;  // no writes after initialization. shouldn't cause a data race
  FILE *stream;
  mtx_t stream_mtx;
};

// get time - thread safe
static void get_time(struct logger *restrict logger, char *restrict time_rep, size_t size) {
  if (!logger) return;

  time_t now = time(NULL);
  struct tm *calendar_time = NULL;
  struct tm tmp = {0};
  if (now != (time_t)-1) { calendar_time = gmtime_r(&now, &tmp); }

  // convert calendar time to text
  if (calendar_time) { strftime(time_rep, size, "%F %T %z", calendar_time); }
}

// still single threaded at this point
struct logger *logger_create(char const *restrict file_name, int signal) {
  struct logger *log = calloc(1, sizeof *log);
  if (!log) { return NULL; }

  FILE *log_fp = stdout;
  if (file_name) {
    log->stream_type = TYPE_FILE;
    log_fp = fopen(file_name, "a");
  }

  log->stream = log_fp;
  log->signal = signal;

  if (mtx_init(&log->stream_mtx, mtx_plain) != thrd_success) {
    if (log->stream_type == TYPE_FILE) fclose(log->stream);
    free(log);
    return NULL;
  }

  return log;
}

static const char *get_log_level(enum level level) {
  switch (level) {
    case ERROR:
      return "ERROR";
    case WARN:
      return "WARN";
    case DEBUG:
      return "DEBUG";
    case INFO:
      return "INFO";
    default:
      return "OTHER";
  }
}

static bool block_signals(sigset_t *restrict sigset_cache, int signum) {
  if (!sigset_cache) return false;

  sigset_t block;
  if (sigemptyset(&block) != 0) return false;
  if (sigaddset(&block, signum) != 0) return false;

  return pthread_sigmask(SIG_BLOCK, &block, sigset_cache) == 0;
}

static bool unblock_signals(sigset_t *restrict sigset_cache) {
  if (!sigset_cache) return false;

  return pthread_sigmask(SIG_SETMASK, sigset_cache, NULL) == 0;
}

void logger_log(struct logger *restrict logger, enum level level, char const *restrict fmt, ...) {
  if (!logger) return;

  sigset_t cache;
  if (logger->signal != SIG_NONE) {
    if (!block_signals(&cache, logger->signal)) return;
  }

  char time_buf[SIZE] = {0};
  get_time(logger, time_buf, sizeof time_buf);

  va_list args;
  va_start(args, fmt);
  while (mtx_lock(&logger->stream_mtx) != thrd_success) {
    continue;
  }

  fprintf(logger->stream, "[%s] : [%s] ", time_buf, get_log_level(level));
  // not the best idea to have the user control the format string, but oh well
  vfprintf(logger->stream, fmt, args);
  fflush(logger->stream);

  while (mtx_unlock(&logger->stream_mtx) != thrd_success) {
    continue;
  }
  va_end(args);

  if (logger->signal != SIG_NONE) {
    (void)unblock_signals(&cache);  // assume naver fails
  }
}

void logger_destroy(struct logger *restrict logger) {
  if (!logger) return;

  mtx_destroy(&logger->stream_mtx);

  fflush(logger->stream);
  if (logger->stream_type == TYPE_FILE) { fclose(logger->stream); }
  free(logger);
}
