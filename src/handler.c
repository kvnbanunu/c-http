#include "../include/handler.h"
#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define DATE_SIZE 128
#define STATUS_OK 200
#define STATUS_NOT_FOUND 404
#define STATUS_SERVER_ERR 500
#define STATUS_NOT_IMPLEMENTED 501
#define HEADER_SIZE_FAILURE 256
#define HEADER_SIZE_SUCCESS 512
#define POST_BODY_SIZE 2048
#define ROW_SIZE 512
#define ID_SIZE 32
#define KEY_LIST_SIZE 1024
#define KEY_SIZE 256

// Handler version - increment when updating
const char *handler_version = "1.0.0";

// Initialize handler
void http_handler_init()
{
    // Nothing to initialize in this simple implementation
    printf("Handler version %s initialized\n", handler_version);
}

// Cleanup handler
void http_handler_cleanup()
{
    // Nothing to cleanup in this simple implementation
}

// Helper function to get MIME type from file extension
static const char* get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    
    if (!ext)
    {
        return "application/octet-stream";
    }

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
    {
        return "text/html";
    }

    if (strcmp(ext, ".txt") == 0)
    {
        return "text/plain";
    }

    if (strcmp(ext, ".css") == 0)
    {
        return "text/css";
    }

    if (strcmp(ext, ".js") == 0)
    {
        return "application/javascript";
    }

    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }

    if (strcmp(ext, ".png") == 0)
    {
        return "image/png";
    }

    if (strcmp(ext, ".gif") == 0)
    {
        return "image/gif";
    }

    if (strcmp(ext, ".ico") == 0)
    {
        return "image/x-icon";
    }

    if (strcmp(ext, ".pdf") == 0)
    {
        return "application/pdf";
    }

    if (strcmp(ext, ".xml") == 0)
    {
        return "application/xml";
    }

    if (strcmp(ext, ".json") == 0)
    {
        return "application/json";
    }

    return "application/octet-stream";
}

// Helper function to generate HTTP date
static char* http_date()
{
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    char *buf = (char *)malloc(DATE_SIZE);
    if (!buf)
    {
        return NULL;
    }
    
    strftime(buf, DATE_SIZE, "%a, %d %b %Y %H:%M:%S GMT", tm);
    return buf;
}

// Handle HTTP GET and HEAD requests
static http_response_t* handle_file_request(const http_request_t *request, const char *document_root, int head_only)
{
    char *uri;
    char *query_string;
    char *file_path;
    char *date;
    char *headers;
    const char *mime_type = NULL;
    int fd;
    struct stat file_stat;
    http_response_t *response = (http_response_t *)malloc(sizeof(http_response_t));
    if (!response) return NULL;
    
    memset(response, 0, sizeof(http_response_t));
    
    // Sanitize URI to prevent directory traversal
    uri = strdup(request->uri);
    if (!uri)
    {
        free(response);
        return NULL;
    }
    
    // Remove query string if present
    query_string = strchr(uri, '?');
    if (query_string)
    {
        *query_string = '\0';
    }
    
    // Default to index.html for root path
    if (strcmp(uri, "/") == 0)
    {
        free(uri);
        uri = strdup("/index.html");
        if (!uri)
        {
            free(response);
            return NULL;
        }
    }
    
    // Construct file path
    file_path = (char *)malloc(strlen(document_root) + strlen(uri) + 1);
    if (!file_path)
    {
        free(uri);
        free(response);
        return NULL;
    }
    sprintf(file_path, "%s%s", document_root, uri);
    
    // Open file
    fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        const char *not_found = "<html><head><title>404 Not Found</title></head>"
            "<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p>"
            "<hr><p>C-HTTP Server</p></body></html>";
        response->status_code = STATUS_NOT_FOUND;
        response->status_text = strdup("Not Found");
        response->body = strdup(not_found);
        response->body_length = strlen(not_found);
        
        date = http_date();
        headers = (char *)malloc(HEADER_SIZE_FAILURE);
        sprintf(headers, "Content-Type: text/html\r\nDate: %s\r\nServer: C-HTTP Server\r\n", date);
        response->headers = headers;
        
        free(date);
        free(uri);
        free(file_path);
        return response;
    }
    
    // Get file information
    if (fstat(fd, &file_stat) < 0)
    {
        close(fd);
        free(uri);
        free(file_path);
        free(response);
        return NULL;
    }
    
    // Set response status
    response->status_code = STATUS_OK;
    response->status_text = strdup("OK");
    
    // Set response headers
    date = http_date();
    mime_type = get_mime_type(file_path);
    
    headers = (char *)malloc(HEADER_SIZE_SUCCESS);
    sprintf(headers, "Content-Type: %s\r\nDate: %s\r\nServer: C-HTTP Server\r\nContent-Length: %ld\r\n", 
            mime_type, date, file_stat.st_size);
    response->headers = headers;
    
    free(date);
    
    // For HEAD requests, don't include the body
    if (head_only)
    {
        response->body = NULL;
        response->body_length = 0;
    }
    else
    {
        // Read file content
        ssize_t bytes_read;
        response->body = (char *)malloc(file_stat.st_size);
        if (!response->body)
        {
            close(fd);
            free(uri);
            free(file_path);
            free(response->status_text);
            free(response->headers);
            free(response);
            return NULL;
        }
        
        bytes_read = read(fd, response->body, file_stat.st_size);
        if (bytes_read != file_stat.st_size)
        {
            close(fd);
            free(uri);
            free(file_path);
            free(response->status_text);
            free(response->headers);
            free(response->body);
            free(response);
            return NULL;
        }
        
        response->body_length = file_stat.st_size;
    }
    
    close(fd);
    free(uri);
    free(file_path);
    return response;
}

