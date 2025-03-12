#!/usr/bin/env bash

# Exit the script if any command fails
set -e

# Input and output files
input_file="files.txt"
output_file="CMakeLists.txt"
processed_sources=""
processed_headers=""
requires_sdl2=false  # Flag to track if SDL2 is required
sdl2_include_path="" # Variable to hold SDL2 include path

# Function to generate CMakeLists content
generate_cmake_content() {
    local entity="$1"
    shift
    local sources=""
    local headers=""
    local libraries=""

    for file in "$@"; do
        if [[ $file == *.c ]]; then
            if ! echo "$processed_sources" | grep -F -q "$file"; then
                sources="${sources} \${CMAKE_SOURCE_DIR}/$file"
                echo "list(APPEND SOURCES \${CMAKE_SOURCE_DIR}/$file)" >> "$output_file"
                processed_sources="$processed_sources $file"
            fi
            echo "list(APPEND ${entity}_SOURCES \${CMAKE_SOURCE_DIR}/$file)" >> "$output_file"
        elif [[ $file == *.h ]]; then
            if ! echo "$processed_headers" | grep -F -q "$file"; then
                headers="${headers} \${CMAKE_SOURCE_DIR}/$file"
                echo "list(APPEND HEADERS \${CMAKE_SOURCE_DIR}/$file)" >> "$output_file"
                processed_headers="$processed_headers $file"
            fi
            echo "list(APPEND ${entity}_HEADERS \${CMAKE_SOURCE_DIR}/$file)" >> "$output_file"
        else
            if [[ $file == "SDL2" ]]; then
                requires_sdl2=true
                sdl2_include_path="/Library/Frameworks/SDL2.framework/Headers"
            fi
            if ! echo "$libraries" | grep -F -q "$file"; then
                libraries="${libraries} $file"
            fi
        fi
    done

    # Add executable
    echo "add_executable($entity \${${entity}_SOURCES})" >> "$output_file"
    echo "target_include_directories($entity PRIVATE /usr/local/include) " >> "$output_file"
    echo "target_include_directories($entity PUBLIC \${CMAKE_SOURCE_DIR}/include)" >> "$output_file"

    # Handle libraries
    for library in $libraries; do
        if [[ $library == "SDL2" ]]; then
            echo "if(APPLE)" >> "$output_file"
            echo "    find_library(SDL2_FRAMEWORK SDL2 REQUIRED PATHS /Library/Frameworks)" >> "$output_file"
            echo "    if(SDL2_FRAMEWORK)" >> "$output_file"
            echo "        message(STATUS \"SDL2 Framework found: \${SDL2_FRAMEWORK}\")" >> "$output_file"
            echo "        target_include_directories($entity PRIVATE \"${sdl2_include_path}\")" >> "$output_file"
            echo "        target_link_options($entity PRIVATE \"-F/Library/Frameworks\")" >> "$output_file"
            echo "        target_link_libraries($entity PRIVATE \"-framework SDL2\")" >> "$output_file"
            echo "    else()" >> "$output_file"
            echo "        message(FATAL_ERROR \"SDL2.framework not found in /Library/Frameworks\")" >> "$output_file"
            echo "    endif()" >> "$output_file"
            echo "elseif(UNIX)" >> "$output_file"
            echo "    find_package(SDL2 REQUIRED)" >> "$output_file"
            echo "    target_include_directories($entity PRIVATE \${SDL2_INCLUDE_DIRS})" >> "$output_file"
            echo "    target_link_libraries($entity PRIVATE SDL2::SDL2)" >> "$output_file"
            echo "endif()" >> "$output_file"
        else
            echo "find_library(LIB_$library NAMES $library REQUIRED)" >> "$output_file"
            echo "if(LIB_$library)" >> "$output_file"
            echo "    message(STATUS \"$library found: \${LIB_$library}\")" >> "$output_file"
            echo "    target_link_libraries($entity PRIVATE \${LIB_$library})" >> "$output_file"
            echo "else()" >> "$output_file"
            echo "    message(FATAL_ERROR \"$library not found\")" >> "$output_file"
            echo "endif()" >> "$output_file"
        fi
    done
}

