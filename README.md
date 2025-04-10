### C HTTP Server

Advanced http server

---

## **Table of Contents**

1. [Prerequisites](#Prequisites)
2. [Installation](#Installation)
3. [Usage](#Usage)

---

## **Prerequisites**

This project uses a build system written by D'arcy Smith.

To compile using the build system you need:
- D'Arcy's libraries built from [https://github.com/programming101dev/scripts]

Tested Platforms:
- Arch Linux 2024.12.01
- Manjaro 24.2
- Ubuntu 2024.04.1
- MacOS 14.2 (clang only)

Dependencies:
- gcc or clang (Makefile specifies clang)
- make

---

## **Installation**

Clone this repository:
```sh
git clone https://github.com/kvnbanunu/c-http
```

Build with D'arcy's system:
1. Link your local scripts directory
   ```sh
   ./create-links.sh <path>
   ```
2. Generate cmakelist
   ```sh
   ./generate-cmakelists.sh
   ```
3. Change compiler to gcc or clang
   ```sh
   ./change-compiler.sh -c <gcc or clang>
   ```
4. Build with chosen compiler
   ```sh
   ./build.sh
   ```
   Build with both gcc and clang
   ```sh
   ./build-all.sh
   ```

Build the shared library:

From project root/

```sh
make lib
```

Copy the public directory into the build directory

```sh
cp -R public build/
```

---

## **Usage**

1. Change Directory
```sh
cd build/
```

2. Run server
```sh
./main
```

Run directly:
```sh
./build/server
```
