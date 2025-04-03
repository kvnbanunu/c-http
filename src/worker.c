#include "../include/worker.h"
#include "../include/handler.h"
#include "../include/database.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

// Context pointer for the signal handler
static worker_context_t *signal_ctx = NULL;

// Signal handler for workers
static void worker_signal_handler(int signo) {
    if (signal_ctx && (signo == SIGTERM || signo == SIGINT)) {
        signal_ctx->exit_flag = 1;
    }
}

// Worker process implementation
static void worker_process(worker_context_t *ctx, int worker_id) {
    if (!ctx) {
        return;
    }
    
    // Set up signal handling for this worker
    signal_ctx = ctx;
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = worker_signal_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    
    printf("Worker %d (PID %d) started\n", worker_id, getpid());
    
    // Initialize database context
    db_context_t db_ctx;
    memset(&db_ctx, 0, sizeof(db_ctx));
    
    if (db_init(&db_ctx, ctx->database_path) < 0) {
        fprintf(stderr, "Worker %d: Failed to initialize database\n", worker_id);
        exit(1);
    }
    
    // Track shared library modification time
    struct stat lib_stat;
    time_t lib_mtime = 0;
    
    if (stat(ctx->handler_lib_path, &lib_stat) == 0) {
        lib_mtime = lib_stat.st_mtime;
    }
    
    // Worker loop
    while (!ctx->exit_flag) {
        // Accept client connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        // Use select() to wait for connections with timeout
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(ctx->server_socket, &read_fds);
        timeout.tv_sec = 1;  // 1 second timeout to check exit flag
        timeout.tv_usec = 0;
        
        int select_result = select(ctx->server_socket + 1, &read_fds, NULL, NULL, &timeout);
        
        if (select_result <= 0) {
            if (select_result < 0 && errno != EINTR) {
                perror("select failed");
            }
            continue;
        }
        
        int client_socket = accept(ctx->server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, try again
            }
            perror("accept failed");
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Worker %d: Accepted connection from %s:%d\n", worker_id, client_ip, ntohs(client_addr.sin_port));
        
        // Check for handler library updates
        void *handler_lib = NULL;
        int reload_lib = 0;

        if (stat(ctx->handler_lib_path, &lib_stat) == 0) {
            if (lib_stat.st_mtime > lib_mtime) {
                lib_mtime = lib_stat.st_mtime;
                reload_lib = 1;
                printf("Worker %d: Detected updated handler library\n", worker_id);
            }
        }
        
        // Load handler library
        handler_lib = dlopen(ctx->handler_lib_path, RTLD_NOW);
        if (!handler_lib) {
            fprintf(stderr, "Worker %d: Failed to load handler library: %s\n", worker_id, dlerror());
            close(client_socket);
            continue;
        }
        
        // Get handler functions
        http_handler_init init_func = (http_handler_init)dlsym(handler_lib, "http_handler_init");
        http_handler_cleanup cleanup_func = (http_handler_cleanup)dlsym(handler_lib, "http_handler_cleanup");
        http_handler_handle_request handle_func = (http_handler_handle_request)dlsym(handler_lib, "http_handler_handle_request");
        http_handler_free_response free_response_func = (http_handler_free_response)dlsym(handler_lib, "http_handler_free_response");
        
        if (!init_func || !cleanup_func || !handle_func || !free_response_func) {
            fprintf(stderr, "Worker %d: Failed to resolve handler functions: %s\n", worker_id, dlerror());
            dlclose(handler_lib);
            close(client_socket);
            continue;
        }
        
        // Initialize handler
        init_func();
        
        // Read HTTP request
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            cleanup_func();
            dlclose(handler_lib);
            close(client_socket);
            continue;
        }
        buffer[bytes_read] = '\0';
        
        // Parse HTTP request (simplified version)
        http_request_t request;
        memset(&request, 0, sizeof(request));
        
        // Split the request into headers and body
        char *header_end = strstr(buffer, "\r\n\r\n");
        if (header_end) {
            *header_end = '\0';
            
            // Body starts after the headers
            request.body = header_end + 4;
            request.body_length = bytes_read - (request.body - buffer);
            
            // Parse the request line
            char *line_end = strstr(buffer, "\r\n");
            if (line_end) {
                *line_end = '\0';
                
                // Parse method, URI and version
                char *method_end = strchr(buffer, ' ');
                if (method_end) {
                    *method_end = '\0';
                    request.method = buffer;
                    
                    char *uri_end = strchr(method_end + 1, ' ');
                    if (uri_end) {
                        *uri_end = '\0';
                        request.uri = method_end + 1;
                        request.version = uri_end + 1;
                    }
                }
                
                // Headers start after the request line
                request.headers = line_end + 2;
            }
        }
        
        if (!request.method || !request.uri || !request.version) {
            const char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
            write(client_socket, bad_request, strlen(bad_request));
            cleanup_func();
            dlclose(handler_lib);
            close(client_socket);
            continue;
        }
        
        printf("Worker %d: %s %s %s\n", worker_id, request.method, request.uri, request.version);
        
        // Handle the request
        http_response_t *response = handle_func(&request, ctx->document_root, ctx->database_path);
        
        // Send response
        if (response) {
            // Construct HTTP response
            char response_header[BUFFER_SIZE];
            int header_len = snprintf(response_header, BUFFER_SIZE,
                                      "HTTP/1.1 %d %s\r\n"
                                      "%s"
                                      "Content-Length: %zu\r\n"
                                      "\r\n",
                                      response->status_code, response->status_text,
                                      response->headers ? response->headers : "",
                                      response->body_length);
            
            // Send header
            write(client_socket, response_header, header_len);
            
            // Send body if present
            if (response->body && response->body_length > 0) {
                write(client_socket, response->body, response->body_length);
            }
            
            // Free response
            free_response_func(response);
        } else {
            // Send error response
            const char *error_response = "HTTP/1.1 500 Internal Server Error\r\n"
                                         "Content-Length: 21\r\n"
                                         "\r\n"
                                         "Internal Server Error";
            write(client_socket, error_response, strlen(error_response));
        }
        
        // Cleanup
        cleanup_func();
        dlclose(handler_lib);
        close(client_socket);
    }
    
    printf("Worker %d (PID %d) shutting down\n", worker_id, getpid());
    
    // Cleanup database
    db_cleanup(&db_ctx);
    exit(0);
}

