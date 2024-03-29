#pragma once

#include <stddef.h>
#include "ascii_str.h"
#include "hash_table.h"
#include "sqlite3.h"

/**
 * @brief opens and return a data base object
 *
 * @param[in] db_name - the name of the data base. if `NULL` is provided - the db object will be placed in memory
 * @return `sqlite3*` - a db object on success, `NULL` otherwise
 */
sqlite3 *dbm_open(char const *restrict db_name);

/**
 * @brief destroys a data base object
 *
 * @param[in] db - the data base object
 */
void dbm_destroy(sqlite3 *restrict db);

/**
 * @brief prepares a query
 *
 * @param[in] db - the data base object
 * @param[in] query - the SQL query
 * @return `sqlite3_stmt*` - a statement object on success, `NULL` otherwise
 */
sqlite3_stmt *dbm_statement_prepare(sqlite3 *restrict db, struct ascii_str *restrict query);

/**
 * @brief destroys a statement object
 *
 * @param[in] statement - the statement object to destroy
 */
void dbm_statement_destroy(sqlite3_stmt *restrict statement);

/**
 * @brief binds parameters and executes a query
 * the function accpet `args_count` args of type `char const *` to bind to the statement. one should make sure they're
 * passing the exact number of args to match the statement's placeholders (?).
 *
 * the function doesn't support indexed / named parameters.
 *
 * for example, for the query: `SELECT * FROM foo WHERE id == ?` one should pass 1 argument of type `char const *` to
 * bind to said statement
 *
 * the function resets the statement and clears the bindings when its done, allowing one to use the same statement
 * multiple times
 *
 * @param[in] statement - the statement to execute
 * @param[out] status - a pointer to an `int` to store the status code returned from a query. may be `NULL` in which
 * case to status code will be returned
 * @param[in] args_count - the number of arguments to bind. one should make sure `args_count` matches the amount of
 * placeholders in the query
 * @param[in] ... - a list of parameters to bind to the statement (all params must be of type `char const *`)
 * @return `struct hash_table *` - a hash table whos keys cosists of row numbers [0..n) of type `size_t` and values
 * consist of a `struct vec *` holding `struct ascii_str`s.
 * in cpp terms: `map<size_t, vector<std::string>> dbm_statement_query(...);`
 */
struct hash_table dbm_statement_query(sqlite3_stmt *restrict statement, int *restrict status, size_t args_count, ...);

/**
 * @brief same as `dbm_statement_query` execpt that this function calls `dbm_statement_prepare` follows by
 * `dbm_statement_query` and `dbm_statement_destroy`
 * @param[in] db - the database object
 * @param[out] status - a pointer to an `int` to store the status code returned from a query. may be `NULL` in which
 * case to status code will be returned
 * @param[in] query - the query to execute
 * @param[in] args_count - the number of arguments to bind. one should make sure `args_count` matches the amount of
 * placeholders in the query
 * @param[in] ... - a list of parameters to bind to the statement (all params must be of type `char const *`)
 * @return `struct hash_table *` - a hash table whos keys cosists of row numbers [0..n) of type `size_t` and values
 * consist of a `struct vec *` holding `struct ascii_str`s.
 * in cpp terms: `map<size_t, vector<std::string>> dbm_query(...);`
 */
struct hash_table dbm_query(sqlite3 *restrict db,
                            int *restrict status,
                            struct ascii_str *restrict query,
                            size_t args_count,
                            ...);

/**
 * @brief binds parameters and executes a query
 * the function accpet `args_count` args of type `char const *` to bind to the statement. one should make sure they're
 * passing the exact number of args to match the statement's placeholders (?).
 *
 * the function doesn't support indexed / named parameters.
 *
 * for example, for the query: `SELECT * FROM foo WHERE id == ?` one should pass 1 argument of type `char const *` to
 * bind to said statement
 *
 * the function resets the statement and clears the bindings when its done, allowing one to use the same statement
 * multiple times
 *
 * @param[in] statement - the statement to execute
 * @param[in] callback - a function to process the returned data. may be `NULL`. said function will be called for each
 * column in a returned row. the function accepts:
 *    @param[out] arg - an object to copy data into
 *    @param[in] col_name - the column name
 *    @param[in] col_data - the data in said column
 * @param[out] arg - the object to place data into. may be `NULL`
 * @param[in] args_count - the number of arguments to bind. one should make sure `args_count` matches the amount of
 * placeholders in the query
 * @param[in] ... - a list of parameters to bind to the statement (all params must be of type `char const *`)
 * @return `int` - `SQLITE_OK` on success, `SQLITE_*` - some error code otherwise
 */
int dbm_statement_query2(sqlite3_stmt *restrict statement,
                         void (*callback)(void *restrict arg,
                                          char const *restrict col_name,
                                          char const *restrict col_data),
                         void *restrict arg,
                         size_t args_count,
                         ...);

/**
 * @brief same as `dbm_statement_query2` execpt that this function calls `dbm_statement_prepare` follows by
 * `dbm_statement_query` and `dbm_statement_destroy`
 *
 * @param[in] db  - the database object
 * @param[in] callback - take a look at `dbm_statement_query`
 * @param[out] arg - an object to copy data into
 * @param[in] query  - the SQL query
 * @param[in] args_count - the number of arguments to bind. one should make sure `args_count` matches the amount of
 * placeholders in the query
 * @param[in] ... - a list of parameters to bind to the statement (all params must be of type `char const *`)
 * @return `int` - `SQLITE_OK` on success, `SQLITE_*` - some error code otherwise
 */
int dbm_query2(sqlite3 *restrict db,
               void (*callback)(void *restrict arg, char const *restrict col_name, char const *restrict col_data),
               void *restrict arg,
               char const *restrict query,
               size_t args_count,
               ...);
