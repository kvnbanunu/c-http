CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -fPIC
LDFLAGS = -lndbm -ldl

# Detect OS for platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Linux
    LDFLAGS += -ldb
endif
ifeq ($(UNAME_S),FreeBSD)
    # FreeBSD
    LDFLAGS += 
endif
ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS += 
endif

# Server source files
SERVER_SRC = main.c server.c worker.c database.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SERVER_TARGET = http_server

# Handler source files
HANDLER_SRC = handler.c
HANDLER_OBJ = $(HANDLER_SRC:.c=.o)
HANDLER_TARGET = libhandler.so

# Query utility source files
QUERY_SRC = query.c
QUERY_OBJ = $(QUERY_SRC:.c=.o)
QUERY_TARGET = db_query

# Default target
all: $(SERVER_TARGET) $(HANDLER_TARGET) $(QUERY_TARGET)

# Server target
$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Handler library target
$(HANDLER_TARGET): $(HANDLER_OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

# Query utility target
$(QUERY_TARGET): $(QUERY_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony targets
.PHONY: all clean test

# Clean target
clean:
	rm -f $(SERVER_OBJ) $(HANDLER_OBJ) $(QUERY_OBJ) $(SERVER_TARGET) $(HANDLER_TARGET) $(QUERY_TARGET)

# Test directory
test:
	mkdir -p www
	echo "<html><body><h1>Hello, World!</h1></body></html>" > www/index.html
	echo "This is a test file." > www/test.txt

# Install target
install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(SERVER_TARGET) $(DESTDIR)/usr/local/bin
	install -m 755 $(QUERY_TARGET) $(DESTDIR)/usr/local/bin
	install -d $(DESTDIR)/usr/local/lib
	install -m 644 $(HANDLER_TARGET) $(DESTDIR)/usr/local/lib
