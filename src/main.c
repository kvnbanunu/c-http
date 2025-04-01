#include "../include/handler.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

static void setup_sig_handler(void);
static void sig_handler(int sig);

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

int main(void)
{
    int       fd;
    pthread_t thread;
    int       retval = EXIT_SUCCESS;

    fd = setup_server(PORT);
    if(fd < 0)
    {
        perror("setup_server\n");
        retval = EXIT_FAILURE;
        goto done;
    }

    setup_sig_handler();
    while(!exit_flag)
    {
        int clientfd;

        clientfd = accept(fd, NULL, 0);
        if(clientfd < 0)
        {
            if(exit_flag)
            {
                printf("Flag has been changed closing..\n");
                break;
            }
            perror("Client File Descriptor");
            continue;
        }

        if(pthread_create(&thread, NULL, handle_request, (void *)&clientfd) != 0)
        {
            fprintf(stderr, "Error: Could not create thread");
        }
        printf("Handling request..\n");
        pthread_join(thread, NULL);

        close(clientfd);
    }

    close(fd);

done:
    exit(retval);
}

static void setup_sig_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sig_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void sig_handler(int sig)
{
    const char *shutdown_message = "\nServer shutting down...\n";
    exit_flag                    = 1;
    write(1, shutdown_message, strlen(shutdown_message) + 1);
}

#pragma GCC diagnostic pop
