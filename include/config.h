#ifndef CONFIG_H
#define CONFIG_H

/* Server configuration constants */
#define PORT 8080
#define WORKER_COUNT 3
#define BACKLOG 10
#define DOCUMENT_ROOT "./public"
#define DB_PATH "./data.db"

/* Buffer size constants */
#define MAX_REQUEST_SIZE 8192
#define MAX_URI_LENGTH 2048
#define MAX_HEADER_SIZE 4096
#define MAX_PATH_LENGTH 1024

/* HTTP handler library constants */
#define HTTP_HANDLER_LIB "./libhttp_handler.so"
#define HTTP_HANDLER_FUNC "handle_http_request"

/* HTTP Status codes */
#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_INTERNAL_SERVER_ERROR 500

/* HTTP Methods */
#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_HEAD "HEAD"
#define HTTP_METHOD_POST "POST"

#endif /* CONFIG_H */
