CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -std=c99 -fPIC
LDFLAGS = -ldl -lgdbm_compat

# Platform-specific settings
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    # Linux settings
    LDFLAGS +=
else ifeq ($(UNAME), FreeBSD)
    # FreeBSD settings
    CFLAGS += -I/usr/local/include
    LDFLAGS += -L/usr/local/lib
else ifeq ($(UNAME), Darwin)
    # macOS settings
    LDFLAGS += -ldbm
endif

# Server executable
SERVER = httpserver
SERVER_OBJS = main.o server.o worker.o http_handler.o database.o utils.o

# HTTP handler shared library
HTTP_LIB = libhttp_handler.so
HTTP_LIB_OBJS = http_handler_lib.o http_handler.o database.o utils.o

# Database query tool
DB_QUERY = db_query
DB_QUERY_OBJS = db_query.o database.o utils.o

.PHONY: all clean

all: $(SERVER) $(HTTP_LIB) $(DB_QUERY)

$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(HTTP_LIB): $(HTTP_LIB_OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

$(DB_QUERY): $(DB_QUERY_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER) $(HTTP_LIB) $(DB_QUERY) *.o
	rm -f data.db data.db.pag data.db.dir

# Create necessary directories
dirs:
	mkdir -p public

# Test target to start the server
run: all dirs
	./$(SERVER) -p 8080 -w 4 -d ./www -b 10 -l ./$(HTTP_LIB)

# Dependencies
main.o: main.c config.h server.h worker.h utils.h
server.o: server.c server.h config.h utils.h
worker.o: worker.c worker.h server.h config.h http_handler.h utils.h
http_handler.o: http_handler.c http_handler.h config.h utils.h
db_manager.o: database.c database.h utils.h
utils.o: utils.c utils.h config.h
http_handler_lib.o: http_handler_lib.c http_handler.h config.h database.h utils.h
db_query.o: database.c database.h config.h
