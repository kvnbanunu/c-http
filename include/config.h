#ifndef CONFIG_H
#define CONFIG_H

#define PORT 8080
#define WORKER_COUNT 3
#define HANDLER_LIBRARY "./lib_handler.so"

#define WORKER_SIGTERM_TIMEOUT 5
#define WORKER_SLEEP 100000 // 100ms

#endif // CONFIG_H
