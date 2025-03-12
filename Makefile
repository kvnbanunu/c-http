build: clean format
	@mkdir -p build
	@clang src/main.c src/setup.c src/http.c -o build/server

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
