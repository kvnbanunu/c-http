#include "../include/worker.h"
#include "../include/config.h"
#include <arpa/inet.h>
#include <dlfcn.h>    // dynlib
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>    // waitpid
#include <time.h>
#include <unistd.h>    // fork

// deconstructed the worker_context into these, might revert
static int                   server_fd   = -1;      // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static pid_t                *worker_pids = NULL;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t exit_flag   = 0;       // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void worker_inner_signal_handler(int sig)
{
    if(sig == SIGTERM || sig == SIGINT)
    {
        exit_flag = 1;
    }
}

static void setup_worker_inner_signal_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = worker_inner_signal_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

_Noreturn static void worker_process(int worker_id)
{
    struct stat lib_stat;
    time_t      lib_mtime = 0;
    setup_worker_inner_signal_handler();

    printf("Worker %d (PID %d) started\n", worker_id, getpid());

    if(stat(HANDLER_LIBRARY, &lib_stat) == 0)
    {
        lib_mtime = lib_stat.st_mtime;
    }

    while(!exit_flag)
    {
        struct sockaddr_in client_addr;
        socklen_t          client_addr_len = sizeof(client_addr);
        fd_set             read_fds;    // using select to wait for connections with timeout
        struct timeval     timeout;
        int                select_result;
        int                client_fd;
        char               client_ip[INET_ADDRSTRLEN];
        void              *handler_lib      = NULL;
        void (*handler_func)(int client_fd) = NULL;
        void (*init_handler_func)(void)     = NULL;

        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        timeout.tv_sec  = 1;    // 1s timeout to check exit flag
        timeout.tv_usec = 0;

        select_result = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);

        if(select_result <= 0)
        {
            if(select_result < 0 && errno != EINTR)
            {
                perror("worker_process: select\n");
            }
            continue;
        }

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_fd < 0)
        {
            if(errno == EINTR)
            {
                continue;    // interrupt sig, try again
            }
            perror("worker_process: accept\n");
            continue;
        }

        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Worker %d: Accepted connection from %s:%d\n", worker_id, client_ip, ntohs(client_addr.sin_port));

        // check handler lib for updates
        if(stat(HANDLER_LIBRARY, &lib_stat) == 0)
        {
            if(lib_stat.st_mtime > lib_mtime)
            {
                lib_mtime = lib_stat.st_mtime;
                printf("Worker %d: Detected updated handler library\n", worker_id);
            }
        }

        // load handler lib
        handler_lib = dlopen(HANDLER_LIBRARY, RTLD_NOW);
        if(!handler_lib)
        {
            fprintf(stderr, "Worker %d: Failed to load handler library: %s\n", worker_id, dlerror());
            close(client_fd);
            continue;
        }

        // link handler func
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        init_handler_func = (void (*)(void))dlsym(handler_lib, "init_handler");
        handler_func      = (void (*)(int))dlsym(handler_lib, "handle_request");
#pragma GCC diagnostic pop
        if(!handler_func)
        {
            fprintf(stderr, "Worker %d: Failed to resolve handler function: %s\n", worker_id, dlerror());
            dlclose(handler_lib);
            close(client_fd);
            continue;
        }

        init_handler_func();

        handler_func(client_fd);

        printf("Worker %d: processed client request\n", worker_id);

        dlclose(handler_lib);
        close(client_fd);
    }

    printf("Worker %d (PID %d) shutting down\n", worker_id, getpid());

    exit(0);
}

// listen for signals from children
static void *worker_monitor(void)
{
    while(1)
    {
        // wait for child process to terminate
        int   status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if(pid > 0)
        {
            // find which worker terminated
            for(int i = 0; i < WORKER_COUNT; i++)
            {
                if(worker_pids[i] == pid)
                {
                    pid_t new_pid;
                    printf("Worker %d (PID %d) terminated, restarting...\n", i, pid);

                    // restart worker process
                    new_pid = fork();
                    if(new_pid == 0)    // child
                    {
                        worker_process(i);
                        // noreturn
                    }
                    else if(new_pid > 0)    // parent
                    {
                        worker_pids[i] = new_pid;
                    }
                    else    // failed
                    {
                        perror("worker_monitor: fork\n");
                    }
                    break;
                }
            }
        }
        // cppcheck-suppress knownConditionTrueFalse
        else if(pid == 0 || (pid < 0 && errno == ECHILD))
        {
            // no children terminated, sleep
            struct timespec t = {0, WORKER_SLEEP};
            nanosleep(&t, NULL);
        }
        else    // waitpid failed
        {
            perror("worker_monitor: waitpid\n");
            break;
        }
    }
    return NULL;
}

int worker_init(int fd)
{
    server_fd   = fd;
    worker_pids = (pid_t *)malloc(WORKER_COUNT * sizeof(pid_t));
    if(!worker_pids)
    {
        perror("worker_init: malloc\n");
        return -1;
    }

    // fork worker processes
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        pid_t pid = fork();
        if(pid < 0)
        {
            perror("worker_init: fork\n");
            return -1;
        }
        if(pid == 0)    // child
        {
            worker_process(i);
            // no return
        }
        // parent
        worker_pids[i] = pid;
    }

    worker_monitor();

    return 0;
}

void worker_cleanup(void)
{
    time_t start;
    printf("Cleaning up workers...\n");

    for(int i = 0; i < WORKER_COUNT; i++)
    {
        if(worker_pids[i] > 0)
        {
            printf("Sending SIGTERM to worker %d (PID %d)\n", i, worker_pids[i]);
            kill(worker_pids[i], SIGTERM);
        }
    }

    start = time(NULL);    // current time
    while(time(NULL) - start < WORKER_SIGTERM_TIMEOUT)
    {
        int             all_terminated = 1;
        struct timespec t              = {0, WORKER_SLEEP};

        for(int i = 0; i < WORKER_COUNT; i++)
        {
            if(worker_pids[i] > 0)
            {
                int   status;
                pid_t result = waitpid(worker_pids[i], &status, WNOHANG);

                if(result == worker_pids[i])
                {
                    printf("Worker %d (PID %d) terminated\n", i, worker_pids[i]);
                    worker_pids[i] = -1;
                }
                else if(result == 0)
                {
                    all_terminated = 0;
                }
            }
        }

        if(all_terminated)
        {
            break;
        }

        nanosleep(&t, NULL);
    }

    // force kill remaining
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        if(worker_pids[i] > 0)
        {
            printf("Force killing worker %d (PID %d)\n", i, worker_pids[i]);
            kill(worker_pids[i], SIGKILL);
            waitpid(worker_pids[i], NULL, 0);
        }
    }

    free(worker_pids);
    worker_pids = NULL;
}

void worker_signal_handler(int sig)
{
    // forward signal to worker processes
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        if(worker_pids[i] > 0)
        {
            kill(worker_pids[i], sig);
        }
    }
}
