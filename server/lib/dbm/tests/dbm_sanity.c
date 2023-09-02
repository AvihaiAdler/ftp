#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "db_manager.h"

#define TABLE_NAME "dummy_table"
#define SIZE 128
#define STRINGIFY_SIZE(SIZE) "(" #SIZE ")"
#define STRINGIFY(TYPE, SIZE) #TYPE STRINGIFY_SIZE(SIZE)

#define QUERY_CREATE \
  "CREATE TABLE IF NOT EXISTS " TABLE_NAME " (id " STRINGIFY(VARCHAR, SIZE) ", data " STRINGIFY(VARCHAR, SIZE) ")"
#define QUERY_INSERT "INSERT INTO " TABLE_NAME " VALUES (?, ?)"
#define QUERY_SELECT_ALL "SELECT * FROM " TABLE_NAME

struct tuple {
  char const *a;
  char const *b;
};

static void test_create_table(sqlite3 *restrict db) {
  int ret = 0;
  struct ascii_str query = ascii_str_create(QUERY_CREATE, STR_C_STR);

  struct hash_table ht = dbm_query(db, &ret, &query, 0);

  assert(ret == SQLITE_OK);
  assert(table_empty(&ht));

  ascii_str_destroy(&query);
  table_destroy(&ht);
}

static void test_insert_values(sqlite3 *restrict db, char const *restrict col_name, char const *restrict col_data) {
  int ret = 0;
  struct ascii_str query = ascii_str_create(QUERY_INSERT, STR_C_STR);

  struct hash_table ht = dbm_query(db, &ret, &query, 2, col_name, col_data);

  assert(ret == SQLITE_OK);
  assert(table_empty(&ht));

  ascii_str_destroy(&query);
  table_destroy(&ht);
}

static void test_select_all(sqlite3 *restrict db, char const *restrict data) {
  int ret = 0;
  struct ascii_str query = ascii_str_create(QUERY_SELECT_ALL, STR_C_STR);

  struct hash_table ht = dbm_query(db, &ret, &query, 0);

  assert(ret == SQLITE_OK);
  assert(!table_empty(&ht));

  size_t row = 0;
  struct vec row_data = {0};

  enum ds_error get_ret = table_get(&ht, &row, &row_data);
  assert(get_ret == DS_VALUE_OK);
  assert(!vec_empty(&row_data));

  assert(strcmp(data, ascii_str_c_str((struct ascii_str *)vec_pop(&row_data))) == 0);

  ascii_str_destroy(&query);
  table_destroy(&ht);
}

static void test_create_table2(sqlite3 *restrict db) {
  int ret = dbm_query2(db, NULL, NULL, QUERY_CREATE, 0);

  if (ret != SQLITE_OK) { fprintf(stderr, "%s : %d\t%s\n", __func__, __LINE__, sqlite3_errstr(ret)); }

  assert(ret == SQLITE_OK);
}

static void test_insert_values2(sqlite3 *restrict db, char const *col_name, char const *col_data) {
  int ret = dbm_query2(db, NULL, NULL, QUERY_INSERT, 2, col_name, col_data);
  if (ret != SQLITE_OK) { fprintf(stderr, "%s : %d\t%s\n", __func__, __LINE__, sqlite3_errstr(ret)); }

  assert(ret == SQLITE_OK);
}

static void process_row(void *restrict arg, char const *restrict col_name, char const *restrict data) {
  struct tuple *const tup = arg;

  assert(strcmp(col_name, tup->a) == 0);
  assert(strcmp(data, tup->b) == 0);
  fprintf(stderr, "%s | %s\n", col_name, data);
}

static void test_select_all2(sqlite3 *restrict db, char const *restrict col_name, char const *restrict col_data) {
  struct tuple tup = {.a = col_name, .b = col_data};

  int ret = dbm_query2(db, process_row, &tup, QUERY_SELECT_ALL, 0);

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

  test_create_table2(db);
  test_insert_values2(db, "1", "hello");
  test_select_all2(db, "1", "hello");

  after_all(db);

  db = before_all();

  test_create_table(db);
  test_insert_values(db, "1", "hello");
  test_select_all(db, "hello");

  after_all(db);
}
