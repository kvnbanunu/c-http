#include "../include/config.h"
#include "../include/database.h"
#include "../include/http_handler.h"
#include "../include/utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Add a header to the HTTP response
 * @param response Pointer to http_response_t structure
 * @param name Header name
 * @param value Header value
 * @return 0 on success, -1 on failure
 */
static int add_response_header(http_response_t *response, const char *name, const char *value)
{
    char   header[MAX_HEADER_SIZE];
    size_t header_len;
    size_t current_len;

    snprintf(header, sizeof(header), "%s: %s\r\n", name, value);
    header_len  = strlen(header);
    current_len = strlen(response->headers);

    if(current_len + header_len >= sizeof(response->headers))
    {
        return -1;
    }

    strcat(response->headers, header);
    return 0;
}

/**
 * Handle GET and HEAD requests
 * @param request Pointer to http_request_t structure
 * @param response Pointer to http_response_t structure
 * @param document_root Document root directory
 * @return 0 on success, -1 on failure
 */
static int handle_get_head(const http_request_t *request, http_response_t *response)
{
    char        path[MAX_PATH_LENGTH];
    char        decoded_uri[MAX_URI_LENGTH];
    char        buffer[MAX_REQUEST_SIZE];
    char        content_length[32];
    const char *uri;
    const char *ext;
    const char *mime_type;
    struct stat st;
    int         is_head;
    int         fd;
    off_t       offset;

    // Decode URI
    uri = request->uri;
    url_decode(uri, decoded_uri, sizeof(decoded_uri));

    // Check for directory traversal attempts
    if(strstr(decoded_uri, ".."))
    {
        response->status_code = HTTP_BAD_REQUEST;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Connection", "close");
        return 0;
    }

    // If URI is just /, serve index.html
    if(strcmp(decoded_uri, "/") == 0)
    {
        strncpy(decoded_uri, "/index.html", sizeof(decoded_uri) - 1);
        decoded_uri[sizeof(decoded_uri) - 1] = '\0';
    }

    // Construct file path
    snprintf(path, sizeof(path), "%s%s", DOCUMENT_ROOT, decoded_uri);

    // Check if file exists and is readable
    if(stat(path, &st) < 0)
    {
        response->status_code = HTTP_NOT_FOUND;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Connection", "close");
        return 0;
    }

    // Check if path is a directory
    if(S_ISDIR(st.st_mode))
    {
        // Append index.html to the path
        size_t len = strlen(path);
        if(path[len - 1] != '/')
        {
            strncat(path, "/", sizeof(path) - len - 1);
            len++;
        }
        strncat(path, "index.html", sizeof(path) - len - 1);

        // Check if index.html exists
        if(stat(path, &st) < 0)
        {
            response->status_code = HTTP_NOT_FOUND;
            add_response_header(response, "Content-Length", "0");
            add_response_header(response, "Connection", "close");
            return 0;
        }
    }

    // Open file
    fd = open(path, O_RDONLY);
    if(fd < 0)
    {
        response->status_code = HTTP_NOT_FOUND;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Connection", "close");
        return 0;
    }

    // Check if this is a HEAD request
    is_head = (strcmp(request->method, HTTP_METHOD_HEAD) == 0);

    // Set response status
    response->status_code = HTTP_OK;

    // Set Content-Length header
    snprintf(content_length, sizeof(content_length), "%lld", (long long)st.st_size);
    add_response_header(response, "Content-Length", content_length);

    // Set Content-Type header
    ext       = get_file_extension(path);
    mime_type = get_mime_type(ext);
    add_response_header(response, "Content-Type", mime_type);

    // Set Connection header
    add_response_header(response, "Connection", "close");

    // For HEAD requests, don't send the body
    if(is_head)
    {
        close(fd);
        return 0;
    }

    // Allocate buffer for file content
    response->body = malloc((size_t)st.st_size);
    if(!response->body)
    {
        close(fd);
        return -1;
    }

    // Read file content
    offset = 0;
    while(offset < st.st_size)
    {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if(bytes_read <= 0)
        {
            if(bytes_read < 0)
            {
                perror("handle_get_head: read\n");
            }
            close(fd);
            free(response->body);
            response->body = NULL;
            return -1;
        }

        memcpy(response->body + offset, buffer, (size_t)bytes_read);
        offset += bytes_read;
    }

    response->body_length = (size_t)st.st_size;
    close(fd);

    return 0;
}

