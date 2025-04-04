#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "config.h"
#include <stddef.h>

/**
 * HTTP request structure
 */
typedef struct
{
    char method[16];
    char uri[MAX_URI_LENGTH];
    char version[16];
    char headers[MAX_HEADER_SIZE];
    char body[MAX_REQUEST_SIZE];
    int  content_length;
} http_request_t;

/**
 * HTTP response structure
 */
typedef struct
{
    int    status_code;
    char   headers[MAX_HEADER_SIZE];
    char  *body;
    size_t body_length;
} http_response_t;

/**
 * Parse an HTTP request from a buffer
 * @param buffer Request buffer
 * @param length Buffer length
 * @param request Pointer to http_request_t structure
 * @return 0 on success, -1 on failure
 */
int parse_http_request(const char *buffer, size_t length, http_request_t *request);

/**
 * Handle an HTTP request and generate a response
 * This is the function that will be dynamically loaded from the shared library
 * @param request Pointer to http_request_t structure
 * @param response Pointer to http_response_t structure
 * @return 0 on success, -1 on failure
 */
int handle_http_request(const http_request_t *request, http_response_t *response);

/**
 * Free resources allocated for an HTTP response
 * @param response Pointer to http_response_t structure
 */
void free_http_response(http_response_t *response);

#endif /* HTTP_HANDLER_H */
