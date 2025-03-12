#include "../include/setup.h"
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct Context
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
} ctx;

static void setup();
static void setup_sig_handler();
static void sig_handler();
static void cleanup();

static volatile sig_atomic_t running = 1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

int main(void)
{
    int retval = EXIT_SUCCESS;

    exit(retval);
}
