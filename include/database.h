#ifndef DATABASE_H
#define DATABASE_H

#include <ndbm.h>
#include <stddef.h>

/**
 * Database context structure
 */
typedef struct {
    DBM *dbm;              // NDBM database handle
    char *db_path;         // Path to the database file
} db_context_t;

/**
 * Initialize the database
 * 
 * @param ctx Pointer to database context structure
 * @param db_path Path to the database file
 * @return 0 on success, -1 on failure
 */
int db_init(db_context_t *ctx, const char *db_path);

/**
 * Store data in the database
 * 
 * @param ctx Pointer to database context
 * @param key Key to store
 * @param value Value to store
 * @param value_length Length of the value
 * @return 0 on success, -1 on failure
 */
int db_store(db_context_t *ctx, const char *key, const char *value, size_t value_length);

/**
 * Retrieve data from the database
 * 
 * @param ctx Pointer to database context
 * @param key Key to retrieve
 * @param value_length Pointer to store the length of the retrieved value
 * @return Pointer to the retrieved value, or NULL on failure (caller must free)
 */
char* db_retrieve(db_context_t *ctx, const char *key, size_t *value_length);

/**
 * Get first key in the database
 * 
 * @param ctx Pointer to database context
 * @param key_length Pointer to store the length of the key
 * @return Pointer to the key, or NULL if no keys (caller must free)
 */
char* db_first_key(db_context_t *ctx, size_t *key_length);

/**
 * Get next key in the database
 * 
 * @param ctx Pointer to database context
 * @param key_length Pointer to store the length of the key
 * @return Pointer to the key, or NULL if no more keys (caller must free)
 */
char* db_next_key(db_context_t *ctx, size_t *key_length);

/**
 * Close the database
 * 
 * @param ctx Pointer to database context
 */
void db_cleanup(db_context_t *ctx);

#endif /* DATABASE_H */