/**
 * Parse form data from POST request
 * @param body Request body
 * @param content_length Content length
 * @param key Buffer to store key
 * @param key_size Size of key buffer
 * @param value Buffer to store value
 * @param value_size Size of value buffer
 * @return 0 on success, -1 on failure
 */
static int parse_form_data(const char *body, int content_length, char *key, size_t key_size, char *value, size_t value_size)
{
    const char *ptr;
    const char *end;
    const char *equal_sign;
    size_t      key_len;
    size_t      value_len;

    if(!body || content_length <= 0 || !key || key_size == 0 || !value || value_size == 0)
    {
        return -1;
    }

    // Initialize buffers
    key[0]   = '\0';
    value[0] = '\0';

    // Find the first key=value pair
    ptr        = body;
    end        = body + content_length;
    equal_sign = strchr(ptr, '=');
    if(!equal_sign || equal_sign >= end)
    {
        return -1;
    }

    // Extract key
    key_len = (size_t)(equal_sign - ptr);
    if(key_len >= key_size)
    {
        return -1;
    }
    strncpy(key, ptr, key_len);
    key[key_len] = '\0';

    // Extract value
    ptr       = equal_sign + 1;
    value_len = (size_t)(end - ptr);
    if(value_len >= value_size)
    {
        value_len = value_size - 1;
    }
    strncpy(value, ptr, value_len);
    value[value_len] = '\0';

    // URL decode key and value
    url_decode(key, key, key_size);
    url_decode(value, value, value_size);

    return 0;
}

/**
 * Handle POST request
 * @param request Pointer to http_request_t structure
 * @param response Pointer to http_response_t structure
 * @return 0 on success, -1 on failure
 */
static int handle_post(const http_request_t *request, http_response_t *response)
{
    DBM *dbm;
    char key[256];
    char value[4096];
    char db_key[256];
    char success_response[] = "Data stored successfully";

    /* Open database */
    dbm = open_database();
    if(!dbm)
    {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Connection", "close");
        return 0;
    }

    // Parse form data
    if(parse_form_data(request->body, request->content_length, key, sizeof(key), value, sizeof(value)) < 0)
    {
        response->status_code = HTTP_BAD_REQUEST;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Connection", "close");
        close_database(dbm);
        return 0;
    }

    // Generate unique key for database
    generate_key(db_key, sizeof(db_key));

    // Store data in database
    if(store_data(dbm, db_key, value) < 0)
    {
        response->status_code = HTTP_INTERNAL_SERVER_ERROR;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Connection", "close");
        close_database(dbm);
        return 0;
    }

    // Close database
    close_database(dbm);

    // Set response
    response->status_code = HTTP_OK;
    add_response_header(response, "Content-Type", "text/plain");
    add_response_header(response, "Content-Length", (const char *)(intptr_t)strlen(success_response));
    add_response_header(response, "Connection", "close");

    // Set response body
    response->body = strdup(success_response);
    if(!response->body)
    {
        return -1;
    }
    response->body_length = strlen(success_response);

    return 0;
}

/*
 * This is the function that will be dynamically loaded by the server
 * to handle HTTP requests.
 */
int handle_http_request(const http_request_t *request, http_response_t *response)
{
    // Check parameters
    if(!request || !response)
    {
        return -1;
    }

    memset(response, 0, sizeof(http_response_t));

    // Handle different HTTP methods
    if(strcmp(request->method, HTTP_METHOD_GET) == 0 || strcmp(request->method, HTTP_METHOD_HEAD) == 0)
    {
        return handle_get_head(request, response);
    }
    else if(strcmp(request->method, HTTP_METHOD_POST) == 0)
    {
        return handle_post(request, response);
    }
    else
    {
        // Method not allowed
        response->status_code = HTTP_METHOD_NOT_ALLOWED;
        add_response_header(response, "Content-Length", "0");
        add_response_header(response, "Allow", "GET, HEAD, POST");
        add_response_header(response, "Connection", "close");
        return 0;
    }
}
