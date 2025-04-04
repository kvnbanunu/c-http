#include "../include/server.h"
#include "../include/worker.h"
#include "../include/config.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PREFIX "192.168"

static int server_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
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

static int find_address(in_addr_t *address, char *address_str)
{
    struct ifaddrs       *ifaddr;
    const struct ifaddrs *ifa;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("find_address: getifaddrs\n");
        return -1;
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr == NULL)
        {
            continue;
        }
        if(ifa->ifa_addr->sa_family == AF_INET)
        {
            if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), address_str, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) != 0)
            {
                perror("find_address: getnameinfo\n");
                continue;
            }
            if(strncmp(address_str, PREFIX, strlen(PREFIX)) == 0)
            {
                inet_pton(AF_INET, address_str, address);
                break;
            }
        }
    }
    if(ifa == NULL)
    {
        freeifaddrs(ifaddr);
        perror("find_address: no address\n");
        return -1;
    }
    freeifaddrs(ifaddr);
    return 0;
}

static socklen_t server_addr(struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port   = htons((in_port_t)PORT);
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
    char               address_str[INET_ADDRSTRLEN];

    int fd = server_socket();
    if(fd < 0)
    {
        return -1;
    }
    if(find_address(&addr.sin_addr.s_addr, address_str) != 0)
    {
        perror("server_init: find_address\n");
        close(fd);
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
    printf("Server listening on %s : %d\n", address_str, PORT);
    return fd;
}

int server_run(int fd)
{
    // init workers
    if(worker_init(fd) < 0)
    {

    }
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
