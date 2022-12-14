#pragma once
#include <stdbool.h>

#define LOG(logger, level, fmt, ...)                                                             \
  do {                                                                                           \
    logger_log(logger, level, "in %s\t%s:%d " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

struct logger;

/* a naive logger implementation for multithreaded programs. the logger must be
 * initialized before attempting to use it with log_init. file_name may be NULL, in such case - the logger will output
 * to stdout */

/* log levels */
enum level { ERROR, WARN, DEBUG, INFO };

/* initializes the logger 'object'. must be called before any use of the logger.
 * expects a valid file_name. if the file doesn't exists - the logger will
 * create one for you. file_name may be NULL - in such case the logger will
 * output to stdout. returns a pointer to a logger object on success, NULL
 * otherwise */
struct logger *logger_init(char *file_name);

/* logs a message. fmt is a string which may contains the specifiers used by fprintf. if such specifiers found - expects
 * matching number of argumnets */
void logger_log(struct logger *logger, enum level level, const char *fmt, ...);

/* destroys the logger 'object'. must be called after all threads are
 * joined(or killed). any attempt to call the function while the threads
 * still uses the logger - may result in undefined behavior */
void logger_destroy(struct logger *logger);
