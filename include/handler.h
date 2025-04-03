#ifndef HANDLER_H
#define HANDLER_H

#include <stddef.h>

/**
 * Structure representing an HTTP request
 */
typedef struct {
    char *method;       // Request method (GET, POST, HEAD, etc.)
    char *uri;          // Request URI
    char *version;      // HTTP version
    char *headers;      // Request headers
    char *body;         // Request body
    size_t body_length; // Length of request body
} http_request_t;

/**
 * Structure representing an HTTP response
 */
typedef struct {
    int status_code;     // HTTP status code
    char *status_text;   // Status text
    char *headers;       // Response headers
    char *body;          // Response body
    size_t body_length;  // Length of response body
} http_response_t;

// Function signatures for the shared library
typedef void (*http_handler_init)();
typedef void (*http_handler_cleanup)();
typedef http_response_t* (*http_handler_handle_request)(const http_request_t *request, const char *document_root, const char *database_path);
typedef void (*http_handler_free_response)(http_response_t *response);

#endif /* HANDLER_H */
