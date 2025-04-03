#ifndef WORKER_H
#define WORKER_H

#include <sys/types.h>

/**
 * Worker context structure
 */
typedef struct {
    pid_t *worker_pids;           // Array of worker process IDs
    int worker_count;             // Number of worker processes
    int server_socket;            // Server socket file descriptor
    char *document_root;          // Path to document root directory
    char *handler_lib_path;       // Path to handler shared library
    char *database_path;          // Path to database file
    int exit_flag;                // Flag indicating workers should exit
} worker_context_t;

/**
 * Initialize worker processes
 * 
 * @param ctx Pointer to worker context
 * @param server_socket Server socket file descriptor
 * @param worker_count Number of worker processes to create
 * @param document_root Path to document root directory
 * @param handler_lib Path to handler shared library
 * @param database_path Path to database file
 * @return 0 on success, -1 on failure
 */
int worker_init(worker_context_t *ctx, int server_socket, int worker_count, 
               const char *document_root, const char *handler_lib, const char *database_path);

/**
 * Cleanup worker processes
 * 
 * @param ctx Pointer to worker context
 */
void worker_cleanup(worker_context_t *ctx);

/**
 * Handle signals for worker processes
 * 
 * @param ctx Pointer to worker context
 * @param signo Signal number
 */
void worker_handle_signal(worker_context_t *ctx, int signo);

#endif /* WORKER_H */
