#!/usr/bin/env bash

# Function to display usage information
usage()
{
    echo "Usage: $0 <source_directory>"
    exit 1
}

# Check if exactly one argument is provided
if [ "$#" -ne 1 ]; then
    usage
fi

source_dir=$1
dest_dir="$(pwd)"

# Verify that the source directory exists
if [ ! -d "$source_dir" ]; then
    echo "Error: Source directory '$source_dir' does not exist."
    exit 1
fi

# List of files to link
files_to_link=("sanitizers.txt" "supported_c_compilers.txt")

for file in "${files_to_link[@]}"; do
    source_file="$source_dir/$file"
    dest_file="$dest_dir/$file"

    if [ -e "$source_file" ]; then
        if [ ! -L "$dest_file" ]; then
            ln -s "$source_file" "$dest_file"
            echo "Linked $file to $dest_dir"
        else
            echo "$file already linked in $dest_dir. Skipping."
        fi
    else
        echo "$file not found in $source_dir. Skipping."
    fi
done

# Special handling for .flags
flags_source="$source_dir/../.flags"
if [ ! -e "$flags_source" ]; then
    flags_source="$source_dir/.flags"
fi

flags_dest="$dest_dir/.flags"

if [ -e "$flags_source" ]; then
    if [ ! -L "$flags_dest" ]; then
        ln -s "$flags_source" "$flags_dest"
        echo "Linked .flags to $dest_dir"
    else
        echo ".flags already linked in $dest_dir. Skipping."
    fi
else
    echo ".flags not found in $source_dir or its parent directory. Skipping."
fi
