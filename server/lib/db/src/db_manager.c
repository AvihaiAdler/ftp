#include "db_manager.h"
#include <stdarg.h>

sqlite3 *dbm_open(char const *restrict db_name) {
  int flags = SQLITE_READONLY;
  if (!db_name) {
    db_name = ":memory:";
    flags |= SQLITE_OPEN_MEMORY;
  }

  sqlite3 *db = NULL;
  int ret = sqlite3_open_v2(db_name, &db, flags, NULL);
  if (ret != SQLITE_OK) { return NULL; }

  return db;
}

void dbm_destroy(sqlite3 *restrict db) {
  if (!db) return;

  sqlite3_close(db);
}

sqlite3_stmt *dbm_statement_prepare(sqlite3 *restrict db, char const *restrict query, size_t query_size) {
  if (!db || !query) { return NULL; }

  sqlite3_stmt *statement = NULL;

  // while sqlite3_prepare_v3 with the flag SQLITE_PREPARE_PERSISTENT will be more suitable for other applications -
  // since this tiny wrapper deals with a case where only 1 or 2 statements should be constructed - its not as important
  if (sqlite3_prepare_v2(db, query, query_size, &statement, NULL) != SQLITE_OK) { return NULL; }

  return statement;
}

void dbm_statement_destroy(sqlite3_stmt *restrict statement) {
  if (!statement) return;

  sqlite3_finalize(statement);
}

static void process_row(sqlite3_stmt *restrict statement,
                        void (*callback)(void *restrict arg,
                                         char const *restrict col_name,
                                         char const *restrict col_data),
                        void *restrict arg) {
  size_t cols_count = sqlite3_column_count(statement);

  for (size_t i = 0; i < cols_count; i++) {
    unsigned char const *col_name = sqlite3_column_text(statement, i);
    unsigned char const *col_data = sqlite3_column_text(statement, i);

    callback(arg, (char const *)col_name, (char const *)col_data);
  }
}

static int dbm_statement_query_internal(sqlite3_stmt *restrict statement,
                                        void (*callback)(void *restrict arg,
                                                         char const *restrict col_name,
                                                         char const *restrict col_data),
                                        void *restrict arg,
                                        size_t args_count,
                                        va_list args) {
  // bind the args to the query
  for (size_t i = 0; i < args_count; i++) {
    char const *curr = va_arg(args, char const *);
    if (sqlite3_bind_text(statement, i + 1, curr, -1, SQLITE_STATIC) != SQLITE_OK) {
      sqlite3_clear_bindings(statement);
      return SQLITE_ERROR;
    }
  }

  // execute query
  int step_ret = SQLITE_DONE;
  do {
    step_ret = sqlite3_step(statement);
    if (step_ret == SQLITE_ROW) { process_row(statement, callback, arg); }
  } while (step_ret != SQLITE_DONE);

  int ret = sqlite3_reset(statement);
  if (ret != SQLITE_OK) { return ret; }

  ret = sqlite3_clear_bindings(statement);
  if (ret != SQLITE_OK) { return ret; }

  return SQLITE_OK;
}

int dbm_statement_query(sqlite3_stmt *restrict statement,
                        void (*callback)(void *restrict arg,
                                         char const *restrict col_name,
                                         char const *restrict col_data),
                        void *restrict arg,
                        size_t args_count,
                        ...) {
  if (!statement || !callback) { return SQLITE_ERROR; }

  va_list args;
  va_start(args, args_count);
  int ret = dbm_statement_query_internal(statement, callback, arg, args_count, args);
  va_end(args);

  return ret;
}

int dbm_query(sqlite3 *restrict db,
              void (*callback)(void *restrict arg, char const *restrict col_name, char const *restrict col_data),
              void *restrict arg,
              char const *restrict query,
              size_t args_count,
              ...) {
  if (!db) { return SQLITE_NOTADB; }
  if (!callback) { return SQLITE_ERROR; }

  sqlite3_stmt *statement = dbm_statement_prepare(db, query, -1);
  if (!statement) { return SQLITE_ERROR; }

  va_list args;
  va_start(args, args_count);
  dbm_statement_query_internal(statement, callback, arg, args_count, args);
  va_end(args);

  dbm_statement_destroy(statement);
  return SQLITE_OK;
}
