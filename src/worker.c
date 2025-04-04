#include "../include/worker.h"
#include "../include/config.h"
#include "../include/http_handler.h"
#include "../include/utils.h"
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Function pointer type for the HTTP handler */
typedef int (*http_handler_func)(const http_request_t *, http_response_t *);

/* Structure to hold shared library information */
typedef struct
{
    void             *handle;
    http_handler_func func;
    time_t            last_modified;
} http_lib_t;

/**
 * Load the HTTP handler shared library
 * @return Populated http_lib_t structure or NULL on failure
 */
static http_lib_t *load_http_lib(void)
{
    http_lib_t *lib;
    struct stat st;

    // Allocate library structure
    lib = (http_lib_t *)malloc(sizeof(http_lib_t));
    if(!lib)
    {
        perror("http_lib_t: malloc\n");
        return NULL;
    }

    memset(lib, 0, sizeof(http_lib_t));

    // Get library file modification time
    if(stat(HTTP_HANDLER_LIB, &st) < 0)
    {
        perror("http_lib_t: stat\n");
        free(lib);
        return NULL;
    }

    lib->last_modified = st.st_mtime;

    // Open the shared library
    lib->handle = dlopen(HTTP_HANDLER_LIB, RTLD_NOW);
    if(!lib->handle)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        free(lib);
        return NULL;
    }

    // Get the handler function
    *(void **)(&lib->func) = dlsym(lib->handle, HTTP_HANDLER_FUNC);
    if(!lib->func)
    {
        fprintf(stderr, "dlsym error: %s\n", dlerror());
        dlclose(lib->handle);
        free(lib);
        return NULL;
    }

    return lib;
}

/**
 * Check if the shared library has been updated
 * @param lib Pointer to http_lib_t structure
 * @return 1 if updated, 0 if not, -1 on error
 */
static int check_lib_update(http_lib_t *lib)
{
    struct stat st;

    if(stat(HTTP_HANDLER_LIB, &st) < 0)
    {
        perror("stat");
        return -1;
    }

    return st.st_mtime > lib->last_modified ? 1 : 0;
}

/**
 * Reload the HTTP handler shared library
 * @param lib Pointer to http_lib_t structure
 * @return 0 on success, -1 on failure
 */
static int reload_http_lib(http_lib_t *lib)
{
    void             *new_handle;
    http_handler_func new_func;
    struct stat       st;

    if(stat(HTTP_HANDLER_LIB, &st) < 0)
    {
        perror("stat");
        return -1;
    }

    // Open the shared lib
    new_handle = dlopen(HTTP_HANDLER_LIB, RTLD_NOW);
    if(!new_handle)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return -1;
    }

    // Get the new handler func
    *(void **)(&new_func) = dlsym(new_handle, HTTP_HANDLER_FUNC);
    if(!new_func)
    {
        fprintf(stderr, "dlsym error: %s\n", dlerror());
        dlclose(new_handle);
        return -1;
    }

    // Close the old library
    if(lib->handle)
    {
        dlclose(lib->handle);
    }

    // Update the library struct
    lib->handle        = new_handle;
    lib->func          = new_func;
    lib->last_modified = st.st_mtime;

    log_message("Reloaded HTTP handler library: %s", HTTP_HANDLER_LIB);
    return 0;
}

/**
 * Free resources allocated for the HTTP library
 * @param lib Pointer to http_lib_t structure
 */
static void free_http_lib(http_lib_t *lib)
{
    if(!lib)
    {
        return;
    }

    if(lib->handle)
    {
        dlclose(lib->handle);
    }

    free(lib);
}

/**
 * Handle client connection
 * @param client_fd Client socket file descriptor
 * @param lib HTTP library structure
 * @return 0 on success, -1 on failure
 */
