#include "../include/config.h"
#include "../include/server.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

// Global pointer to the server context for the signal handler
static server_context_t *server_ctx = NULL;

// Signal handler
static void signal_handler(int signo);
static void print_usage(const char *program_name);

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    int worker_count = DEFAULT_WORKER_COUNT;
    char *document_root = DEFAULT_DOCUMENT_ROOT;
    char *handler_lib = DEFAULT_HANDLER_LIB;
    char *database_path = DEFAULT_DATABASE;
    server_context_t ctx;
    int opt;
    int option_index = 0;
    int retval;
    
    // Parse command-line options
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"workers", required_argument, 0, 'w'},
        {"document-root", required_argument, 0, 'd'},
        {"handler-lib", required_argument, 0, 'l'},
        {"database", required_argument, 0, 'b'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "p:w:d:l:b:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > UINT16_MAX)
                {
                    fprintf(stderr, "Invalid port number: %s\n", optarg);
                    return 1;
                }
                break;
            
            case 'w':
                worker_count = atoi(optarg);
                if (worker_count <= 0)
                {
                    fprintf(stderr, "Invalid worker count: %s\n", optarg);
                    return 1;
                }
                break;
            
            case 'd':
                document_root = optarg;
                break;
            
            case 'l':
                handler_lib = optarg;
                break;
            
            case 'b':
                database_path = optarg;
                break;
            
            case 'h':
                print_usage(argv[0]);
                return 0;
            
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Initialize server context
    server_ctx = &ctx;
    
    // Set up signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    // Ignore SIGPIPE (happens when client disconnects)
    signal(SIGPIPE, SIG_IGN);
    
    // Initialize server
    if (server_init(&ctx, port) < 0)
    {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    printf("Server initialized on port %d\n", port);
    
    // Run server
    retval = server_run(&ctx, worker_count, document_root, handler_lib, database_path);
    
    // Cleanup
    server_cleanup(&ctx);
    
    return retval;
}

static void signal_handler(int signo)
{
    if (server_ctx && (signo == SIGINT || signo == SIGTERM))
    {
        printf("\nReceived signal %d, shutting down...\n", signo);
        
        // Forward signal to server processes
        server_handle_signal(server_ctx, signo);
        
        // Cleanup server resources
        server_cleanup(server_ctx);
        
        exit(0);
    }
}

static void print_usage(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -p, --port PORT           Port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -w, --workers COUNT       Number of worker processes (default: %d)\n", DEFAULT_WORKER_COUNT);
    printf("  -d, --document-root DIR   Document root directory (default: %s)\n", DEFAULT_DOCUMENT_ROOT);
    printf("  -l, --handler-lib FILE    Path to handler shared library (default: %s)\n", DEFAULT_HANDLER_LIB);
    printf("  -b, --database FILE       Path to database file (default: %s)\n", DEFAULT_DATABASE);
    printf("  -h, --help                Display this help message\n");
}
