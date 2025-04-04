#include "../include/config.h"
#include "../include/server.h"
#include "../include/utils.h"
#include "../include/worker.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void signal_handler(int sig);
static void setup_signal_handler(void);

int main(void)
{
    int       fd;
    worker_t *workers;
    int       retval = EXIT_FAILURE;

    setup_signal_handler();

    /* Initialize server */
    fd = init_server();
    if(fd < 0)
    {
        fprintf(stderr, "Failed to initialize server\n");
        goto done;
    }

    // Allocate workers array
    workers = (worker_t *)calloc((size_t)WORKER_COUNT, sizeof(worker_t));
    if(!workers)
    {
        perror("main: calloc\n");
        goto cleanup_server;
    }

    // Initialize worker processes
    if(init_workers(workers, fd) < 0)
    {
        fprintf(stderr, "Failed to initialize worker processes\n");
        goto cleanup_workers;
    }

    log_message("Server started on port %d with %d workers", PORT, WORKER_COUNT);

    // Monitor workers while server is running
    while(running)
    {
        if(monitor_workers(workers, fd) < 0)
        {
            fprintf(stderr, "Failed to monitor worker processes\n");
            goto cleanup_workers;
        }
        sleep(1);
    }

    log_message("Server shutting down...");
    retval = EXIT_SUCCESS;

cleanup_workers:
    cleanup_workers(workers);
    free(workers);

cleanup_server:
    cleanup_server(fd);

done:
    return retval;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void signal_handler(int sig)
{
    running = 0;
}

#pragma GCC diagnostic pop

static void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = signal_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
