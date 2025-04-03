#ifndef HANDLER_H
#define HANDLER_H

#include <stddef.h>

/**
 * Struct respresenting an HTTP request
 */
typedef struct
{
    char *method;
    char *uri;
    char *version;
    char *headers;
    char *body;
    size_t body_len;
} request_t;

/**
 * Struct representing an HTTP response
 */
typedef struct
{
    int status_code;
    char *status_text;
    char *headers;
    char *body;
    size_t body_len;
} response_t;

// Function signatures for the shared library
typedef response_t* (*handle_request)(const request_t *req, const char *doc_root, const char *db_path);
typedef void (*handler_free_response)(response_t *res);

#endif /* HANDLER_H */
