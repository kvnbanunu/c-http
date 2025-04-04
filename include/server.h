#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

/**
 * Sets up socket
 *
 * @return fd on success, -1 on failure
 */
int server_setup(void);

/**
 * Main server loop
 *
 * @param fd Server socket file descriptor
 *
 * @return 0 on success, non-zero on failure
 */
int server_run(int fd);

/**
 * Clean up server resources
 *
 * @param fd Server socket file descriptor
 */
void server_cleanup(int fd);

/**
 * Handle signals for the server
 *
 * @param sig Signal number
 */
void server_signal_handler(int sig);

#endif    // SERVER_H
