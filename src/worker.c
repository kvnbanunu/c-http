#include "../include/handler.h"
#include "../include/worker.h"
#include "../include/config.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h> // fork
#include <sys/types.h>
#include <sys/wait.h> // waitpid
#include <dlfcn.h> // dynlib

static worker_context_t w_ctx; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void worker_inner_signal_handler(int sig)
{
    if(sig == SIGTERM || sig == SIGINT)
    {
        w_ctx.exit_flag = 1;
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

static void worker_process(int worker_id)
{
    struct stat lib_stat;
    time_t lib_mtime = 0;
    setup_worker_inner_signal_handler();

    printf("Worker %d (PID %d) started\n", worker_id, getpid());

    if(stat(HANDLER_LIBRARY, &lib_stat) == 0)
    {
        lib_mtime = lib_stat.st_mtime;
    }

    while(!w_ctx.exit_flag)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        fd_set read_fds; // using select to wait for connections with timeout
        struct timeval timeout;
        int select_result;
        int client_fd;
        char client_ip[INET_ADDRSTRLEN];
        void *handler_lib = NULL;
        dyn_handle_request handler_func;
        
        FD_ZERO(&read_fds);
        FD_SET(w_ctx.fd, &read_fds);
        timeout.tv_sec = 1; // 1s timeout to check exit flag
        timeout.tv_usec = 0;

        select_result = select(w_ctx.fd + 1, &read_fds, NULL, NULL, &timeout);

        if(select_result <= 0)
        {
            if(select_result < 0 && errno != EINTR)
            {
                perror("worker_process: select\n");
            }
            continue;
        }

        client_fd = accept(w_ctx.fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_fd < 0)
        {
            if(errno == EINTR)
            {
                continue; // interrupt sig, try again
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
        handler_func = (dyn_handle_request)dlsym(handler_lib, "handle_request");
        if(!handler_func)
        {
            fprintf(stderr, "Worker %d: Failed to resolve handler function: %s\n", worker_id, dlerror());
            dlclose(handler_lib);
            close(client_fd);
            continue;
        }

        handler_func(client_fd);

        printf("Worker %d: processed client request\n", worker_id);
        
        dlclose(handler_lib);
        close(client_fd);
    }

    printf("Worker %d (PID %d) shutting down\n", worker_id, getpid());

    exit(0);
}

// listen for signals from children
static void *worker_monitor()
{
    while(1)
    {
        // wait for child process to terminate
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if(pid > 0)
        {
            // find which worker terminated
            for(int i = 0; i < WORKER_COUNT; i++)
            {
                if(w_ctx.pids[i] == pid)
                {
                    pid_t new_pid;
                    printf("Worker %d (PID %d) terminated, restarting...\n", i, pid);

                    //restart worker process
                    new_pid = fork();
                    if(new_pid == 0) // child
                    {
                        worker_process(i);
                        exit(0);
                    }
                    else if(new_pid > 0) // parent
                    {
                        w_ctx.pids[i] = new_pid;
                    }
                    else // failed
                    {
                        perror("worker_monitor: fork\n");
                    }
                    break;
                }
            }
        }
        else if(pid == 0 || (pid < 0 && errno == ECHILD)) // no children terminated, sleep
        {
            usleep(WORKER_SLEEP);
        }
        else // waitpid failed
        {
            perror("worker_monitor: waitpid\n");
            break;
        }
    }
    return NULL;
}

int worker_init(int fd)
{
    w_ctx.fd = fd;
    w_ctx.exit_flag = 0;
    w_ctx.pids = (pid_t *)malloc(WORKER_COUNT * sizeof(pid_t));
    if(!w_ctx.pids)
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
        else if(pid == 0) // child
        {
            worker_process(i);
            exit(0); // should not reach here
        }
        else // parent
        {
            w_ctx.pids[i] = pid;
        }
    }

    worker_monitor();

    return 0;
}

void worker_cleanup()
{
    time_t start;
    printf("Cleaning up workers...\n");

    for(int i = 0; i < WORKER_COUNT; i++)
    {
        if(w_ctx.pids[i] > 0)
        {
            printf("Sending SIGTERM to worker %d (PID %d)\n", i, w_ctx.pids[i]);
            kill(w_ctx.pids[i], SIGTERM);
        }
    }

    start = time(NULL); // current time
    while(time(NULL) - start < WORKER_SIGTERM_TIMEOUT)
    {
        int all_terminated = 1;

        for(int i = 0; i < WORKER_COUNT; i++)
        {
            if(w_ctx.pids[i] > 0)
            {
                int status;
                pid_t result = waitpid(w_ctx.pids[i], &status, WNOHANG);

                if(result == w_ctx.pids[i])
                {
                    printf("Worker %d (PID %d) terminated\n", i, w_ctx.pids[i]);
                    w_ctx.pids[i] = -1;
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
        usleep(WORKER_SLEEP);
    }

    // force kill remaining
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        if(w_ctx.pids[i] > 0)
        {
            printf("Force killing worker %d (PID %d)\n", i, w_ctx.pids[i]);
            kill(w_ctx.pids[i], SIGKILL);
            waitpid(w_ctx.pids[i], NULL, 0);
        }
    }

    free(w_ctx.pids);
    w_ctx.pids = NULL;
}

void worker_signal_handler(int sig)
{
    // forward signal to worker processes
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        if(w_ctx.pids[i] > 0)
        {
            kill(w_ctx.pids[i], sig);
        }
    }
}

