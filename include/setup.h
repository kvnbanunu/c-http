#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

/* sets up socket and returns fd */
int setup_server(in_port_t port);

#endif    // !SETUP_H
