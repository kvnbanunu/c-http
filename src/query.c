#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -d, --database FILE    Path to database file (required)\n");
    printf("  -l, --list             List all post IDs\n");
    printf("  -i, --id ID            Show data for specific post ID\n");
    printf("  -h, --help             Display this help message\n");
}

// List all post IDs in the database
void list_post_ids(db_context_t *db_ctx) {
    printf("Available Post IDs:\n");
    
    size_t key_length;
    int found = 0;
    
    // Get first key
    char *key_str = db_first_key(db_ctx, &key_length);
    
    // Iterate over all keys
    while (key_str != NULL) {
        // Ensure the key is null-terminated
        key_str[key_length] = '\0';
        
        // Check if it's a keys list (ends with .keys)
        if (strstr(key_str, ".keys") != NULL) {
            char *id = strtok(key_str, ".");
            printf("- %s\n", id);
            found = 1;
        }
        
        free(key_str);
        
        // Get next key
        key_str = db_next_key(db_ctx, &key_length);
    }
    
    if (!found) {
        printf("No posts found in the database.\n");
    }
}

// Show data for a specific post ID
void show_post_data(db_context_t *db_ctx, const char *post_id) {
    char keys_key[256];
    sprintf(keys_key, "%s.keys", post_id);
    
    size_t content_length;
    char *keys_list = db_retrieve(db_ctx, keys_key, &content_length);
    
    if (!keys_list) {
        printf("Post ID '%s' not found in the database.\n", post_id);
        return;
    }
    
    // Ensure the keys list is null-terminated
    keys_list[content_length] = '\0';
    
    printf("Data for Post ID: %s\n", post_id);
    printf("-------------------------\n");
    
    // Split the comma-separated list of keys
    char *key_name = strtok(keys_list, ",");
    while (key_name != NULL) {
        char db_key[256];
        sprintf(db_key, "%s.%s", post_id, key_name);
        
        size_t value_length;
        char *value = db_retrieve(db_ctx, db_key, &value_length);
        
        if (value != NULL) {
            // Ensure the value is null-terminated
            value[value_length] = '\0';
            printf("%-20s: %s\n", key_name, value);
            free(value);
        } else {
            printf("%-20s: <not found>\n", key_name);
        }
        
        key_name = strtok(NULL, ",");
    }
    
    free(keys_list);
}

int main(int argc, char *argv[]) {
    char *database_path = NULL;
    char *post_id = NULL;
    int list_posts = 0;
    
    // Parse command-line options
    static struct option long_options[] = {
        {"database", required_argument, 0, 'd'},
        {"list", no_argument, 0, 'l'},
        {"id", required_argument, 0, 'i'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "d:li:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'd':
                database_path = optarg;
                break;
            
            case 'l':
                list_posts = 1;
                break;
            
            case 'i':
                post_id = optarg;
                break;
            
            case 'h':
                print_usage(argv[0]);
                return 0;
            
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Check if database path is provided
    if (database_path == NULL) {
        fprintf(stderr, "Error: Database path is required.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Check if an operation is specified
    if (!list_posts && post_id == NULL) {
        fprintf(stderr, "Error: No operation specified.\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Initialize database context
    db_context_t db_ctx;
    memset(&db_ctx, 0, sizeof(db_ctx));
    
    // Open database
    if (db_init(&db_ctx, database_path) < 0) {
        perror("Failed to open database");
        return 1;
    }
    
    // Perform requested operation
    if (list_posts) {
        list_post_ids(&db_ctx);
    }
    
    if (post_id != NULL) {
        show_post_data(&db_ctx, post_id);
    }
    
    // Close database
    db_cleanup(&db_ctx);
    
    return 0;
}
