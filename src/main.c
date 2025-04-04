#include "../include/server.h"
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int fd = -1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void main_signal_handler_setup(void);
static void main_signal_handler(int sig);

int main(void)
{
    int       retval = EXIT_SUCCESS;

    fd = server_init();
    if(fd < 0)
    {
        perror("main: setup_server\n");
        retval = EXIT_FAILURE;
        exit(retval);
    }
    
    main_signal_handler_setup();

    retval = server_run(fd);

    // The only way for the server to shutdown is signal, so code should never reach here

    server_cleanup(fd);

    exit(retval);
    
}

static void main_signal_handler_setup(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = main_signal_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Ignore SIGPIPE (client disconnect)
    signal(SIGPIPE, SIG_IGN);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void main_signal_handler(int sig)
{
    if(sig == SIGINT || sig == SIGTERM)
    {
        const char *shutdown_message = "\nSignal received, shutting down...\n";
        write(1, shutdown_message, strlen(shutdown_message) + 1);

        server_signal_handler(sig);

        server_cleanup(fd);

        exit(EXIT_SUCCESS);
    }
}

#pragma GCC diagnostic pop
