#ifndef HANDLER_H
#define HANDLER_H

#include <stdlib.h>

struct req_info
{
    char *method;
    char *path;
    char *protocol;
};

void       *handle_request(void *arg);
void        construct_response(int clientfd, const char *status, const char *body, const char *mime, size_t body_len);
void        parse_request(struct req_info *info, char *buffer);
void        trim_protocol(char *protocol);
const char *get_mime_type(const char *file_path);
int         file_verification(const char *file_path);
int         open_requested_file(int *fd, const char *path);
int         file_readable(const char *file_path);
int         file_exists(const char *file_path);
size_t      find_content_length(int fd);
void        linktest(void);

#endif    // !HANDLER_H
