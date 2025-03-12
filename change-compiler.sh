#!/usr/bin/env bash

c_compiler=""
clang_format_name="clang-format"
clang_tidy_name="clang-tidy"
cppcheck_name="cppcheck"
sanitizers=""
sanitizers_passed=false

# Function to display script usage
usage()
{
    echo "Usage: $0 -c <c compiler> [-f <clang-format>] [-t <clang-tidy>] [-k <cppcheck>] [-s <sanitizers>]"
    echo "  -c c compiler   Specify the C compiler name (e.g. gcc or clang)"
    echo "  -f clang-format   Specify the clang-format name (e.g. clang-format or clang-format-17)"
    echo "  -t clang-tidy     Specify the clang-tidy name (e.g. clang-tidy or clang-tidy-17)"
    echo "  -k cppcheck       Specify the cppcheck name (e.g. cppcheck)"
    echo "  -s sanitizers     Specify the sanitizers to use (e.g. address,undefined; empty for none)"
    exit 1
}

# Parse command-line options using getopt
while getopts ":c:f:t:k:s:" opt; do
  case $opt in
    c)
      c_compiler="$OPTARG"
      ;;
    f)
      clang_format_name="$OPTARG"
      ;;
    t)
      clang_tidy_name="$OPTARG"
      ;;
    k)
      cppcheck_name="$OPTARG"
      ;;
    s)
      sanitizers="$OPTARG"
      sanitizers_passed=true
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      usage
      ;;
  esac
done

# Check if the compiler argument is provided
if [ -z "$c_compiler" ]; then
  echo "Error: C compiler argument (-c) is required."
  usage
fi

# Ensure supported_c_compilers.txt exists
if [ ! -f "supported_c_compilers.txt" ]; then
    echo "Error: supported_c_compilers.txt not found."
    exit 1
fi

# Ensure the compiler is listed in supported_c_compilers.txt
if ! grep -Fxq "$c_compiler" supported_c_compilers.txt; then
    echo "Error: The specified compiler '$c_compiler' is not in supported_c_compilers.txt."
    echo "Supported compilers:"
    cat supported_c_compilers.txt
    exit 1
fi

if [ ! -d "./.flags" ]; then
    echo "Error: .flags directory does not exist. Please run generate-flags.sh to create it."
    exit 1
fi

./check-env.sh -c "$c_compiler" -f "$clang_format_name" -t "$clang_tidy_name" -k "$cppcheck_name"

if [ ! -d "./.flags/$c_compiler" ]; then
    ./generate-flags.sh
fi

# Read sanitizers from sanitizers.txt if -s was not passed
if ! $sanitizers_passed; then
    if [ -f "sanitizers.txt" ]; then
        sanitizers=$(tr -d ' \n' < sanitizers.txt)  # Remove spaces and newlines
        echo "Sanitizers loaded from sanitizers.txt: $sanitizers"
    else
        echo "Error: sanitizers.txt not found and no sanitizers provided via -s option."
        exit 1
    fi
else
    echo "Sanitizers specified via command-line: $sanitizers"
fi

# Split the sanitizers string and construct flags
IFS=',' read -ra SANITIZERS <<< "$sanitizers"
sanitizer_flags=""
for sanitizer in "${SANITIZERS[@]}"; do
    sanitizer_flags+="-DSANITIZER_${sanitizer}=ON "
done

rm -rf build/CMakeCache.txt
cmake -S . -B build -DCMAKE_C_COMPILER="$c_compiler" -DCLANG_FORMAT_NAME="$clang_format_name" -DCLANG_TIDY_NAME="$clang_tidy_name" -DCPPCHECK_NAME="$cppcheck_name" $sanitizer_flags -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_SYSROOT=""
