CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -fPIC

# Server source files
SERVER_SRC = src/main.c src/server.c src/worker.c
SERVER_FLAGS = -ldl -lgdbm_compat
SERVER_TARGET = build/main

HANDLER_SRC = src/handler.c
HANDLER_FLAGS = -shared -lgdbm_compat -ldl
HANDLER_TARGET = build/lib_handler.so

server: format
	@mkdir -p build
	@$(CC) $(CFLAGS) $(SERVER_SRC) $(SERVER_FLAGS) -o $(SERVER_TARGET)

lib: format
	@mkdir -p build
	@$(CC) -fPIC $(HANDLER_SRC) $(HANDLER_FLAGS) -o $(HANDLER_TARGET)

query:
	@$(CC) src/query.c -o build/query

debug: format
	@mkdir -p debug/
	@clang -Wall -Wextra -Wpedantic -Wconversion src/main.c src/setup.c -o debug/server
	
clean:
	@rm -rf build/
	@rm -rf debug/

format:
	@clang-format -i -style=file include/*.h src/*.c

run:
	@./build/server
