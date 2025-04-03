#include "../include/handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <ndbm.h>

#define TIME_SIZE 128
#define STATUS_OK 200
#define STATUS_NOT_FOUND 404
#define HEADER_SIZE_FAIL 256
#define HEADER_SIZE_SUCCESS 512
#define HEX 16

const char *handler_version = "1.0.0"; // increment when updating

static char* get_mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');

    if(!ext)
    {
        return "application/octet-stream";
    }

    if(strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
    {
        return "text/html";
    }

    if(strcmp(ext, ".txt") == 0)
    {
        return "text/plain";
    }

    if(strcmp(ext, ".css") == 0)
    {
        return "text/css";
    }

    if(strcmp(ext, ".js") == 0)
    {
        return "application/javascript";
    }

    if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }

    if(strcmp(ext, ".png") == 0)
    {
        return "image/png";
    }

    if(strcmp(ext, ".gif") == 0)
    {
        return "image/gif";
    }

    if(strcmp(ext, ".ico") == 0)
    {
        return "image/x-icon";
    }

    if(strcmp(ext, ".pdf") == 0)
    {
        return "application/pdf";
    }

    if(strcmp(ext, ".xml") == 0)
    {
        return "application/xml";
    }

    if(strcmp(ext, ".json") == 0)
    {
        return "application/json";
    }
    
    return "application/octet-stream";
}

static char* http_date()
{
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    char *buf = malloc(TIME_SIZE);
    if(!buf)
    {
        return NULL;
    }
    strftime(buf, TIME_SIZE, "%a, %d %b %Y %H:%M:%S GMT", tm);
    return buf;
}

static response_t* handle_file_request(const request_t *req, const char *doc_root, int head_only)
{
    response_t *res;
    struct stat file_stat;
    int fd;
    const char *mime_type = NULL;
    char *uri;
    char *query_str;
    char *file_path;
    char *date;
    char *headers;

    res = (response_t *)malloc(sizeof(response_t));
    if(!res)
    {
        return NULL;
    }
    memset(res, 0, sizeof(response_t));
    
    // sanitize URI to prevent directory traversal
    uri = strdup(req->uri);
    if(!uri)
    {
        free(res);
        return NULL;
    }

    // remove query string if present
    query_str = strchr(uri, '?');
    if(query_str)
    {
        *query_str = '\0';
    }

    // Default to index.html for root
    if(strcmp(uri, "/") == 0)
    {
        free(uri);
        uri = strdup("/index.html");
        if(!uri)
        {
            free(res);
            return NULL;
        }
    }

    // Construct file path
    file_path = (char *)malloc(strlen(doc_root) + strlen(uri) + 1);
    if(!file_path)
    {
        free(uri);
        free(res);
        return NULL;
    }
    sprintf(file_path, "%s%s", doc_root, uri);

    fd = open(file_path, O_RDONLY);
    if(fd < 0)
    {
        res->status_code = STATUS_NOT_FOUND;
        res->status_text = strdup("Not Found");

        const char *not_found = "<html><head><title>404 Not Found</title></head>"
                                "<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p>"
                                "<hr><p>C-HTTP Server</p></body></html>";
        res->body = strdup(not_found);
        res->body_len = strlen(not_found);

        date = http_date();
        headers = (char *)malloc(HEADER_SIZE_FAIL);
        sprintf(headers, "Content-Type: text/html\r\nDate: %s\r\nServer: C HTTP Server\r\n", date);
        res->headers = headers;

        free(date);
        free(uri);
        free(file_path);
        return res;
    }

    // get file info
    if(fstat(fd, &file_stat) < 0)
    {
        close(fd);
        free(uri);
        free(file_path);
        free(res);
        return NULL;
    }

    res->status_code = STATUS_OK;
    res->status_text = strdup("OK");
    date = http_date();
    
    mime_type = get_mime_type(file_path);
    headers = (char *)malloc(HEADER_SIZE_SUCCESS);
    sprintf(headers, "Content-Type: %s\r\nDate: %s\r\nServer: C-HTTP Server\r\nContent-Length: %ld\r\n", mime_type, date, file_stat.st_size);
    res->headers = headers;
    
    free(date);

    // HEAD requests
    if(head_only)
    {
        res->body = NULL;
        res->body_len = 0;
    }
    else
    {
        ssize_t bytes_read;
        res->body = (char *)malloc(file_stat.st_size);
        if(!res->body)
        {
            close(fd);
            free(uri);
            free(file_path);
            free(res->status_text);
            free(res->headers);
            free(res);
            return NULL;
        }
        
        bytes_read = read(fd, res->body, file_stat.st_size);
        if(bytes_read != file_stat.st_size)
        {
            close(fd);
            free(uri);
            free(file_path);
            free(res->status_text);
            free(res->headers);
            free(res->body);
            free(res);
            return NULL;
        }
        res->body_len = file_stat.st_size;
    }
    close(fd);
    free(uri);
    free(file_path);
    return res;
}

static char* url_decode(const char *str)
{
    char *decoded;
    char *p;
    if(str == NULL)
    {
        return NULL;
    }
    
    decoded = (char *)malloc(strlen(str) + 1);
    if(decoded == NULL)
    {
        return NULL;
    }

    p = decoded;
    while(*str)
    {
        if(*str == '%' && str[1] && str[2]) // handle %
        {
            char hex[3] = { str[1], str[2], 0 };
            *p++ = (char)strtol(hex, NULL, HEX);
            str += 3;
        }
        else if(*str == '+') // handle space
        {
            *p++ ' ';
            str++;
        }
        else
        {
            *p++ = *str++
        }
    }
    *p = '\0';
    return decoded;
}

int parse_post_data(const char *body, size_t body_len, char ***keys, char ***values, int *count)
{
    char *body_copy;
    char *saveptr;
    char *pair;
    int pairs = 1;
    int index = 0;

    if(!body || body_len == 0)
    {
        *count = 0;
        return 0;
    }

    body_copy = (char *)malloc(body_len + 1);
    if(!body_copy)
    {
        return -1;
    }
    memcpy(body_copy, body, body_len);
    body_copy[body_len] = '\0';

    // Count the number of '&' to determine num pairs
    for(size_t i = 0; i < body_len; i++)
    {
        if(body_copy[i] == '&')
        {
            pairs++;
        }
    }

    *keys = (char**)malloc(pairs * sizeof(char*));
    *values = (char**)malloc(pairs * sizeof(char*));
    if(!*keys || !*values)
    {
        // prevent double free
        if(*keys)
        {
            free(*keys);
        }
        if(*values)
        {
            free(*values);
        }
        free(body_copy);
        return -1;
    }

    // init arr
    for(int i = 0; i < pairs; i++)
    {
        (*keys)[i] = NULL;
        (*values)[i] = NULL;
    }

    // parse each pair
    pair = strtok_r(body_copy, "&", &saveptr);
    while(pair && index < pairs)
    {
        char *eq = strchr(pair, '='); // find equal sign
        if(eq)
        {
            char *key;
            char *val;
            *eq = '\0';
            key = url_decode(pair);
            val = url_decode(eq + 1);

            if(!key || !val)
            {
                if(key)
                {
                    free(key);
                }
                if(val)
                {
                    free(val);
                }
                // clean everything
                for(int i = 0; i < index; i++)
                {
                    if((*keys)[i])
                    {
                        free((*keys)[i]);
                        free((*values)[i]);
                    }
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
        // next pair
        pair = strtok_r(NULL, "&", &saveptr);
    }
    *count = index;
    free(body_copy);
    return 0;
}

static response_t* handle_post_request()
{

}
