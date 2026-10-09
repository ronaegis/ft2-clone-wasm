#!/bin/bash

# FastTracker 2 WASM Build Script
# This script sets up Emscripten and builds the WASM version

set -e

# Emscripten version the build is tested with (keep in sync with .github/workflows/wasm.yml)
EMSDK_VERSION=5.0.2

echo "=== FastTracker 2 WASM Build Script ==="

# Check if we're in the right directory
if [ ! -f "../../src/ft2_main.c" ]; then
    echo "Error: Please run this script from the wasm/build directory"
    exit 1
fi

# Check if Emscripten is installed
if ! command -v emcc &> /dev/null; then
    echo "Emscripten not found. Installing Emscripten..."

    # Install Emscripten using emsdk
    if [ ! -d "emsdk" ]; then
        git clone https://github.com/emscripten-core/emsdk.git
    fi

    cd emsdk
    ./emsdk install "$EMSDK_VERSION"
    ./emsdk activate "$EMSDK_VERSION"
    source ./emsdk_env.sh
    cd ..

    echo "Emscripten installed successfully"
else
    echo "Emscripten found"
fi

# Set up Emscripten environment
if [ -f "emsdk/emsdk_env.sh" ]; then
    source emsdk/emsdk_env.sh
fi

# Configure with CMake
echo "Configuring with CMake..."
cmake . -DCMAKE_TOOLCHAIN_FILE=Toolchain-emscripten.cmake

# Build the project
echo "Building WASM module..."
make ft2-wasm

echo "Build completed successfully!"
echo "Output files:"
ls -la ../web/

echo ""
echo "To serve the web version:"
echo "  cd ../web"
echo "  python3 -m http.server 8000"
echo ""
echo "Then open http://localhost:8000 in your browser"