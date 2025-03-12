#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

void find_address(in_addr_t *address, char *address_str);
int setup_server(struct sockaddr_in *addr);
void find_port(struct sockaddr_in *addr, const char host_address[INET_ADDRSTRLEN]);

#endif // !SETUP_H
