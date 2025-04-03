#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

/**
 * Sets up socket and returns fd
 *
 * @param port  Desired port to listen on (host byte order)
 *
 * @return      0 on success, -1 on failure
 */
int setup_server(in_port_t port);

/**
 * Run the server
 *
 * @param fd            Socket file descriptor
 * @param worker_count  Number of worker processes to create
 * @param doc_root      Path to document root directory
 * @param handler_lib   Path to handler shared library
 * @param db_path       Path to database file
 *
 * @return              0 on success, non_zero on failure
 */
int run_server(int fd, int worker_count, const char *doc_root, const char *handler_lib, const char *db_path);

/**
 * Cleanup server resources
 *
 * @param fd Server file descriptor
 */
void server_cleanup(int fd);

/**
 * Handle signals for the server
 *
 * @param sig Signal number
 */
void server_handle_signal(int sig);

#endif    /* SERVER_H */