# Additional code
{
  # Read the first line of files.txt to determine the first target
  first_target=$(awk '{print $1; exit}' "$input_file")
  echo "cmake_minimum_required(VERSION 3.21)" > "$output_file"
  echo "" >> "$output_file"
  echo "project($first_target" >> "$output_file"
  echo "        VERSION 0.0.1" >> "$output_file"
  echo "        DESCRIPTION \"\"" >> "$output_file"
  echo "        LANGUAGES C)" >> "$output_file"
  echo "" >> "$output_file"
  echo "message(STATUS \"Compiler being used: \${CMAKE_C_COMPILER}\")" >> "$output_file"
  # Extract the compiler name without the path
  echo "get_filename_component(COMPILER_NAME \"\${CMAKE_C_COMPILER}\" NAME_WE)" >> "$output_file"
  echo "message(\"COMPILER_NAME: \${COMPILER_NAME}\")" >> "$output_file"
  echo "" >> "$output_file"

  echo "set(CMAKE_C_STANDARD 17)" >> "$output_file"
  echo "set(CMAKE_C_STANDARD_REQUIRED ON)" >> "$output_file"
  echo "set(CMAKE_C_EXTENSIONS OFF)" >> "$output_file"
  echo "" >> "$output_file"

  echo "# Check if the system uses 64-bit libraries" >> "$output_file"
  echo "get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)" >> "$output_file"
  echo "" >> "$output_file"

  echo "#Set the library suffix based on whether it's 64-bit or not" >> "$output_file"
  echo "if (\"\${LIB64}\" STREQUAL \"TRUE\")" >> "$output_file"
  echo "   set(LIBSUFFIX 64)" >> "$output_file"
  echo "else()" >> "$output_file"
  echo "    set(LIBSUFFIX \"\")" >> "$output_file"
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  # Read the file and process lines
  targets=()  # Array to store target names
  while IFS= read -r line; do
    arr=($line)
    if [ "${#arr[@]}" -ge 2 ]; then
      entity="${arr[0]}"
      files="${arr[@]:1}"
      targets+=("$entity")  # Add the target name to the array
      generate_cmake_content "$entity" $files
    fi
  done < "$input_file"

  echo "function(split_string_into_list _input_string _output_list)" >> "$output_file"
  echo "    string(REGEX REPLACE \"[ ]+\" \";\" _split_list \"\${_input_string}\")" >> "$output_file"
  echo "    set(\${_output_list} \${_split_list} PARENT_SCOPE)" >> "$output_file"
  echo "endfunction()" >> "$output_file"
  echo "" >> "$output_file"

  # Import analyzer_flags.txt
  echo "file(STRINGS \"\${CMAKE_SOURCE_DIR}/.flags/\${COMPILER_NAME}/analyzer_flags.txt\" ANALYZER_FLAGS_STRING)" >> "$output_file"
  echo "split_string_into_list(\"\${ANALYZER_FLAGS_STRING}\" ANALYZER_FLAGS_LIST)" >> "$output_file"
  echo "" >> "$output_file"

  # Import code_generation_flags.txt
  echo "file(STRINGS \"\${CMAKE_SOURCE_DIR}/.flags/\${COMPILER_NAME}/code_generation_flags.txt\" CODE_GENERATION_FLAGS_STRING)" >> "$output_file"
  echo "split_string_into_list(\"\${CODE_GENERATION_FLAGS_STRING}\" CODE_GENERATION_FLAGS_LIST)" >> "$output_file"
  echo "" >> "$output_file"

  # Import debug_flags.txt
  echo "file(STRINGS \"\${CMAKE_SOURCE_DIR}/.flags/\${COMPILER_NAME}/debug_flags.txt\" DEBUG_FLAGS_STRING)" >> "$output_file"
  echo "split_string_into_list(\"\${DEBUG_FLAGS_STRING}\" DEBUG_FLAGS_LIST)" >> "$output_file"
  echo "" >> "$output_file"

  # Import instrumentation_flags.txt
  echo "file(STRINGS \"\${CMAKE_SOURCE_DIR}/.flags/\${COMPILER_NAME}/instrumentation_flags.txt\" INSTRUMENTATION_FLAGS_STRING)" >> "$output_file"
  echo "split_string_into_list(\"\${INSTRUMENTATION_FLAGS_STRING}\" INSTRUMENTATION_FLAGS_LIST)" >> "$output_file"
  echo "" >> "$output_file"

  # Import optimization_flags.txt
  echo "file(STRINGS \"\${CMAKE_SOURCE_DIR}/.flags/\${COMPILER_NAME}/optimization_flags.txt\" OPTIMIZATION_FLAGS_STRING)" >> "$output_file"
  echo "split_string_into_list(\"\${OPTIMIZATION_FLAGS_STRING}\" OPTIMIZATION_FLAGS_LIST)" >> "$output_file"
  echo "" >> "$output_file"

  # Import warning_flags.txt
  echo "file(STRINGS \"\${CMAKE_SOURCE_DIR}/.flags/\${COMPILER_NAME}/warning_flags.txt\" WARNING_FLAGS_STRING)" >> "$output_file"
  echo "split_string_into_list(\"\${WARNING_FLAGS_STRING}\" WARNING_FLAGS_LIST)" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_address STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_address is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/address_sanitizer_flags.txt\" ADDRESS_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" ADDRESS_SANITIZER_FLAGS_LIST \"\${ADDRESS_SANITIZER_FLAGS_STRING}\")" >> "$output_file"

  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${ADDRESS_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${ADDRESS_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done

  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_cfi STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_cfi is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/cfi_sanitizer_flags.txt\" CFI_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" CFI_SANITIZER_FLAGS_LIST \"\${CFI_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${CFI_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${CFI_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_dataflow STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_dataflow is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/dataflow_sanitizer_flags.txt\" DATAFLOW_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" DATAFLOW_SANITIZER_FLAGS_LIST \"\${DATAFLOW_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${DATAFLOW_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${DATAFLOW_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_hwaddress STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_hwaddress is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/hwaddress_sanitizer_flags.txt\" HWADDRESS_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" HWADDRESS_SANITIZER_FLAGS_LIST \"\${HWADDRESS_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${HWADDRESS_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${HWADDRESS_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_leak STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_leak is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/leak_sanitizer_flags.txt\" LEAK_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" LEAK_SANITIZER_FLAGS_LIST \"\${LEAK_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${LEAK_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${LEAK_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_memory STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_memory is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/memory_sanitizer_flags.txt\" MEMORY_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" MEMORY_SANITIZER_FLAGS_LIST \"\${MEMORY_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${MEMORY_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${MEMORY_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_pointer_overflow STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_pointer_overflow is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/pointer_overflow_sanitizer_flags.txt\" POINTER_OVERFLOW_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" POINTER_OVERFLOW_SANITIZER_FLAGS_LIST \"\${POINTER_OVERFLOW_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${POINTER_OVERFLOW_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${POINTER_OVERFLOW_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_safe_stack STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_safe_stack is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/safe_stack_sanitizer_flags.txt\" SAFE_STACK_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" SAFE_STACK_SANITIZER_FLAGS_LIST \"\${SAFE_STACK_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${SAFE_STACK_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${SAFE_STACK_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_shadow_call_stack STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_shadow_call_stack is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/shadow_call_stack_sanitizer_flags.txt\" SHADOW_CALL_STACK_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" SHADOW_CALL_STACK_SANITIZER_FLAGS_LIST \"\${SHADOW_CALL_STACK_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${SHADOW_CALL_STACK_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${SHADOW_CALL_STACK_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_thread STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_thread is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/thread_sanitizer_flags.txt\" THREAD_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" THREAD_SANITIZER_FLAGS_LIST \"\${THREAD_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${THREAD_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${THREAD_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  echo "if(SANITIZER_undefined STREQUAL \"ON\")" >> "$output_file"
  echo "    message(STATUS \"SANITIZER_undefined is ON\")" >> "$output_file"
  echo "    file(STRINGS \"\${CMAKE_CURRENT_SOURCE_DIR}/.flags/\${COMPILER_NAME}/undefined_sanitizer_flags.txt\" UNDEFINED_SANITIZER_FLAGS_STRING)" >> "$output_file"
  echo "    string(REPLACE \" \" \";\" UNDEFINED_SANITIZER_FLAGS_LIST \"\${UNDEFINED_SANITIZER_FLAGS_STRING}\")" >> "$output_file"
  for entity in "${targets[@]}"; do
    echo "    target_compile_options($entity PRIVATE \${UNDEFINED_SANITIZER_FLAGS_LIST})" >> "$output_file"
    echo "    target_link_options($entity PRIVATE \${UNDEFINED_SANITIZER_FLAGS_LIST})" >> "$output_file"
  done
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"

  # Common compiler flags
  echo "set(STANDARD_FLAGS" >> "$output_file"
  echo "    -D_POSIX_C_SOURCE=200809L" >> "$output_file"
  echo "    -D_XOPEN_SOURCE=700" >> "$output_file"
  echo "    -D_GNU_SOURCE" >> "$output_file"
  echo "    -D_DARWIN_C_SOURCE" >> "$output_file"
  echo "    -D__BSD_VISIBLE" >> "$output_file"
  echo "    -Werror" >> "$output_file"
  echo ")" >> "$output_file"
  echo "" >> "$output_file"

  for entity in "${targets[@]}"; do
    echo "target_link_directories($entity PRIVATE /usr/local/lib\${LIBSUFFIX})" >> "$output_file"
    echo "target_link_options($entity PRIVATE \${INSTRUMENTATION_FLAGS_LIST})" >> "$output_file"
  done
  echo "" >> "$output_file"

  # Loop through targets and set compile options and libraries
  for target in "${targets[@]}"; do
    # Set compiler flags for the target
    echo "# Set compiler flags for the target $target" >> "$output_file"
    echo "target_compile_options($target PRIVATE" >> "$output_file"
    echo "    \${STANDARD_FLAGS}" >> "$output_file"
    echo "    \${ANALYZER_FLAGS_LIST}" >> "$output_file"
    echo "    \${DEBUG_FLAGS_LIST}" >> "$output_file"
    echo "    \${INSTRUMENTATION_FLAGS_LIST}" >> "$output_file"
    echo "    \${OPTIMIZATION_FLAGS_LIST}" >> "$output_file"
    echo "    \${WARNING_FLAGS_LIST}" >> "$output_file"
    echo ")" >> "$output_file"

    echo "# Add target_link_libraries for $target" >> "$output_file"
    echo "" >> "$output_file"
  done

  echo "if (NOT DEFINED CLANG_FORMAT_NAME)" >> "$output_file"
  echo "    set(CLANG_FORMAT_NAME \"clang-format\")" >> "$output_file"
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"
  echo "if (NOT DEFINED CLANG_TIDY_NAME)" >> "$output_file"
  echo "    set(CLANG_TIDY_NAME \"clang-tidy\")" >> "$output_file"
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"
  echo "if (NOT DEFINED CPPCHECK_NAME)" >> "$output_file"
  echo "    set(CPPCHECK_NAME \"cppcheck\")" >> "$output_file"
  echo "endif()" >> "$output_file"
  echo "" >> "$output_file"
  echo "find_program(CLANG_FORMAT NAMES \${CLANG_FORMAT_NAME} REQUIRED)" >> "$output_file"
  echo "find_program(CLANG_TIDY NAMES \${CLANG_TIDY_NAME} REQUIRED)" >> "$output_file"
  echo "find_program(CPPCHECK NAMES \${CPPCHECK_NAME} REQUIRED)" >> "$output_file"
  echo "" >> "$output_file"

  # clang-format integration
  echo "add_custom_target(format" >> "$output_file"
  echo "    COMMAND \${CLANG_FORMAT} --style=file -i \${SOURCES} \${HEADERS}" >> "$output_file"
  echo "    WORKING_DIRECTORY \${CMAKE_SOURCE_DIR}" >> "$output_file"
  echo "    COMMENT \"Running clang-format on source and header files\"" >> "$output_file"
  echo ")" >> "$output_file"
  echo "" >> "$output_file"
  echo "add_dependencies($first_target format)" >> "$output_file"
  echo "" >> "$output_file"

  # clang-tidy integration
  echo "add_custom_command(TARGET $first_target POST_BUILD" >> "$output_file"
  echo -n "    COMMAND \${CLANG_TIDY} \${SOURCES} \${HEADERS} -quiet --warnings-as-errors='*' -checks=*,-llvmlibc-restrict-system-libc-headers,-altera-struct-pack-align,-readability-identifier-length,-altera-unroll-loops,-cppcoreguidelines-init-variables,-cert-err33-c,-modernize-macro-to-enum,-bugprone-easily-swappable-parameters,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,-altera-id-dependent-backward-branch,-concurrency-mt-unsafe,-misc-unused-parameters,-hicpp-signed-bitwise,-google-readability-todo,-cert-msc30-c,-cert-msc50-cpp,-readability-function-cognitive-complexity,-clang-analyzer-security.insecureAPI.strcpy,-cert-env33-c,-android-cloexec-accept,-clang-analyzer-security.insecureAPI.rand,-misc-include-cleaner,-llvm-header-guard,-cppcoreguidelines-macro-to-enum -- \${STANDARD_FLAGS} -I\${CMAKE_SOURCE_DIR}/include -I/usr/local/include" >> "$output_file"
  if $requires_sdl2; then
    echo " -isystem${sdl2_include_path}" >> "$output_file"
  else
    echo "" >> "$output_file"
  fi
  echo "    COMMENT \"Running clang-tidy checks\"" >> "$output_file"
  echo ")" >> "$output_file"

  # Check if CMAKE_C_COMPILER starts with "clang" and add custom targets
  echo "if (CMAKE_C_COMPILER MATCHES \".*/clang.*\")" >> "$output_file"
  echo "    # Add a custom target for clang --analyze" >> "$output_file"
  echo "    add_custom_command(" >> "$output_file"
  echo "        TARGET $first_target POST_BUILD" >> "$output_file"
  echo "        COMMAND \${CMAKE_C_COMPILER} --analyzer-output text --analyze -Xclang -analyzer-checker=core --analyze -Xclang -analyzer-checker=deadcode -Xclang -analyzer-checker=security -Xclang -analyzer-disable-checker=security.insecureAPI.DeprecatedOrUnsafeBufferHandling -Xclang -analyzer-checker=unix -Xclang -analyzer-checker=unix \${CMAKE_C_FLAGS} \${STANDARD_FLAGS} -I\${CMAKE_SOURCE_DIR}/include -I/usr/local/include" >> "$output_file"
  if $requires_sdl2; then
    echo "             -isystem${sdl2_include_path}" >> "$output_file"
  else
    echo "" >> "$output_file"
  fi
  echo "             \${SOURCES} \${HEADERS}" >> "$output_file"
  echo "        WORKING_DIRECTORY \${CMAKE_SOURCE_DIR}" >> "$output_file"
  echo "        COMMENT \"Running clang --analyze\"" >> "$output_file"
  echo "    )" >> "$output_file"
  echo "" >> "$output_file"
  echo "    # Add a custom command to delete .gch files after the analysis" >> "$output_file"
  echo "    add_custom_command(" >> "$output_file"
  echo "        TARGET $first_target POST_BUILD" >> "$output_file"
  echo "        COMMAND \${CMAKE_COMMAND} -E remove \${CMAKE_SOURCE_DIR}/include/*.gch" >> "$output_file"
  echo "        COMMENT \"Removing .gch files\"" >> "$output_file"
  echo "    )" >> "$output_file"
  echo "endif ()" >> "$output_file"
  echo "" >> "$output_file"

  # Add a custom target for cppcheck
  echo "add_custom_command(" >> "$output_file"
  echo "    TARGET $first_target POST_BUILD" >> "$output_file"
  echo "    COMMAND \${CPPCHECK} --error-exitcode=1 --force --quiet --inline-suppr --check-level=exhaustive --library=posix --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=checkersReport -I\${CMAKE_SOURCE_DIR}/include -I/usr/local/include \${SOURCES} \${HEADERS}" >> "$output_file"
  echo "    WORKING_DIRECTORY \${CMAKE_SOURCE_DIR}" >> "$output_file"
  echo "    COMMENT \"Running cppcheck\"" >> "$output_file"
  echo ")" >> "$output_file"
  echo "" >> "$output_file"

  # Add a custom target for cppcheck
  echo "add_custom_target(clean_gcda" >> "$output_file"
  echo "      COMMAND find . -name "*.gcda" -exec rm -f {} +" >> "$output_file"
  echo "      COMMENT "Cleaning up old .gcda files"" >> "$output_file"
  echo ")" >> "$output_file"
  echo "add_dependencies($first_target clean_gcda)" >> "$output_file"
}

exit $?
