#include "../include/handler.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define FMT_BUFFER 50
#define HANDLER_VERSION "5.3.4"

void init_handler(void)
{
    printf("Initialized Handler version: %s\n", HANDLER_VERSION);
}

static void construct_response(int clientfd, const char *status, const char *body, const char *mime, size_t body_len)
{
    char response[BUFFER_SIZE];

    snprintf(response,
             sizeof(response),
             "HTTP/1.0 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n\r\n",
             status,
             mime,
             body_len);

    printf("%s", response);
    write(clientfd, response, strlen(response));

    if(body != NULL)
    {
        write(clientfd, body, body_len);
    }
}

static void trim_protocol(char *protocol)
{
    for(int i = 0; protocol[i] != '\0'; i++)
    {
        if(protocol[i] == '\r' || protocol[i] == '\n')
        {
            protocol[i] = '\0';
            break;
        }
    }
}

static void parse_request(struct req_info *info, char *buffer)
{
    char *save;

    info->method   = strtok_r(buffer, " ", &save);
    info->path     = strtok_r(NULL, " ", &save);
    info->protocol = strtok_r(NULL, " ", &save);
    if(info->protocol)
    {
        trim_protocol(info->protocol);
    }

    printf("PARSED METHOD: %s\n", info->method);
    printf("PARSED PATH: %s\n", info->path);
    printf("PARSED PROTOCOL: %s\n", info->protocol);
}

static const char *get_mime_type(const char *file_path)
{
    const char *ext = strrchr(file_path, '.');
    if(ext == NULL)
    {
        return "application/octet-stream";
    }

    if(strcmp(ext, ".html") == 0)
    {
        return "text/html";
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

    return "application/octet-stream";
}

static int file_readable(const char *file_path)
{
    struct stat file_stat;

    if(stat(file_path, &file_stat) == 0 && (file_stat.st_mode & S_IRUSR))
    {
        return 0;
    }

    return -1;
}

static int file_exists(const char *file_path)
{
    struct stat file_stat;

    if(stat(file_path, &file_stat) == 0)
    {
        return 0;
    }

    return -1;
}

static int file_verification(char *file_path)
{
    int  retval;
    int  a;
    int  b;
    char file_path_formatted[FMT_BUFFER];

    snprintf(file_path_formatted, FMT_BUFFER, ".%s", file_path);

    a      = file_readable(file_path_formatted);
    b      = file_exists(file_path_formatted);
    retval = 0;

    // 404 case
    if(b != 0)
    {
        retval = -1;
        return retval;
    }

    // 403 case
    if(a != 0)
    {
        retval = -2;
        return retval;
    }

    return retval;
}

static int open_requested_file(char *path)    // removed fd pointer -> now return fd
{
    int  fd;
    char file_path_formatted[FMT_BUFFER];

    snprintf(file_path_formatted, FMT_BUFFER, ".%s", path);

    fd = open(file_path_formatted, O_RDONLY | O_CLOEXEC);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }
    return fd;
}

static size_t find_content_length(int fd)
{
    struct stat fileStat;

    if(fstat(fd, &fileStat) == 0)
    {
        return (size_t)fileStat.st_size;
    }
    return (size_t)-1;
}

static void construct_get_response400(int clientfd)
{
    const char body[] = "<html><body><h1>400 Bad Request</h1></body></html>";
    construct_response(clientfd, "400 Not Found", body, "text/html", strlen(body));
}

static void construct_get_response404(int clientfd)
{
    const char body[] = "<html><body><h1>404 Not Found</h1></body></html>";
    construct_response(clientfd, "404 Not Found", body, "text/html", strlen(body));
}

static void construct_get_response405(int clientfd)
{
    const char body[] = "<html><body><h1>405 Unknown Method</h1></body></html>";
    construct_response(clientfd, "405 Unknown Method", body, "text/html", strlen(body));
}

static void construct_get_response403(int clientfd)
{
    const char body[] = "<html><body><h1>403 Forbidden</h1></body></html>";
    construct_response(clientfd, "403 Forbidden", body, "text/html", strlen(body));
}

static void construct_get_response500(int clientfd)
{
    const char body[] = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    construct_response(clientfd, "500 Internal Server Error", body, "text/html", strlen(body));
}

