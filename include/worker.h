#ifndef WORKER_H
#define WORKER_H

/**
 * Initialize worker processes
 *
 * @param fd            Socket file descriptor
 * @param worker_count  Number of worker processes to create
 * @param doc_root      Path to document root directory
 * @param handler_lib   Path to handler shared library
 * @param db_path       Path to database file
 *
 * @return              0 on success, -1 on failure
 */
int worker_init(int fd, int worker_count, const char *doc_root, const char *handler_lib, const char *db_path);

/** Cleanup worker processes */
void worker_cleanup();

/**
 * Handle signal for worker process
 *
 * @param sig Signal number
 */
void worker_handle_signal(int sig);

#endif /* WORKER_H */
