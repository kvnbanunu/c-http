#include "../include/setup.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct Context
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
} ctx_t;

static volatile sig_atomic_t running = 1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void sig_handler(int sig);
static void setup_sig_handler(void);
static void setup(ctx_t *c, char s[INET_ADDRSTRLEN]);
static void cleanup(const ctx_t *c);

int main(void)
{
    ctx_t ctx = {0};
    char addr_str[INET_ADDRSTRLEN];
    int retval = EXIT_FAILURE;

    setup(&ctx, addr_str);

    while(running)
    {
        int cfd = accept(ctx.fd, NULL, 0);
        if(cfd < 0)
        {
            goto cleanup;
        }
    }

    retval = EXIT_SUCCESS;
cleanup:
    cleanup(&ctx);
exit:
    exit(retval);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* Write to stdout a shutdown message and set exit_flag to end while loop in main */
static void sig_handler(int sig)
{
    const char *message = "\nSIGINT received. Server shutting down.\n";
    write(1, message, strlen(message));
    running = 0;
}

#pragma GCC diagnostic pop

/* Pairs SIGINT with sig_handler */
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

static void setup(ctx_t *c, char s[INET_ADDRSTRLEN])
{
    memset(c, 0, sizeof(ctx_t));
    find_address(&(c->addr.sin_addr.s_addr), s);
    c->addr_len = sizeof(struct sockaddr_in);
    c->fd = setup_server(&(c->addr));
    find_port(&(c->addr), s);
    setup_sig_handler();
}

static void cleanup(const ctx_t *c)
{
    if (c->fd)
    {
        close(c->fd);
    }
}
