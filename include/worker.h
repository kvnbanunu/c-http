#ifndef WORKER_H
#define WORKER_H

#include <sys/types.h>

typedef struct
{
    int fd;
    pid_t *pids;
    int exit_flag;
} worker_context_t;

int worker_init(int fd);
void worker_cleanup();
void worker_signal_handler(int sig);

#endif // WORKER_H
