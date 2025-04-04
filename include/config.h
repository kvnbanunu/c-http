#ifndef CONFIG_H
#define CONFIG_H

#define PORT 8080
#define WORKER_COUNT 3
#define HANDLER_LIBRARY "./lib_handler.so"
#define DOCUMENT_ROOT "../public"

#define WORKER_SIGTERM_TIMEOUT 5
#define WORKER_SLEEP 100000000    // 100ms in nanosecs

#endif    // CONFIG_H
