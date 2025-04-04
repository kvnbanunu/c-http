#include "../include/server.h"
#include "../include/config.h"
#include "../include/worker.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static int server_socket(void)
{
    int fd  = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    int opt = 1;
    if(fd < 0)
    {
        perror("server_socket: socket\n");
        return -1;
    }

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("server_socket: setsockopt\n");
        close(fd);
        return -1;
    }
    return fd;
}

static socklen_t server_addr(struct sockaddr_in *addr)
{
    addr->sin_family      = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port        = htons((in_port_t)PORT);
    return sizeof(*addr);
}

// cppcheck-suppress constParameterPointer
static int server_bind(int fd, struct sockaddr_in *addr, socklen_t addr_len)
{
    int res = bind(fd, (struct sockaddr *)addr, addr_len);
    if(res != 0)
    {
        perror("server_bind\n");
        close(fd);
    }
    return res;
}

static int server_listen(int fd)
{
    int res = listen(fd, SOMAXCONN);
    if(res != 0)
    {
        perror("server_listen\n");
        close(fd);
    }
    return res;
}

int server_init(void)
{
    struct sockaddr_in addr;
    socklen_t          addr_len;

    int fd = server_socket();
    if(fd < 0)
    {
        return -1;
    }
    addr_len = server_addr(&addr);
    if(server_bind(fd, &addr, addr_len) != 0)
    {
        return -1;
    }
    if(server_listen(fd) != 0)
    {
        return -1;
    }
    printf("Server listening on port: %d\n", PORT);
    return fd;
}

int server_run(int fd)
{
    // init workers
    if(worker_init(fd) < 0)
    {
        fprintf(stderr, "server_run: Failed to initialize workers\n");
        return -1;
    }

    printf("Server running with %d workers\n", WORKER_COUNT);
    printf("Handler library: %s\n", HANDLER_LIBRARY);

    while(1)
    {
        sleep(1);
    }

    // no return should not reach here
}

void server_cleanup(int fd)
{
    worker_cleanup();

    if(fd >= 0)
    {
        close(fd);
    }
}

/* forward signal to workers */
void server_signal_handler(int sig)
{
    worker_signal_handler(sig);
}
