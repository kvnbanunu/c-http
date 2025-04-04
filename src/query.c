#include "../include/config.h"
#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Print usage information
 * @param program Program name
 */
static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s [-l] [-g key] [-a]\n", program);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -l            List all keys\n");
    fprintf(stderr, "  -g key        Get value for specified key\n");
    fprintf(stderr, "  -a            Display all key-value pairs\n");
}

/**
 * List all keys in the database
 * @param dbm DBM pointer
 * @return 0 on success, -1 on failure
 */
static int list_keys(DBM *dbm)
{
    datum key;
    int   count = 0;

    // Start at the first key
    key = dbm_firstkey(dbm);

    // Iterate through all keys
    while(key.dptr != NULL)
    {
        printf("Key: %s\n", key.dptr);
        count++;

        // Get next key
        key = dbm_nextkey(dbm);
    }

    printf("Total keys: %d\n", count);
    return 0;
}

/**
 * Get value for a specified key
 * @param dbm DBM pointer
 * @param key_str Key string
 * @return 0 on success, -1 on failure
 */
static int get_value(DBM *dbm, const char *key_str)
{
    datum key;
    datum value;

    key.dptr  = (char *)key_str;
    key.dsize = (datum_size)strlen(key_str) + 1;

    value = dbm_fetch(dbm, key);
    if(value.dptr == NULL)
    {
        fprintf(stderr, "Key not found: %s\n", key_str);
        return -1;
    }

    // Print key-value pair
    printf("Key: %s\n", key_str);
    printf("Value: %s\n", value.dptr);

    return 0;
}

/**
 * Display all key-value pairs
 * @param dbm DBM pointer
 * @return 0 on success, -1 on failure
 */
static int display_all(DBM *dbm)
{
    datum key, value;
    int   count = 0;

    key = dbm_firstkey(dbm);

    // Iterate through all keys
    while(key.dptr != NULL)
    {
        value = dbm_fetch(dbm, key);
        if(value.dptr != NULL)
        {
            // Print key-value pair
            printf("Key: %s\n", key.dptr);
            printf("Value: %s\n", value.dptr);
            printf("---\n");
            count++;
        }

        // Get next key
        key = dbm_nextkey(dbm);
    }

    printf("Total entries: %d\n", count);
    return 0;
}

int main(int argc, char *argv[])
{
    int   list_flag = 0;
    int   get_flag  = 0;
    int   all_flag  = 0;
    char *get_key   = NULL;
    DBM  *dbm;
    int   opt;

    while((opt = getopt(argc, argv, "lg:a")) != -1)
    {
        switch(opt)
        {
            case 'l':
                list_flag = 1;
                break;
            case 'g':
                get_flag = 1;
                get_key  = optarg;
                break;
            case 'a':
                all_flag = 1;
                break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Check if at least one operation is specified
    if(!list_flag && !get_flag && !all_flag)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Open database
    dbm = open_database();
    if(!dbm)
    {
        fprintf(stderr, "Failed to open database: %s\n", DB_PATH);
        return EXIT_FAILURE;
    }

    if(list_flag)
    {
        printf("=== Listing all keys ===\n");
        list_keys(dbm);
    }

    if(get_flag && get_key)
    {
        printf("=== Getting value for key: %s ===\n", get_key);
        get_value(dbm, get_key);
    }

    if(all_flag)
    {
        printf("=== Displaying all key-value pairs ===\n");
        display_all(dbm);
    }

    close_database(dbm);

    return EXIT_SUCCESS;
}
