#include "../include/server.h"
#include "../include/worker.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

int server_init(server_context_t *ctx, int port) {
    if (!ctx) {
        return -1;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(server_context_t));
    ctx->port = port;
    
    // Create socket
    ctx->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->server_socket < 0) {
        perror("socket failed");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(ctx->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(ctx->server_socket);
        return -1;
    }
    
    // Initialize server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket
    if (bind(ctx->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(ctx->server_socket);
        return -1;
    }
    
    // Listen for connections
    if (listen(ctx->server_socket, SOMAXCONN) < 0) {
        perror("listen failed");
        close(ctx->server_socket);
        return -1;
    }
    
    return 0;
}

int server_run(server_context_t *ctx, int worker_count, const char *document_root, 
              const char *handler_lib, const char *database_path) {
    if (!ctx || ctx->server_socket < 0) {
        return -1;
    }
    
    // Initialize worker context
    worker_context_t worker_ctx;
    memset(&worker_ctx, 0, sizeof(worker_ctx));
    
    // Initialize worker processes
    if (worker_init(&worker_ctx, ctx->server_socket, worker_count, document_root, handler_lib, database_path) < 0) {
        fprintf(stderr, "Failed to initialize workers\n");
        return 1;
    }
    
    printf("Server running with %d workers\n", worker_count);
    printf("Document root: %s\n", document_root);
    printf("Handler library: %s\n", handler_lib);
    printf("Database path: %s\n", database_path);
    
    // Main server loop - wait for signals
    while (!ctx->exit_flag) {
        sleep(1);
    }
    
    // Cleanup worker processes
    worker_cleanup(&worker_ctx);
    
    return 0;
}

void server_cleanup(server_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    // Close server socket
    if (ctx->server_socket >= 0) {
        close(ctx->server_socket);
        ctx->server_socket = -1;
    }
    
    printf("Server shutdown complete\n");
}

void server_handle_signal(server_context_t *ctx, int signo) {
    if (!ctx) {
        return;
    }
    
    if (signo == SIGINT || signo == SIGTERM) {
        ctx->exit_flag = 1;
    }
}