// URL decode function
static char* url_decode(const char *str)
{
    char *decoded;
    char *p;

    if (str == NULL)
    {
        return NULL;
    }
    
    decoded = (char *)malloc(strlen(str) + 1);
    if (decoded == NULL)
    {
        return NULL;
    }
    
    p = decoded;
    while (*str)
    {
        if (*str == '%' && str[1] && str[2])
        {
            // Handle percent encoding
            char hex[3] = { str[1], str[2], 0 };
            *p++ = (char)strtol(hex, NULL, 16);
            str += 3;
        }
        else if (*str == '+')
        {
            // Handle plus as space
            *p++ = ' ';
            str++;
        }
        else
        {
            // Copy normal characters
            *p++ = *str++;
        }
    }
    *p = '\0';
    
    return decoded;
}

// Parse key-value pairs from POST data
static int parse_post_data(const char *body, size_t body_length, char ***keys, char ***values, int *count)
{
    char *body_copy;
    char *saveptr;
    char *pair;
    int pairs = 1;
    int index = 0;
    if (!body || body_length == 0)
    {
        *count = 0;
        return 0;
    }
    
    // Make a copy of the body with a null terminator
    body_copy = (char *)malloc(body_length + 1);
    if (!body_copy)
    {
        return -1;
    }
    memcpy(body_copy, body, body_length);
    body_copy[body_length] = '\0';
    
    // Count the number of '&' characters to determine the number of key-value pairs
    for (size_t i = 0; i < body_length; i++)
    {
        if (body_copy[i] == '&')
        {
            pairs++;
        }
    }
    
    // Allocate arrays for keys and values
    *keys = (char **)malloc(pairs * sizeof(char*));
    *values = (char **)malloc(pairs * sizeof(char*));
    if (!*keys || !*values)
    {
        if (*keys)
        {
            free(*keys);
        }
        if (*values)
        {
            free(*values);
        }
        free(body_copy);
        return -1;
    }
    
    // Initialize arrays
    for (int i = 0; i < pairs; i++)
    {
        (*keys)[i] = NULL;
        (*values)[i] = NULL;
    }
    
    // Parse each key-value pair
    pair = strtok_r(body_copy, "&", &saveptr);
    
    while (pair && index < pairs)
    {
        char *key;
        char *value;
        // Find the '=' separator
        char *eq = strchr(pair, '=');
        if (eq)
        {
            *eq = '\0';
            
            // Decode URL-encoded values
            key = url_decode(pair);
            value = url_decode(eq + 1);
            
            if (!key || !value)
            {
                if (key)
                {
                    free(key);
                }
                if (value)
                {
                    free(value);
                }
                
                // Cleanup on error
                for (int i = 0; i < index; i++)
                {
                    if ((*keys)[i]) free((*keys)[i]);
                    if ((*values)[i]) free((*values)[i]);
                }
                free(*keys);
                free(*values);
                free(body_copy);
                return -1;
            }
            
            (*keys)[index] = key;
            (*values)[index] = value;
            index++;
        }
        
        // Get next pair
        pair = strtok_r(NULL, "&", &saveptr);
    }
    
    *count = index;
    free(body_copy);
    return 0;
}

