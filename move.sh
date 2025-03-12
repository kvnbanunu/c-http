#!/usr/bin/env bash

# Default values
dest_dir=""

# Function to display usage information
usage()
{
    echo "Usage: $0 <destination_directory>"
    exit 1
}

# Check if exactly one argument is provided
if [ "$#" -ne 1 ]; then
    usage
fi

dest_dir=$1

# Construct the source directory path
source_dir="."

# Check if the destination directory exists; if not, create it
# TODO: don't overwrite?
if [ ! -d "$dest_dir" ]; then
    mkdir -p "$dest_dir"
fi

# List of files and directories to copy (excluding .flags)
files_to_copy=(".clang-format" ".gitignore" "build.sh" "build-all.sh" "change-compiler.sh" "check-compilers.sh" "check-env.sh" "create-links.sh" "files.txt" "generate-cmakelists.sh" "generate-flags.sh" "move.sh" "README.md" "src" "include")

# Copy files and directories to the destination directory
for item in "${files_to_copy[@]}"; do
    source_item="$source_dir/$item"
    dest_item="$dest_dir/$item"

    if [ -e "$source_item" ]; then
        if [ ! -e "$dest_item" ]; then
            cp -a "$source_item" "$dest_item"
            echo "Copied $item to $dest_dir"
        else
            echo "$item already exists in $dest_dir. Skipping."
        fi
    else
        echo "$item not found in the template directory. Skipping."
    fi
done

echo "Copy operation complete."

# Navigate to the destination directory
pushd "$dest_dir" || exit

# Return to the original directory
popd || exit
