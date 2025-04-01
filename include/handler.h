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

#endif    // !HANDLER_H
