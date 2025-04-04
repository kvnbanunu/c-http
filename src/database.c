#include "../include/database.h"
#include "../include/config.h"
#include "../include/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

DBM *open_database(void)
{
    DBM *dbm = dbm_open(DB_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(!dbm)
    {
        log_message("Failed to open database: %s", DB_PATH);
        perror("open_database: dbm_open\n");
        return NULL;
    }

    return dbm;
}

int store_data(DBM *dbm, char *key, char *data)
{
    datum key_datum;
    datum data_datum;
    int   result;

    if(!dbm || !key || !data)
    {
        return -1;
    }

    key_datum.dptr   = key;
    key_datum.dsize  = (datum_size)strlen(key) + 1;
    data_datum.dptr  = data;
    data_datum.dsize = (datum_size)strlen(data) + 1;

    result = dbm_store(dbm, key_datum, data_datum, DBM_REPLACE);
    if(result < 0)
    {
        log_message("Failed to store data for key: %s", key);
        perror("store_data: dbm_store\n");
        return -1;
    }

    return 0;
}

int retrieve_data(DBM *dbm, char *key, char *data, size_t size)
{
    datum key_datum;
    datum data_datum;
    datum temp_datum;

    if(!dbm || !key || !data || size == 0)
    {
        return -1;
    }

    key_datum.dptr  = key;
    key_datum.dsize = (datum_size)strlen(key) + 1;

    // safe fetch
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggregate-return"
    temp_datum = dbm_fetch(dbm, key_datum);
#pragma GCC diagnostic pop
    if(!temp_datum.dptr)
    {
        // Key not found
        return -1;
    }

    data_datum.dptr  = temp_datum.dptr;
    data_datum.dsize = temp_datum.dsize;

    // Copy data to output buffer
    if((size_t)data_datum.dsize <= size)
    {
        memcpy(data, data_datum.dptr, (size_t)data_datum.dsize);
    }
    else
    {
        memcpy(data, data_datum.dptr, size - 1);
        data[size - 1] = '\0';
    }

    return 0;
}

void close_database(DBM *dbm)
{
    if(dbm)
    {
        dbm_close(dbm);
    }
}
