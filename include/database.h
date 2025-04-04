#ifndef DATABASE_H
#define DATABASE_H

#include <ndbm.h>
#include <stddef.h>

#ifdef __APPLE__
typedef size_t datum_size;
    #define DPT_CAST(ptr) ((char *)(ptr))
#else
typedef int datum_size;
    #define DPT_CAST(ptr) (ptr)
#endif

/**
 * Open the database
 * @return DBM pointer or NULL on failure
 */
DBM *open_database(void);

/**
 * Store data in the database
 * @param dbm DBM pointer
 * @param key Key string
 * @param data Data string
 * @return 0 on success, -1 on failure
 */
int store_data(DBM *dbm, char *key, char *data);

/**
 * Retrieve data from the database
 * @param dbm DBM pointer
 * @param key Key string
 * @param data Buffer to store retrieved data
 * @param size Size of the buffer
 * @return 0 on success, -1 on failure
 */
int retrieve_data(DBM *dbm, char *key, char *data, size_t size);

/**
 * Close the database
 * @param dbm DBM pointer
 */
void close_database(DBM *dbm);

#endif /* DATABASE_H */
