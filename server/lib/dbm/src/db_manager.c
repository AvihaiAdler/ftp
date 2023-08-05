#include "db_manager.h"
#include <stdarg.h>
#include <string.h>
#include "vec.h"

static int cmpr(void const *left_, void const *right_) {
  size_t const *left = left_;
  size_t const *right = right_;

  return *left > *right - *left < *right;
}

static void destroy_str(void *ascii_str) {
  ascii_str_destroy((struct ascii_str *)ascii_str);
}

static void destroy_vec(void *vec_ptr) {
  struct vec **vec = vec_ptr;
  vec_destroy(*vec);
}

sqlite3 *dbm_open(char const *restrict db_name) {
  if (!db_name) { db_name = ":memory:"; }

  sqlite3 *db = NULL;
  int ret = sqlite3_open(db_name, &db);
  if (ret != SQLITE_OK) { return NULL; }

  return db;
}

void dbm_destroy(sqlite3 *restrict db) {
  if (!db) return;

  sqlite3_close(db);
}

static sqlite3_stmt *dbm_statement_prepare_internal(sqlite3 *restrict db,
                                                    char const *restrict query,
                                                    size_t query_size) {
  sqlite3_stmt *statement = NULL;

  // while sqlite3_prepare_v3 with the flag SQLITE_PREPARE_PERSISTENT will be more suitable for other applications -
  // since this tiny wrapper deals with a case where only 1 or 2 statements should be constructed - its not as important
  if (sqlite3_prepare_v2(db, query, query_size, &statement, NULL) != SQLITE_OK) { return NULL; }

  return statement;
}

sqlite3_stmt *dbm_statement_prepare(sqlite3 *restrict db, struct ascii_str *restrict query) {
  if (!db || !query) { return NULL; }
  return dbm_statement_prepare_internal(db, ascii_str_c_str(query), ascii_str_len(query));
}

void dbm_statement_destroy(sqlite3_stmt *restrict statement) {
  if (!statement) return;

  sqlite3_finalize(statement);
}

static struct vec *dbm_process_row_internal(sqlite3_stmt *restrict statement) {
  struct vec *row_data = vec_init(sizeof(struct ascii_str), destroy_str);
  if (!row_data) return NULL;

  size_t col_count = sqlite3_column_count(statement);

  // even numbers correspond to column names and thus ignored
  for (size_t i = 1; i < col_count; i += 2) {
    struct ascii_str col_data = ascii_str_create((char const *)sqlite3_column_text(statement, i), STR_C_STR);

    if (!vec_push(row_data, &col_data)) {
      if (row_data) vec_destroy(row_data);
      return NULL;
    }
  }

  return row_data;
}

static bool dbm_statement_cleanup_internal(sqlite3_stmt *restrict statement) {
  return (sqlite3_reset(statement) | sqlite3_clear_bindings(statement)) == SQLITE_OK;
}

struct hash_table *dbm_statement_query_internal(sqlite3_stmt *restrict statement, size_t args_count, va_list args) {
  for (size_t i = 0; i < args_count; i++) {
    if (sqlite3_bind_text(statement, i + 1, va_arg(args, char const *), -1, SQLITE_STATIC) != SQLITE_OK) {
      sqlite3_clear_bindings(statement);
      return NULL;
    }
  }

  struct hash_table *ht = table_init(cmpr, NULL, destroy_vec);
  if (!ht) {
    (void)dbm_statement_cleanup_internal(statement);
    return NULL;
  }

  // execute query
  int step = SQLITE_OK;
  for (size_t i = 0; step != SQLITE_DONE; i++) {
    step = sqlite3_step(statement);

    // populate ht
    if (step != SQLITE_ROW) continue;

    struct vec *row_data = dbm_process_row_internal(statement);
    if (!row_data) {
      (void)dbm_statement_cleanup_internal(statement);
      table_destroy(ht);
      return NULL;
    }

    (void)table_put(ht, &i, sizeof i, &row_data, sizeof row_data);
  }

  if (!dbm_statement_cleanup_internal(statement)) {
    table_destroy(ht);
    return NULL;
  }

  return ht;
}

struct hash_table *dbm_query(sqlite3 *restrict db,
                             int *restrict status,
                             struct ascii_str *restrict query,
                             size_t args_count,
                             ...) {
  if (!db) {
    if (status) *status = SQLITE_NOTADB;
    return NULL;
  }

  if (!query) { return NULL; }

  sqlite3_stmt *statement = dbm_statement_prepare(db, query);
  if (!statement) {
    if (status) *status = SQLITE_ERROR;
    return NULL;
  }

  va_list args;
  va_start(args, args_count);
  struct hash_table *ht = dbm_statement_query_internal(statement, args_count, args);
  va_end(args);

  dbm_statement_destroy(statement);

  if (status) { *status = ht ? SQLITE_OK : SQLITE_ERROR; }
  return ht;
}

struct hash_table *dbm_statement_query(sqlite3_stmt *restrict statement, int *status, size_t args_count, ...) {
  if (!statement) {
    if (status) *status = SQLITE_MISUSE;
    return NULL;
  }

  va_list args;
  va_start(args, args_count);
  struct hash_table *ht = dbm_statement_query_internal(statement, args_count, args);
  va_end(args);

  if (status) { *status = ht ? SQLITE_OK : SQLITE_ERROR; }
  return ht;
}

static void process_row(sqlite3_stmt *restrict statement,
                        void (*callback)(void *restrict arg,
                                         char const *restrict col_name,
                                         char const *restrict col_data),
                        void *restrict arg) {
  size_t cols_count = sqlite3_column_count(statement);

  for (size_t i = 0; i < cols_count; i += 2) {
    unsigned char const *col_name = sqlite3_column_text(statement, i);
    unsigned char const *col_data = sqlite3_column_text(statement, i + 1);

    callback(arg, (char const *)col_name, (char const *)col_data);
  }
}

static int dbm_statement_query_internal2(sqlite3_stmt *restrict statement,
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
    if (step_ret == SQLITE_ROW) {
      if (callback) { process_row(statement, callback, arg); }
    }
  } while (step_ret != SQLITE_DONE);

  int ret = sqlite3_reset(statement);
  if (ret != SQLITE_OK) { return ret; }

  ret = sqlite3_clear_bindings(statement);
  if (ret != SQLITE_OK) { return ret; }

  return SQLITE_OK;
}

int dbm_statement_query2(sqlite3_stmt *restrict statement,
                         void (*callback)(void *restrict arg,
                                          char const *restrict col_name,
                                          char const *restrict col_data),
                         void *restrict arg,
                         size_t args_count,
                         ...) {
  if (!statement) { return SQLITE_ERROR; }

  va_list args;
  va_start(args, args_count);
  int ret = dbm_statement_query_internal2(statement, callback, arg, args_count, args);
  va_end(args);

  return ret;
}

int dbm_query2(sqlite3 *restrict db,
               void (*callback)(void *restrict arg, char const *restrict col_name, char const *restrict col_data),
               void *restrict arg,
               char const *restrict query,
               size_t args_count,
               ...) {
  if (!db) { return SQLITE_NOTADB; }
  if (!query) { return SQLITE_MISUSE; }

  sqlite3_stmt *statement = dbm_statement_prepare_internal(db, query, -1);
  if (!statement) { return SQLITE_ERROR; }

  va_list args;
  va_start(args, args_count);
  dbm_statement_query_internal2(statement, callback, arg, args_count, args);
  va_end(args);

  dbm_statement_destroy(statement);
  return SQLITE_OK;
}
