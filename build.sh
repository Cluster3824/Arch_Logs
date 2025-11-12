#!/bin/bash

# ArchLog Build Script
# Builds both CLI and GUI versions of ArchLog

set -e

echo "🔨 Building ArchLog..."

# Clean previous builds
echo "🧹 Cleaning previous builds..."
make clean 2>/dev/null || true
make -f Makefile.gui clean 2>/dev/null || true

# Build CLI version
echo "⚙️ Building CLI version..."
make archlog

# Build GUI version (check if GTK is available)
if pkg-config --exists gtk+-3.0; then
    echo "🖥️ Building GUI version..."
    make -f Makefile.gui gui
    echo "✅ Both CLI and GUI versions built successfully!"
    echo ""
    echo "Usage:"
    echo "  CLI: ./archlog --help"
    echo "  GUI: ./archlog-gui"
else
    echo "⚠️ GTK+3.0 not found. Only CLI version built."
    echo ""
    echo "Usage:"
    echo "  CLI: ./archlog --help"
fi

echo ""
echo "📁 Built files:"
ls -la archlog* | grep -v "\.desktop\|\.sh"