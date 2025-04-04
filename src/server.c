#include "../include/server.h"
#include "../include/config.h"
#include "../include/utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int init_server(void)
{
    int                fd;
    struct sockaddr_in addr;
    int                reuse = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    if(fd < 0)
    {
        perror("init_server: socket\n");
        return -1;
    }

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("init_server: setsockopt\n");
        close(fd);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((in_port_t)PORT);

    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("init_server: bind\n");
        close(fd);
        return -1;
    }

    if(listen(fd, SOMAXCONN) < 0)
    {
        perror("init_server: listen\n");
        close(fd);
        return -1;
    }

    return fd;
}

void cleanup_server(int fd)
{
    if(fd >= 0)
    {
        close(fd);
    }
}
