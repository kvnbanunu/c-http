#ifndef WORKER_H
#define WORKER_H

#include <sys/types.h>

typedef struct
{
    pid_t *workers;
} worker_context_t;

int worker_init(int fd);
int morker_monitor();
void worker_signal_handler(int sig);
void worker_cleanup();

#endif // WORKER_H
