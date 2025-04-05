#include <stdio.h>
#include <ndbm.h>
#include <fcntl.h>
#include <string.h>

int main() {
    DBM *db;
    datum key, value;

    // Open the database in read-only mode
    db = dbm_open("posts_db", O_RDONLY, 0);
    if (!db) {
        perror("dbm_open");
        return 1;
    }

    // Get the first key
    key = dbm_firstkey(db);

    // Iterate through all keys
    while (key.dptr != NULL) {
        // Fetch the corresponding value
        value = dbm_fetch(db, key);

        // Print key-value pair
        if (value.dptr) {
            printf("Key: %s, Value: %s\n", key.dptr, value.dptr);
        }

        // Move to the next key
        key = dbm_nextkey(db);
    }

    // Close the database
    dbm_close(db);

    return 0;
}
