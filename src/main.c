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

static void setup_signal_handler(void);
static void sigint_handler(int signum);

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void sig_handler(int sig);
static void setup_sig_handler(void);
static void setup(ctx_t *c, char s[INET_ADDRSTRLEN]);
static void cleanup(const ctx_t *c);

int main(void)
{
    int                serverfd;
    socklen_t          host_addrlen;
    struct sockaddr_in host_addr;
    pthread_t          listenerThread;

    setup_socket(&serverfd);
    setup_address(&host_addr, &host_addrlen, PORT);

    if(bind(serverfd, (struct sockaddr *)&host_addr, host_addrlen) != 0)
    {
        perror("Bind");
        close(serverfd);
        return EXIT_FAILURE;
    }

    if(listen(serverfd, SOMAXCONN) != 0)
    {
        perror("Listen");
        close(serverfd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port: %d\n", PORT);

    setup_signal_handler();
    while(!exit_flag)
    {
        int clientfd;

        clientfd = accept(serverfd, (struct sockaddr *)&host_addr, &host_addrlen);
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

        if(pthread_create(&listenerThread, NULL, handle_request, (void *)&clientfd) != 0)
        {
            fprintf(stderr, "Error: Could not create thread");
        }
        printf("Handling request..\n");
        pthread_join(listenerThread, NULL);

        close(clientfd);
    }
    close(serverfd);
    return 0;
}

static void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sigint_handler;
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

static void sigint_handler(int signum)
{
    const char *shutdown_message = "\nServer shutting down...\n";
    exit_flag                    = 1;
    write(1, shutdown_message, strlen(shutdown_message) + 1);
}

#pragma GCC diagnostic pop
