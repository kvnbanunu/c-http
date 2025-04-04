#ifndef WORKER_H
#define WORKER_H

#include <sys/types.h>

/**
 * Worker process structure
 */
typedef struct
{
    pid_t pid;
    int   pipe_fd[2]; /* Communication pipe with parent */
} worker_t;

/**
 * Initialize worker processes
 * @param workers Array of worker_t structures
 * @param fd Server socket file descriptor
 * @return 0 on success, -1 on failure
 */
int init_workers(worker_t *workers, int fd);

/**
 * Worker process main function
 * @param fd Server socket file descriptor
 * @param pipe_fd Communication pipe file descriptor
 * @return Never returns on success, -1 on failure
 */
int worker_main(int fd, int pipe_fd);

/**
 * Monitor worker processes and restart them if needed
 * @param workers Array of worker_t structures
 * @param fd Server socket file descriptor
 * @return 0 on success, -1 on failure
 */
int monitor_workers(worker_t *workers, int fd);

/**
 * Clean up worker processes
 * @param workers Array of worker_t structures
 */
void cleanup_workers(worker_t *workers);

#endif /* WORKER_H */
