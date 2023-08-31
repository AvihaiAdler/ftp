#pragma once
#include <stdbool.h>

/**
 * @file logger.h
 * @brief a simple logger for multithreaded environment
 * note that `logger` only blocks `SIGUSR1` during logging. this means that
 */

#define SIG_NONE -1

#define LOG(logger, level, fmt, ...)                                                           \
  do {                                                                                         \
    logger_log(logger, level, "in %s\t%s:%d " fmt, __func__, __FILE__, __LINE__, __VA_ARGS__); \
  } while (0)

struct logger;

/* a naive logger implementation for multithreaded programs. the logger must be
 * initialized before attempting to use it with log_init. file_name may be NULL, in such case - the logger will output
 * to stdout */

/* log levels */
enum level {
  ERROR,
  WARN,
  DEBUG,
  INFO,
};

/**
 * @brief creates a logger
 *
 * @param file_name the file to write into or `NULL`. if `NULL` `logger` will writes to `stdout`
 * @param signal a signal to block during the internal logging process. if `signal` is `SIG_NONE` no singals will be
 * blocked
 * @return `struct logger*`
 */
struct logger *logger_create(char const *restrict file_name, int signal);

/* logs a message. fmt is a string which may contains the specifiers used by fprintf. if such specifiers found - expects
 * matching number of argumnets */
void logger_log(struct logger *restrict logger, enum level level, char const *restrict fmt, ...);

/* destroys the logger 'object'. must be called after all threads are
 * joined(or killed). any attempt to call the function while the threads
 * still uses the logger - may result in undefined behavior */
void logger_destroy(struct logger *restrict logger);