static int handle_client(int client_fd, http_lib_t *lib)
{
    char            buffer[MAX_REQUEST_SIZE];
    char            status_line[64];
    const char     *status_text;
    ssize_t         bytes_read;
    http_request_t  request;
    http_response_t response;

    // Read request
    bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(bytes_read <= 0)
    {
        if(bytes_read < 0)
        {
            perror("recv");
        }
        return -1;
    }

    buffer[bytes_read] = '\0';

    // Parse request
    memset(&request, 0, sizeof(request));
    if(parse_http_request(buffer, (size_t)bytes_read, &request) < 0)
    {
        // Send bad request response
        const char *response_str = "HTTP/1.1 400 Bad Request\r\n"
                                   "Content-Length: 0\r\n"
                                   "Connection: close\r\n\r\n";
        send(client_fd, response_str, strlen(response_str), 0);
        return -1;
    }

    // Init response
    memset(&response, 0, sizeof(response));

    // Handle request
    if(lib->func(&request, &response) < 0)
    {
        // Send internal server error response
        const char *response_str = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Length: 0\r\n"
                                   "Connection: close\r\n\r\n";
        send(client_fd, response_str, strlen(response_str), 0);
        return -1;
    }

    // Send response headers
    switch(response.status_code)
    {
        case HTTP_OK:
            status_text = "OK";
            break;
        case HTTP_BAD_REQUEST:
            status_text = "Bad Request";
            break;
        case HTTP_NOT_FOUND:
            status_text = "Not Found";
            break;
        case HTTP_METHOD_NOT_ALLOWED:
            status_text = "Method Not Allowed";
            break;
        case HTTP_INTERNAL_SERVER_ERROR:
        default:
            status_text = "Internal Server Error";
            break;
    }

    snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", response.status_code, status_text);

    send(client_fd, status_line, strlen(status_line), 0);
    send(client_fd, response.headers, strlen(response.headers), 0);
    send(client_fd, "\r\n", 2, 0);

    // Send response body for non-HEAD requests
    if(strcmp(request.method, HTTP_METHOD_HEAD) != 0 && response.body && response.body_length > 0)
    {
        size_t total_sent = 0;
        while(total_sent < response.body_length)
        {
            ssize_t bytes_sent = send(client_fd, response.body + total_sent, response.body_length - total_sent, 0);
            if(bytes_sent <= 0)
            {
                if(bytes_sent < 0)
                {
                    perror("send");
                }
                break;
            }
            total_sent += (size_t)bytes_sent;
        }
    }

    free_http_response(&response);

    return 0;
}

int init_workers(worker_t *workers, int fd)
{
    pid_t pid;

    // init pipe fds to -1 to indicate not in use
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        workers[i].pipe_fd[0] = -1;
        workers[i].pipe_fd[1] = -1;
        workers[i].pid        = -1;
    }

    for(int i = 0; i < WORKER_COUNT; i++)
    {
        // Create pipe for communication
        if(pipe(workers[i].pipe_fd) < 0)
        {
            perror("init_workers: pipe\n");

            for(int j = 0; j < i; j++)
            {
                // cleanup previously created pipes
                if(workers[j].pipe_fd[1] >= 0)
                {
                    close(workers[j].pipe_fd[1]);
                    workers[j].pipe_fd[1] = -1;
                }
            }
            return -1;
        }

        // Fork worker process
        pid = fork();
        if(pid < 0)
        {
            perror("init_workers: fork\n");
            close(workers[i].pipe_fd[0]);
            close(workers[i].pipe_fd[1]);
            workers[i].pipe_fd[0] = -1;
            workers[i].pipe_fd[1] = -1;

            // cleanup previously created pipes and kill workers
            for(int j = 0; j < i; j++)
            {
                if(workers[j].pid > 0)
                {
                    kill(workers[j].pid, SIGTERM);
                    waitpid(workers[j].pid, NULL, 0);
                    workers[j].pid = -1;
                }
                if(workers[j].pipe_fd[1] >= 0)
                {
                    close(workers[j].pipe_fd[1]);
                    workers[j].pipe_fd[1] = -1;
                }
            }
            return -1;
        }
        else if(pid == 0)    // child process
        {
            int result;
            // Close write end of current pipe
            close(workers[i].pipe_fd[1]);
            workers[i].pipe_fd[1] = -1;

            // Close all other pipes
            for(int k = 0; k < i; k++)
            {
                if(workers[k].pipe_fd[0] >= 0)
                {
                    close(workers[k].pipe_fd[0]);
                    workers[k].pipe_fd[0] = -1;
                }
                if(workers[k].pipe_fd[1] >= 0)
                {
                    close(workers[k].pipe_fd[1]);
                    workers[k].pipe_fd[1] = -1;
                }
            }

            // Set up signal handlers
            signal(SIGINT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);

            // Run worker main function
            result = worker_main(fd, workers[i].pipe_fd[0]);
            close(workers[i].pipe_fd[0]);    // close read end
            exit(result);
        }
        else
        {
            // Parent process
            workers[i].pid = pid;
            close(workers[i].pipe_fd[0]);    // Close read end
            workers[i].pipe_fd[0] = -1;
        }
    }

    return 0;
}