static void construct_get_response200(int clientfd, const char *mime, int filefd)
{
    size_t  fileSize;
    char   *buffer;
    ssize_t bytesread;

    fileSize = find_content_length(filefd);

    buffer = (char *)malloc(sizeof(char) * fileSize);

    bytesread = read(filefd, buffer, fileSize);

    if(bytesread < 0)
    {
        construct_get_response404(clientfd);
        free(buffer);
    }
    else
    {
        construct_response(clientfd, "200 OK", buffer, mime, fileSize);
        free(buffer);
    }
}

static void get_body_pos(char buffer[BUFFER_SIZE], char **bodyptr)
{
    const char *key;

    key = "\r\n\r\n";

    *bodyptr = strstr(buffer, key);

    if(*bodyptr)
    {
        *bodyptr += 4;
    }
}

static int store_string(DBM *db, const char *key, const char *value)
{
    const_datum key_datum   = MAKE_CONST_DATUM(key);
    const_datum value_datum = MAKE_CONST_DATUM(value);

    return dbm_store(db, *(datum *)&key_datum, *(datum *)&value_datum, DBM_REPLACE);
}

static void store_in_db(const char *key, const char *value)
{
    char db_name[] = "posts_db";    // cppcheck-suppress constVariable
    DBM *db        = dbm_open(db_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(!db)
    {
        perror("dbm_open");
        return;
    }

    if(store_string(db, key, value) != 0)
    {
        perror("store_string");
        dbm_close(db);
        return;
    }
    dbm_close(db);
}

static int tokenize_post(char *body)
{
    const char *key;
    const char *value;
    char       *save;

    key   = strtok_r(body, "=", &save);
    value = strtok_r(NULL, "=", &save);

    if(key == NULL || value == NULL)
    {
        printf("Invalid post body structure\n");
        return -1;
    }

    store_in_db(key, value);
    return 0;
}

void handle_request(int client_fd)
{
    struct req_info info;
    char            buffer[BUFFER_SIZE];
    char to_parse[BUFFER_SIZE];
    ssize_t         valread;

    memset(buffer, 0, BUFFER_SIZE);
    memset(to_parse, '\0', BUFFER_SIZE);
    valread = read(client_fd, buffer, BUFFER_SIZE);
    if(valread < 0)
    {
        // handle reading error
        perror("handle_request: read\n");
        return;
    }

    strncpy(to_parse, buffer, BUFFER_SIZE - 1);
    printf("INCOMING:\n%s\nEND BUFFER\n", buffer);
    // parse
    parse_request(&info, to_parse);
    // check for get, head, post
    if(strcmp("GET", info.method) == 0)
    {
        // size_t      content_length;
        const char *mime;
        int         requested_fd;
        int         verification;

        verification = file_verification(info.path);

        // error handle if path is not real and stuff
        if(verification == -1)
        {
            // send 404 error back to client
            construct_get_response404(client_fd);
            return;
        }

        else if(verification == -2)
        {
            // send 404 error back to client
            construct_get_response403(client_fd);
            return;
        }

        // handle
        requested_fd = open_requested_file(info.path);
        if(requested_fd < 0)
        {
            construct_get_response500(client_fd);
            return;
        }

        mime = get_mime_type(info.path);
        // content_length = find_content_length(clientfd);
        // construct_response(clientfd, "200 OK", buffer, mime, content_length);
        construct_get_response200(client_fd, mime, requested_fd);
    }

    else if(strcmp("HEAD", info.method) == 0)
    {
        int verification;

        verification = file_verification(info.path);

        // error handle if path is not real and stuff
        if(verification == -1)
        {
            // send 404 error back to client
            construct_get_response404(client_fd);
            return;
        }

        else if(verification == -2)
        {
            // send 404 error back to client
            construct_get_response403(client_fd);
            return;
        }

        // handle head
        construct_response(client_fd, "200 OK", NULL, "text/html", 0);
    }

    else if(strcmp("POST", info.method) == 0)
    {
        // handle post
        // int   content_len;
        char *bodyptr;

        // content_len = get_content_length(buffer);
        get_body_pos(buffer, &bodyptr);
        if(tokenize_post(bodyptr) < 0)
        {
            construct_get_response400(client_fd);
            return;
        }
        construct_response(client_fd, "200 OK", NULL, "text/html", 0);
        return;
    }

    else
    {
        // handler error
        construct_get_response405(client_fd);
    }
}
