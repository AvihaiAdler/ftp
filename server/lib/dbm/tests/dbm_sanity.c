#include <assert.h>
#include "db_manager.h"

int main(void) {
  sqlite3 *db = dbm_open(NULL);
  assert(db);

  dbm_destroy(db);
}
