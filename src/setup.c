#include "../include/setup.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PREFIX "192.168"

void find_address(in_addr_t *address, char *address_str)
{
    struct ifaddrs       *ifaddr;
    const struct ifaddrs *ifa;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
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
                perror("getnameinfo");
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
        perror("no address");
        exit(EXIT_FAILURE);
    }
    freeifaddrs(ifaddr);
}

static int setup_socket(void)
{
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    if(fd < 0)
    {
        perror("socket");
        return -1;
    }

    return fd;
}

int setup_server(struct sockaddr_in *addr)
{
    int       fd;
    socklen_t addr_len = sizeof(struct sockaddr);

    fd = setup_socket();
    if(fd < 0)
    {
        goto fail;
    }

    addr->sin_family = AF_INET;
    addr->sin_port   = 0;    // lets system decide the port

    if(bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind");
        goto fail;
    }

    if(listen(fd, SOMAXCONN) < 0)
    {
        perror("listen");
        goto fail;
    }

    if(getsockname(fd, (struct sockaddr *)addr, &addr_len) == -1)
    {
        perror("getsockname");
        goto fail;
    }
    return fd;
fail:
    if(fd >= 0)
    {
        close(fd);
    }
    exit(EXIT_FAILURE);
}

// cppcheck-suppress constParameterPointer
void find_port(struct sockaddr_in *addr, const char host_address[INET_ADDRSTRLEN])
{
    char port_str[NI_MAXSERV];
    if(getnameinfo((struct sockaddr *)addr, sizeof(struct sockaddr_in), NULL, 0, port_str, sizeof(port_str), NI_NUMERICSERV) != 0)
    {
        perror("getnameinfo");
        exit(EXIT_FAILURE);
    }
    printf("Listening on %s:%s\n", host_address, port_str);
}
