#ifndef DATABASE_H
#define DATABASE_H

#include <ndbm.h>
#include <stddef.h>

/**
 * Initialize the database
 *
 * @param   path Path to the database file
 *
 * @return  0 on success, -1 on failure
 */
int db_init(const char *path);

/**
 * Store data in the database
 *
 * @param key   Key to store
 * @param val   Value to store
 * @param len   Length of the value
 *
 * @return      0 on success, -1 on failure
 */
int db_store(const char *key, const char *val, size_t len);

/**
 * Get data from the database
 *
 * @param key   Key to retrieve
 * @param len   Pointer to store the length of the retreived value
 *
 * @return      Pointer to the retrieved value, or NULL on failure (must free)
 */
char* db_get(const char *key, size_t *len);

/**
 * Close the database connection
 */
void db_cleanup();

#endif /* DATABASE_H */
