#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

void setup_socket(int *fd);
void setup_address(struct sockaddr_in *addr, socklen_t *addrlen, in_port_t port);

#endif    // !SETUP_H
