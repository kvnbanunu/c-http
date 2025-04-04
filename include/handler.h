#ifndef HANDLER_H
#define HANDLER_H

#include <stdlib.h>

#ifdef __APPLE__
typedef size_t datum_size;
#else
typedef int datum_size;
#endif

#define TO_SIZE_T(x) ((size_t)(x))

#define MAKE_CONST_DATUM(str) ((const_datum){(str), (datum_size)strlen(str) + 1})

struct req_info
{
    char *method;
    char *path;
    char *protocol;
};

typedef struct
{
    const void *dptr;
    datum_size  dsize;
} const_datum;

// Function signature for shared library
void handle_request(int client_fd);

#endif    // !HANDLER_H
