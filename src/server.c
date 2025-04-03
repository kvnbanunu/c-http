#include "../include/server.h"
#include "../include/worker.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PREFIX "192.168"

static int setup_socket(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)
    int opt = 1;
    if(fd < 0)
    {
        perror("setup_socket\n");
        return fd;
    }

    // allows reuse of local addresses
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setup_socket: setsockopt\n");
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
                perror("find_address: getnameinfo");
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
        perror("find_address: no address");
        return -1;
    }
    freeifaddrs(ifaddr);
    return 0;
}

static socklen_t setup_address(struct sockaddr_in *addr, in_port_t port)
{
    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);
    return sizeof(*addr);
}

// cppcheck-suppress constParameterPointer
static int setup_bind(int fd, struct sockaddr_in *addr, socklen_t addr_len)
{
    int res = bind(fd, (struct sockaddr *)addr, addr_len);
    if(res != 0)
    {
        perror("setup_bind\n");
        close(fd);
    }
    return res;
}

static int setup_listen(int fd)
{
    int res = listen(fd, SOMAXCONN);
    if(res != 0)
    {
        perror("setup_listen\n");
        close(fd);
    }
    return res;
}

int setup_server(in_port_t port)
{
    struct sockaddr_in addr;
    socklen_t          addr_len;
    char               address_str[INET_ADDRSTRLEN];

    int fd = setup_socket();
    if(fd < 0)
    {
        return -1;
    }
    if(find_address(&addr.sin_addr.s_addr, address_str) != 0)
    {
        perror("find_address\n");
        close(fd);
        return -1;
    }
    addr_len = setup_address(&addr, port);
    if(setup_bind(fd, &addr, addr_len) != 0)
    {
        return -1;
    }
    if(setup_listen(fd) != 0)
    {
        return -1;
    }
    printf("Server listening on %s : %hu\n", address_str, port);
    return fd;
}

int run_server(int fd, int worker_count, const char *doc_root, const char *handler_lib, const char *db_path)
{
    if(worker_init(fd, worker_count, doc_root, handler_lib, db_path) < 0)
    {
        fprintf(stderr, "Failed to initialize workers\n");
        return 1;
    }

    printf("Server running with %d workers\n", worker_count);
    printf("Document root: %s\n", doc_root);
    printf("Handler library: %s\n", handler_lib);
    printf("Database path: %s\n", db_path);

    // Server loop
    while (1)
    {
        sleep(1); // wait for signals
    }

    return 0;
}

void server_cleanup(int fd)
{
    worker_cleanup();

    if (fd >= 0)
    {
        close(fd);
    }
    
    printf("Server shutdown complete\n");
}

void server_handle_signal(int sig)
{
    worker_handle_signal(sig);
}
