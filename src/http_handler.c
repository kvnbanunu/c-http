#include "../include/http_handler.h"
#include "../include/config.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Parse a line from the HTTP request
 * @param line Request line
 * @param request Pointer to http_request_t structure
 * @return 0 on success, -1 on failure
 */
static int parse_request_line(const char *line, http_request_t *request)
{
    const char *uri_start;
    const char *uri_end;
    const char *version_start;
    size_t      method_len;
    size_t      uri_len;
    size_t      version_len;

    // Find the first space (after method)
    uri_start = strchr(line, ' ');
    if(!uri_start)
    {
        return -1;
    }

    // Skip the space
    uri_start++;

    // Find the second space (after URI)
    uri_end = strchr(uri_start, ' ');
    if(!uri_end)
    {
        return -1;
    }

    // Get the HTTP version
    version_start = uri_end + 1;

    // Calculate lengths
    method_len  = (size_t)(uri_start - line - 1);
    uri_len     = (size_t)(uri_end - uri_start);
    version_len = strlen(version_start);

    // Check lengths
    if(method_len >= sizeof(request->method) || uri_len >= sizeof(request->uri) || version_len >= sizeof(request->version))
    {
        return -1;
    }

    // Copy values
    memcpy(request->method, line, method_len);
    request->method[method_len] = '\0';

    memcpy(request->uri, uri_start, uri_len);
    request->uri[uri_len] = '\0';

    memcpy(request->version, version_start, version_len);
    request->version[version_len] = '\0';

    return 0;
}

int parse_http_request(const char *buffer, size_t length, http_request_t *request)
{
    const char *line_start;
    const char *line_end;
    const char *header_end;
    const char *body_start;
    char        line[MAX_HEADER_SIZE];
    size_t      line_len;
    int         content_length = 0;
    int         result         = -1;

    if(!buffer || !request || length == 0)
    {
        return -1;
    }

    memset(request, 0, sizeof(http_request_t));

    // Find end of headers
    header_end = strstr(buffer, "\r\n\r\n");
    if(!header_end)
    {
        return -1;
    }

    // Body starts after headers
    body_start = header_end + 4;

    // Parse request line
    line_start = buffer;
    line_end   = strstr(line_start, "\r\n");
    if(!line_end)
    {
        return -1;
    }

    line_len = (size_t)(line_end - line_start);
    if(line_len >= sizeof(line))
    {
        return -1;
    }

    memcpy(line, line_start, line_len);
    line[line_len] = '\0';

    if(parse_request_line(line, request) < 0)
    {
        return -1;
    }

    // Parse headers
    line_start = line_end + 2;
    while(line_start < header_end)
    {
        line_end = strstr(line_start, "\r\n");
        if(!line_end)
        {
            break;
        }

        line_len = (size_t)(line_end - line_start);
        if(line_len >= sizeof(line))
        {
            return -1;
        }

        memcpy(line, line_start, line_len);
        line[line_len] = '\0';

        // Look for Content-Length header
        if(strncasecmp(line, "Content-Length:", 15) == 0)
        {
            const char *val = line + 15;
            while(*val && isspace((unsigned char)*val))
            {
                val++;
            }
            content_length          = atoi(val);
            request->content_length = content_length;
        }

        // Append header to request headers
        if(strlen(request->headers) + line_len + 3 < sizeof(request->headers))
        {
            strcat(request->headers, line);
            strcat(request->headers, "\r\n");
        }

        line_start = line_end + 2;
    }

    // Copy request body if present
    if(content_length > 0)
    {
        size_t body_length = length - (size_t)(body_start - buffer);
        if(body_length > 0)
        {
            if(body_length > (size_t)content_length)
            {
                body_length = (size_t)content_length;
            }

            if(body_length < sizeof(request->body))
            {
                memcpy(request->body, body_start, body_length);
                request->body[body_length] = '\0';
                result                     = 0;
            }
        }
    }
    else
    {
        result = 0;
    }

    return result;
}

void free_http_response(http_response_t *response)
{
    if(!response)
    {
        return;
    }

    free(response->body);
    response->body        = NULL;
    response->body_length = 0;
}