int worker_main(int fd, int pipe_fd)
{
    int                client_fd;
    struct sockaddr_in client_addr;
    socklen_t          client_len;
    http_lib_t        *lib;
    fd_set             read_fds;
    int                max_fd;
    int                ready;

    // Load HTTP handler library
    lib = load_http_lib();
    if(!lib)
    {
        fprintf(stderr, "Failed to load HTTP handler library\n");
        return -1;
    }

    // Set up for select
    max_fd = (fd > pipe_fd) ? fd : pipe_fd;

    while(1)
    {
        // Prepare file descriptor set
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        FD_SET(pipe_fd, &read_fds);

        // Wait for activity
        ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if(ready < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            perror("worker_main: select\n");
            break;
        }

        // Check for message from parent
        if(FD_ISSET(pipe_fd, &read_fds))
        {
            char    cmd;
            ssize_t bytes_read = read(pipe_fd, &cmd, 1);
            if(bytes_read <= 0)
            {
                if(bytes_read < 0)
                {
                    perror("worker_main: read\n");
                }
                break;
            }

            if(cmd == 'Q')    // quit
            {
                break;
            }
        }

        // Check for new connection
        if(FD_ISSET(fd, &read_fds))
        {
            client_len = sizeof(client_addr);
            client_fd  = accept(fd, (struct sockaddr *)&client_addr, &client_len);
            if(client_fd < 0)
            {
                perror("worker_main: accept\n");
                continue;
            }

            // Log client connection
            log_message("Worker %d: Connection from %s:%d", (int)getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Check for library update
            if(check_lib_update(lib) > 0)
            {
                reload_http_lib(lib);
            }

            // Handle client request
            handle_client(client_fd, lib);

            // Close client connection
            close(client_fd);
        }
    }

    free_http_lib(lib);
    return 0;
}

int monitor_workers(worker_t *workers, int fd)
{
    int status;

    // Check for terminated workers
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if(pid > 0)
    {
        // Find the worker that terminated
        for(int i = 0; i < WORKER_COUNT; i++)
        {
            if(workers[i].pid == pid)
            {
                log_message("Worker %d terminated, restarting...", pid);

                workers[i].pid = -1;    // mark as terminated

                // close old pipe if open
                if(workers[i].pipe_fd[1] >= 0)
                {
                    close(workers[i].pipe_fd[1]);
                    workers[i].pipe_fd[1] = -1;
                }

                // Create new pipe
                if(pipe(workers[i].pipe_fd) < 0)
                {
                    perror("monitor_workers: pipe\n");
                    return -1;
                }

                // Fork new worker
                pid = fork();
                if(pid < 0)
                {
                    perror("monitor_workers: fork\n");
                    // clean up new pipe
                    close(workers[i].pipe_fd[0]);
                    close(workers[i].pipe_fd[1]);
                    workers[i].pipe_fd[0] = -1;
                    workers[i].pipe_fd[1] = -1;
                    return -1;
                }
                else if(pid == 0)    // Child process
                {
                    int result;
                    // Close write end of current pipe
                    close(workers[i].pipe_fd[1]);
                    workers[i].pipe_fd[1] = -1;

                    // Close all other pipes
                    for(int j = 0; j < WORKER_COUNT; j++)
                    {
                        if(j != i)
                        {
                            if(workers[j].pipe_fd[0] >= 0)
                            {
                                close(workers[j].pipe_fd[0]);
                                workers[j].pipe_fd[0] = -1;
                            }
                            if(workers[j].pipe_fd[1] >= 0)
                            {
                                close(workers[j].pipe_fd[1]);
                                workers[j].pipe_fd[1] = -1;
                            }
                        }
                    }

                    // Set up signal handlers
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTERM, SIG_DFL);

                    // Run worker main function
                    result = worker_main(fd, workers[i].pipe_fd[0]);

                    close(workers[i].pipe_fd[0]);    // close read end of pipe
                    exit(result);
                }
                else
                {
                    // Parent process
                    workers[i].pid = pid;
                    close(workers[i].pipe_fd[0]);    // Close read end
                    workers[i].pipe_fd[0] = -1;
                }

                break;
            }
        }
    }

    return 0;
}

void cleanup_workers(worker_t *workers)
{
    for(int i = 0; i < WORKER_COUNT; i++)
    {
        // Send quit command to worker
        if(workers[i].pipe_fd[1] >= 0)
        {
            // try to send quit command
            if(write(workers[i].pipe_fd[1], "Q", 1) < 0)
            {
                perror("cleanup_workers: write\n");
            }
            // close write end of pipe
            close(workers[i].pipe_fd[1]);
            workers[i].pipe_fd[1] = -1;
        }

        // close read end if still open
        if(workers[i].pipe_fd[0] >= 0)
        {
            close(workers[i].pipe_fd[0]);
            workers[i].pipe_fd[0] = -1;
        }

        // Wait for worker to terminate
        if(workers[i].pid > 0)
        {
            int   status;
            pid_t result;
            int   attempts = 0;

            // first try sigterm
            kill(workers[i].pid, SIGTERM);

            // wait for timeout
            result = waitpid(workers[i].pid, &status, WNOHANG);

            // if process has not terminated yet, wait and check again
            while(result == 0 && attempts < 3)
            {
                sleep(1);
                result = waitpid(workers[i].pid, &status, WNOHANG);
                attempts++;
            }

            // if still alive, send sigkill
            if(result == 0)
            {
                log_message("Worker %d not responding to SIGTERM, sending SIGKILL", workers[i].pid);
                kill(workers[i].pid, SIGKILL);
                waitpid(workers[i].pid, NULL, 0);
            }
            workers[i].pid = -1;
        }
    }
}
