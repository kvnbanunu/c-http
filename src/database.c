#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int db_init(db_context_t *ctx, const char *db_path)
{
    if (!ctx)
    {
        return -1;
    }

    // If already initialized, clean up first
    if (ctx->dbm != NULL)
    {
        db_cleanup(ctx);
    }
    
    // Store the database path
    ctx->db_path = strdup(db_path);
    if (!ctx->db_path)
    {
        perror("strdup failed");
        return -1;
    }
    
    // Open the database
    ctx->dbm = dbm_open(db_path, O_RDWR | O_CREAT, 0644);
    if (ctx->dbm == NULL)
    {
        perror("dbm_open failed");
        free(ctx->db_path);
        ctx->db_path = NULL;
        return -1;
    }
    
    return 0;
}

int db_store(db_context_t *ctx, const char *key, const char *value, size_t value_length)
{
    datum key_datum;
    datum value_datum;

    if (!ctx || !ctx->dbm)
    {
        return -1;
    }
    
    key_datum.dptr = (void*)key;
    key_datum.dsize = strlen(key);
    
    value_datum.dptr = (void*)value;
    value_datum.dsize = value_length;
    
    return dbm_store(ctx->dbm, key_datum, value_datum, DBM_REPLACE);
}

char* db_retrieve(db_context_t *ctx, const char *key, size_t *value_length)
{
    datum key_datum;
    datum value_datum;
    char *result;

    if (!ctx || !ctx->dbm)
    {
        return NULL;
    }
    
    key_datum.dptr = (void*)key;
    key_datum.dsize = strlen(key);
    
    value_datum = dbm_fetch(ctx->dbm, key_datum);
    if (value_datum.dptr == NULL)
    {
        return NULL;
    }
    
    *value_length = value_datum.dsize;
    result = malloc(value_datum.dsize);
    if (result == NULL)
    {
        return NULL;
    }
    
    memcpy(result, value_datum.dptr, value_datum.dsize);
    return result;
}

char* db_first_key(db_context_t *ctx, size_t *key_length)
{
    datum key_datum;
    char *result;

    if (!ctx || !ctx->dbm)
    {
        return NULL;
    }
    
    key_datum = dbm_firstkey(ctx->dbm);
    if (key_datum.dptr == NULL)
    {
        return NULL;
    }
    
    *key_length = key_datum.dsize;
    result = malloc(key_datum.dsize);
    if (result == NULL)
    {
        return NULL;
    }
    
    memcpy(result, key_datum.dptr, key_datum.dsize);
    return result;
}

char* db_next_key(db_context_t *ctx, size_t *key_length)
{
    datum key_datum;
    char *result;

    if (!ctx || !ctx->dbm)
    {
        return NULL;
    }
    
    key_datum = dbm_nextkey(ctx->dbm);
    if (key_datum.dptr == NULL)
    {
        return NULL;
    }
    
    *key_length = key_datum.dsize;
    result = malloc(key_datum.dsize);
    if (result == NULL)
    {
        return NULL;
    }
    
    memcpy(result, key_datum.dptr, key_datum.dsize);
    return result;
}

void db_cleanup(db_context_t *ctx)
{
    if (!ctx)
    {
        return;
    }
    
    if (ctx->dbm != NULL)
    {
        dbm_close(ctx->dbm);
        ctx->dbm = NULL;
    }
    
    if (ctx->db_path != NULL)
    {
        free(ctx->db_path);
        ctx->db_path = NULL;
    }
}