// Handle HTTP POST request
static http_response_t* handle_post_request(const http_request_t *request, const char *database_path)
{
    char **keys = NULL;
    char **values = NULL;
    char *date;
    char *headers;
    char *body;
    char id[ID_SIZE];
    char db_key[ID_SIZE];
    char keys_list[KEY_LIST_SIZE] = {0};
    int count = 0;
    db_context_t db_ctx;
    http_response_t *response = (http_response_t *)malloc(sizeof(http_response_t));
    if (!response)
    {
        return NULL;
    }
    
    memset(response, 0, sizeof(http_response_t));
    
    // Parse POST data
    if (parse_post_data(request->body, request->body_length, &keys, &values, &count) < 0)
    {
        const char *error = "<html><head><title>500 Internal Server Error</title></head>"
            "<body><h1>500 Internal Server Error</h1><p>Failed to parse POST data.</p>"
            "<hr><p>C-HTTP Server</p></body></html>";

        response->status_code = STATUS_SERVER_ERR;
        response->status_text = strdup("Internal Server Error");
        response->body = strdup(error);
        response->body_length = strlen(error);
        
        date = http_date();
        headers = malloc(HEADER_SIZE_FAILURE);
        sprintf(headers, "Content-Type: text/html\r\nDate: %s\r\nServer: C-HTTP Server\r\n", date);
        response->headers = headers;
        
        free(date);
        return response;
    }
    
    // Initialize database context
    memset(&db_ctx, 0, sizeof(db_ctx));
    
    if (db_init(&db_ctx, database_path) < 0)
    {
        const char *error = "<html><head><title>500 Internal Server Error</title></head>"
            "<body><h1>500 Internal Server Error</h1><p>Failed to open database.</p>"
            "<hr><p>C-HTTP Server</p></body></html>";

        for (int i = 0; i < count; i++) {
            free(keys[i]);
            free(values[i]);
        }
        free(keys);
        free(values);
        
        response->status_code = STATUS_SERVER_ERR;
        response->status_text = strdup("Internal Server Error");
        response->body = strdup(error);
        response->body_length = strlen(error);
        
        date = http_date();
        headers = (char *)malloc(HEADER_SIZE_FAILURE);
        sprintf(headers, "Content-Type: text/html\r\nDate: %s\r\nServer: C-HTTP Server\r\n", date);
        response->headers = headers;
        
        free(date);
        return response;
    }
    
    // Generate a unique ID for this POST
    sprintf(id, "post_%ld", time(NULL));
    
    // Store each key-value pair
    for (int i = 0; i < count; i++)
    {
        char key[KEY_SIZE];
        sprintf(key, "%s.%s", id, keys[i]);
        
        db_store(&db_ctx, key, values[i], strlen(values[i]));
    }
    
    // Store the list of keys
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            strcat(keys_list, ",");
        }
        strcat(keys_list, keys[i]);
    }
    
    sprintf(db_key, "%s.keys", id);
    db_store(&db_ctx, db_key, keys_list, strlen(keys_list));
    db_cleanup(&db_ctx);
    
    // Create success response
    response->status_code = STATUS_OK;
    response->status_text = strdup("OK");
    
    date = http_date();
    headers = malloc(HEADER_SIZE_SUCCESS);
    sprintf(headers, "Content-Type: text/html\r\nDate: %s\r\nServer: C-HTTP Server\r\n", date);
    response->headers = headers;
    
    // Build HTML response with the stored data
    body = (char *)malloc(POST_BODY_SIZE);
    sprintf(body, "<html><head><title>POST Successful</title></head>"
                  "<body><h1>POST Successful</h1>"
                  "<p>Data stored with ID: %s</p>"
                  "<h2>Submitted Data:</h2>"
                  "<table border=\"1\"><tr><th>Key</th><th>Value</th></tr>", id);
    
    for (int i = 0; i < count; i++)
    {
        char row[ROW_SIZE];
        sprintf(row, "<tr><td>%s</td><td>%s</td></tr>", keys[i], values[i]);
        strcat(body, row);
    }
    
    strcat(body, "</table><hr><p>C-HTTP Server</p></body></html>");
    response->body = body;
    response->body_length = strlen(body);
    
    free(date);
    
    // Cleanup
    for (int i = 0; i < count; i++)
    {
        free(keys[i]);
        free(values[i]);
    }
    free(keys);
    free(values);
    
    return response;
}

// Handler entry point
http_response_t* http_handler_handle_request(const http_request_t *request, const char *document_root, const char *database_path)
{
    if (!request || !request->method)
    {
        return NULL;
    }
    
    if (strcmp(request->method, "GET") == 0)
    {
        return handle_file_request(request, document_root, 0);
    }
    else if (strcmp(request->method, "HEAD") == 0)
    {
        return handle_file_request(request, document_root, 1);
    }
    else if (strcmp(request->method, "POST") == 0)
    {
        return handle_post_request(request, database_path);
    }
    else
    {
        // Method not implemented
        const char *not_implemented = "<html><head><title>501 Not Implemented</title></head>"
            "<body><h1>501 Not Implemented</h1><p>The method is not implemented by this server.</p>"
            "<hr><p>C-HTTP Server</p></body></html>";
        char *date;
        char *headers;
        http_response_t *response = (http_response_t *)malloc(sizeof(http_response_t));
        if (!response)
        {
            return NULL;
        }
        
        memset(response, 0, sizeof(http_response_t));
        
        response->status_code = STATUS_NOT_IMPLEMENTED;
        response->status_text = strdup("Not Implemented");
        response->body = strdup(not_implemented);
        response->body_length = strlen(not_implemented);
        
        date = http_date();
        headers = (char *)malloc(HEADER_SIZE_FAILURE);
        sprintf(headers, "Content-Type: text/html\r\nDate: %s\r\nServer: C-HTTP Server\r\n", date);
        response->headers = headers;
        
        free(date);
        return response;
    }
}

// Free response resources
void http_handler_free_response(http_response_t *response)
{
    if (response)
    {
        if (response->status_text) free(response->status_text);
        if (response->headers) free(response->headers);
        if (response->body) free(response->body);
        free(response);
    }
}
