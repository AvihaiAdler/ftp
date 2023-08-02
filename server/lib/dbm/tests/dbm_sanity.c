#include <assert.h>
#include <stdio.h>
#include "db_manager.h"

#define TABLE_NAME "dummy_table"
#define SIZE 128
#define STRINGIFY_SIZE(SIZE) "(" #SIZE ")"
#define STRINGIFY(TYPE, SIZE) #TYPE STRINGIFY_SIZE(SIZE)

static void test_create_table(sqlite3 *restrict db) {
  int ret = dbm_query2(db,
                       NULL,
                       NULL,
                       "CREATE TABLE IF NOT EXISTS " TABLE_NAME
                       " (id " STRINGIFY(VARCHAR, SIZE) ", data " STRINGIFY(VARCHAR, SIZE) ")",
                       0);

  if (ret != SQLITE_OK) { fprintf(stderr, "%s : %d\t%s\n", __func__, __LINE__, sqlite3_errstr(ret)); }

  assert(ret == SQLITE_OK);
}

static void test_insert_values(sqlite3 *restrict db) {
  int ret = dbm_query2(db, NULL, NULL, "INSERT INTO " TABLE_NAME " VALUES (?, ?)", 2, "1", "hello");
  if (ret != SQLITE_OK) { fprintf(stderr, "%s : %d\t%s\n", __func__, __LINE__, sqlite3_errstr(ret)); }

  assert(ret == SQLITE_OK);
}

static void process_row(void *restrict arg, char const *restrict col_name, char const *restrict data) {
  (void)arg;
  fprintf(stderr, "%s | %s\n", col_name, data);
}

static void test_select(sqlite3 *restrict db) {
  int ret = dbm_query2(db, process_row, NULL, "SELECT * FROM " TABLE_NAME, 0);

  assert(ret == SQLITE_OK);
}

static sqlite3 *before_all(void) {
  sqlite3 *db = dbm_open(NULL);
  assert(db);
  return db;
}

static void after_all(sqlite3 *restrict db) {
  dbm_destroy(db);
}

int main(void) {
  sqlite3 *db = before_all();

  test_create_table(db);
  test_insert_values(db);
  test_select(db);

  after_all(db);
}
