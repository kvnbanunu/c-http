#ifndef WORKER_H
#define WORKER_H

#include <sys/types.h>

/**
 * Initialize worker context and start worker processes
 *
 * @param fd Server socket file descriptor
 *
 * @return 0 on success, -1 on failure
 */
int worker_init(int fd);

/**
 * Clean up worker resources
 */
void worker_cleanup(void);

/**
 * Handle signal for workers
 *
 * @param sig Signal number
 */
void worker_signal_handler(int sig);

#endif    // WORKER_H
