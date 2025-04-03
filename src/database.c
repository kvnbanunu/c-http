#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

static DBM *db = NULL;

int db_init(const char *path)
{
    if(db != NULL)
    {
        return 0; // already initialized
    }

    db = dbm_open(path, O_RDWR | O_CREAT, 0664);
    if(db == NULL)
    {
        perror("db_init: dbm_open\n");
        return -1;
    }
    return 0;
}

int db_store(const char *key, const char *val, size_t len)
{
    datum key_datum;
    datum val_datum;

    if(db == NULL)
    {
        return -1;
    }
    
    key_datum.dptr = (void*)key;
    key_datum.dsize = strlen(key);
    val_datum.dptr = (void*)val;
    val_datum.dsize = len;

    return dbm_store(db, key_datum, val_datum, DBM_REPLACE);
}

char* db_get(const char *key, size_t *len)
{
    datum key_datum;
    datum val_datum;
    char *result;

    if(db == NULL)
    {
        return NULL;
    }
    
    key_datum.dptr = (void*)key;
    key_datum.dsize = strlen(key);
    val_datum = dbm_fetch(db, key_datum);
    if(val_datum.dptr == NULL)
    {
        return NULL;
    }

    *len = val_datum.dsize;
    result = (char *)malloc(val_datum.dsize);
    if(result == NULL)
    {
        return NULL;
    }

    memcpy(result, val_datum.dptr, val_datum.dsize);
    return result;
}

void db_cleanup()
{
    if(db != NULL)
    {
        dbm_close(db);
        db = NULL;
    }
}
