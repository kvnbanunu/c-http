#include "../include/setup.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void setup_socket(int *fd)
{
    *fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    if(*fd < 0)
    {
        perror("Server Socket creation");
        exit(EXIT_FAILURE);
    }
}

void setup_address(struct sockaddr_in *addr, socklen_t *addrlen, in_port_t port)
{
    memset(addr, 0, sizeof(*addr));

    addr->sin_family      = AF_INET;
    addr->sin_port        = htons(port);
    addr->sin_addr.s_addr = htonl(INADDR_ANY);

    *addrlen = sizeof(*addr);
}
