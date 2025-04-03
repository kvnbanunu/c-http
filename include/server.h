#ifndef SERVER_H
#define SERVER_H

/**
 * Server context structure
 */
typedef struct {
    int server_socket;            // Server socket file descriptor
    int port;                     // Port number
    int exit_flag;                // Flag indicating server should exit
} server_context_t;

/**
 * Initialize the server
 * 
 * @param ctx Pointer to server context
 * @param port Port number to listen on
 * @return 0 on success, -1 on failure
 */
int server_init(server_context_t *ctx, int port);

/**
 * Run the server
 * 
 * @param ctx Pointer to server context
 * @param worker_count Number of worker processes to create
 * @param document_root Path to document root directory
 * @param handler_lib Path to handler shared library
 * @param database_path Path to database file
 * @return 0 on success, non-zero on failure
 */
int server_run(server_context_t *ctx, int worker_count, const char *document_root, 
              const char *handler_lib, const char *database_path);

/**
 * Clean up server resources
 * 
 * @param ctx Pointer to server context
 */
void server_cleanup(server_context_t *ctx);

/**
 * Handle signals for the server
 * 
 * @param ctx Pointer to server context
 * @param signo Signal number
 */
void server_handle_signal(server_context_t *ctx, int signo);

#endif /* SERVER_H */
