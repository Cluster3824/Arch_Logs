#!/bin/bash
set -euo pipefail

echo "Building ArchVault..."

# Check dependencies
make check-deps

# Clean previous build
make clean

# Build with optimizations
make -j$(nproc)

echo "Build completed successfully!"
echo "Run './archlog --help' to see usage options"