// Monitor and restart worker processes
static void *monitor_workers(worker_context_t *ctx) {
    if (!ctx || !ctx->worker_pids) {
        return NULL;
    }
    
    while (!ctx->exit_flag) {
        // Wait for any child process to terminate
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        
        if (pid > 0) {
            // Find which worker terminated
            for (int i = 0; i < ctx->worker_count; i++) {
                if (ctx->worker_pids[i] == pid) {
                    printf("Worker %d (PID %d) terminated, restarting...\n", i, pid);
                    
                    // Restart the worker
                    pid_t new_pid = fork();
                    if (new_pid == 0) {
                        // Child process
                        worker_process(ctx, i);
                        exit(0);
                    } else if (new_pid > 0) {
                        // Parent process
                        ctx->worker_pids[i] = new_pid;
                    } else {
                        // Fork failed
                        perror("fork failed");
                    }
                    
                    break;
                }
            }
        } else if (pid == 0 || (pid < 0 && errno == ECHILD)) {
            // No children have terminated, sleep for a bit
            usleep(100000); // 100ms
        } else {
            perror("waitpid failed");
            break;
        }
    }
    
    return NULL;
}

int worker_init(worker_context_t *ctx, int server_socket, int w_count, 
                const char *doc_root, const char *handler_lib, const char *db_path) {
    if (!ctx) {
        return -1;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(worker_context_t));
    ctx->server_socket = server_socket;
    ctx->worker_count = w_count;
    ctx->document_root = strdup(doc_root);
    ctx->handler_lib_path = strdup(handler_lib);
    ctx->database_path = strdup(db_path);
    
    if (!ctx->document_root || !ctx->handler_lib_path || !ctx->database_path) {
        perror("strdup failed");
        worker_cleanup(ctx);
        return -1;
    }
    
    // Allocate worker PIDs array
    ctx->worker_pids = malloc(w_count * sizeof(pid_t));
    if (!ctx->worker_pids) {
        perror("malloc failed");
        worker_cleanup(ctx);
        return -1;
    }
    
    // Fork worker processes
    for (int i = 0; i < w_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            worker_cleanup(ctx);
            return -1;
        } else if (pid == 0) {
            // Child process
            worker_process(ctx, i);
            exit(0); // Should not reach here
        } else {
            // Parent process
            ctx->worker_pids[i] = pid;
        }
    }
    
    // Start monitoring worker processes
    monitor_workers(ctx);
    
    return 0;
}

void worker_cleanup(worker_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    printf("Cleaning up workers...\n");
    
    // Terminate all worker processes
    if (ctx->worker_pids) {
        for (int i = 0; i < ctx->worker_count; i++) {
            if (ctx->worker_pids[i] > 0) {
                printf("Sending SIGTERM to worker %d (PID %d)\n", i, ctx->worker_pids[i]);
                kill(ctx->worker_pids[i], SIGTERM);
            }
        }
        
        // Wait for all workers to terminate (with timeout)
        int timeout = 5; // 5 seconds timeout
        time_t start_time = time(NULL);
        
        while (time(NULL) - start_time < timeout) {
            int all_terminated = 1;
            
            for (int i = 0; i < ctx->worker_count; i++) {
                if (ctx->worker_pids[i] > 0) {
                    int status;
                    pid_t result = waitpid(ctx->worker_pids[i], &status, WNOHANG);
                    
                    if (result == ctx->worker_pids[i]) {
                        printf("Worker %d (PID %d) terminated\n", i, ctx->worker_pids[i]);
                        ctx->worker_pids[i] = -1;
                    } else if (result == 0) {
                        all_terminated = 0;
                    }
                }
            }
            
            if (all_terminated) {
                break;
            }
            
            usleep(100000); // 100ms
        }
        
        // Force kill any remaining workers
        for (int i = 0; i < ctx->worker_count; i++) {
            if (ctx->worker_pids[i] > 0) {
                printf("Force killing worker %d (PID %d)\n", i, ctx->worker_pids[i]);
                kill(ctx->worker_pids[i], SIGKILL);
                waitpid(ctx->worker_pids[i], NULL, 0);
            }
        }
        
        free(ctx->worker_pids);
        ctx->worker_pids = NULL;
    }
    
    // Free other resources
    if (ctx->document_root) {
        free(ctx->document_root);
        ctx->document_root = NULL;
    }
    
    if (ctx->handler_lib_path) {
        free(ctx->handler_lib_path);
        ctx->handler_lib_path = NULL;
    }
    
    if (ctx->database_path) {
        free(ctx->database_path);
        ctx->database_path = NULL;
    }
}

void worker_handle_signal(worker_context_t *ctx, int signo) {
    if (!ctx) {
        return;
    }
    
    // Set exit flag
    if (signo == SIGINT || signo == SIGTERM) {
        ctx->exit_flag = 1;
    }
    
    // Forward signal to all worker processes
    for (int i = 0; i < ctx->worker_count; i++) {
        if (ctx->worker_pids[i] > 0) {
            kill(ctx->worker_pids[i], signo);
        }
    }
}

