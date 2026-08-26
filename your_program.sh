#!/bin/sh

set -e

# Go to the project root directory (where CMakeLists.txt lives)
cd "$(dirname "$0")"

# Configure and build without vcpkg
cmake -B build -S .
cmake --build ./build

# Run the shell, passing any arguments
exec ./build/shell "$@"
