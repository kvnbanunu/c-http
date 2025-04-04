#ifndef SERVER_H
#define SERVER_H

/**
 * Initialize the server based on configuration
 * @return Server socket file descriptor or -1 on error
 */
int init_server(void);

/**
 * Clean up server resources
 * @param fd Server socket file descriptor
 */
void cleanup_server(int fd);

#endif /* SERVER_H */
