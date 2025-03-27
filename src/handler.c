#include "../include/handler.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void *handle_request(void *arg)
{
    int             clientfd;
    int             requestedfd;
    struct req_info info;
    char            buffer[BUFFER_SIZE];
    ssize_t         valread;

    clientfd = *(int *)arg;
    valread  = read(clientfd, buffer, BUFFER_SIZE);
    if(valread < 0)
    {
        // handle reading error
        perror("read");
        pthread_exit(NULL);
    }

    // parse
    parse_request(&info, buffer);

    // check for get, head, post
    if(strcmp("GET", info.method) == 0)
    {
        // size_t      content_length;
        const char *mime;

        printf("%s\n", info.path);
        // error handle if path is not real and stuff
        if(file_verification(info.path) == -1)
        {
            // send 404 error back to client
            construct_get_response404(clientfd);
            return NULL;
        }

        printf("opening requested file..\n");
        // handle
        if(open_requested_file(&requestedfd, info.path) == -1)
        {
            fprintf(stderr, "Failed to open requested file");
            pthread_exit(NULL);
        }

        mime = get_mime_type(info.path);
        // content_length = find_content_length(clientfd);
        // construct_response(clientfd, "200 OK", buffer, mime, content_length);
        construct_get_response200(clientfd, mime, requestedfd);
    }

    else if(strcmp("HEAD", info.method) == 0)
    {
        // error handle if path is not real and stuff
        if(file_verification(info.path) == -1)
        {
            // send 404 back to client
            construct_response(clientfd, "404", NULL, "text/html", 0);
            exit(EXIT_FAILURE);
        }

        // handle head
        construct_response(clientfd, "200 OK", NULL, "text/html", 0);
    }

    // else if(strcmp("POST", info.method) == 0)
    // {
    //     //handle post
    // }

    // else
    // {
    //     //handler error
    // }

    exit(EXIT_SUCCESS);
}

void construct_response(int clientfd, const char *status, const char *body, const char *mime, size_t body_len)
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

void parse_request(struct req_info *info, char *buffer)
{
    char *save;

    info->method   = strtok_r(buffer, " ", &save);
    info->path     = strtok_r(NULL, " ", &save);
    info->protocol = strtok_r(NULL, " ", &save);
    if(info->protocol)
    {
        trim_protocol(info->protocol);
    }
}

void trim_protocol(char *protocol)
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

const char *get_mime_type(const char *file_path)
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

int file_verification(const char *file_path)
{
    int  retval;
    int  a;
    int  b;
    char file_path_formatted[BUFFER_SIZE];

    snprintf(file_path_formatted, sizeof(file_path_formatted), ".%s", file_path);

    a      = file_readable(file_path_formatted);
    b      = file_exists(file_path_formatted);
    retval = -1;

    printf("%s\n", file_path_formatted);
    printf("readable: %d exists: %d\n", a, b);

    if(file_readable(file_path_formatted) == 0 && file_exists(file_path_formatted) == 0)
    {
        retval = 0;
    }

    return retval;
}

int open_requested_file(int *fd, const char *path)
{
    char file_path_formatted[BUFFER_SIZE];

    snprintf(file_path_formatted, sizeof(file_path_formatted), ".%s", path);

    *fd = open(file_path_formatted, O_RDONLY | O_CLOEXEC);
    if(*fd < 0)
    {
        perror("open");
        return -1;
    }
    return 0;
}

int file_readable(const char *file_path)
{
    struct stat file_stat;

    if(stat(file_path, &file_stat) == 0 && (file_stat.st_mode & S_IRUSR))
    {
        return 0;
    }

    return -1;
}

int file_exists(const char *file_path)
{
    struct stat file_stat;

    if(stat(file_path, &file_stat) == 0)
    {
        return 0;
    }

    return -1;
}

size_t find_content_length(int fd)
{
    struct stat fileStat;

    if(fstat(fd, &fileStat) == 0)
    {
        return (size_t)fileStat.st_size;
    }
    return (size_t)-1;
}

void linktest(void)
{
    printf("link success\n");
}

void construct_get_response404(int clientfd)
{
    const char body[] = "<html><body><h1>404 Not Found</h1></body></html>";
    construct_response(clientfd, "404 Not Found", body, "text/html", strlen(body));
}

void construct_get_response200(int clientfd, const char *mime, int filefd)
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
