#!/bin/bash

# Comprehensive build script for ropeless_pi plugin on Ubuntu 22.04
# Based on instructions from BUILDING.md

set -e  # Exit on any error

echo "=== Building ropeless_pi plugin for Ubuntu 22.04 ==="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the plugin root directory."
    exit 1
fi

# Install basic build tools if not present
echo "Checking for required tools..."
if ! command -v git &> /dev/null || ! command -v cmake &> /dev/null; then
    echo "Installing basic build tools..."
    sudo apt update
    sudo apt install -y git cmake
fi

# Initialize submodules if they haven't been initialized
echo "Initializing submodules..."
if [ ! -d "opencpn-libs/.git" ]; then
    git submodule update --init opencpn-libs
fi

# Install build dependencies
echo "Installing build dependencies..."
if ! dpkg -l | grep -q devscripts; then
    sudo apt install -y devscripts equivs software-properties-common
fi

# Install plugin-specific dependencies
if [ -f "build-deps/control" ]; then
    echo "Installing plugin dependencies..."
    mk-build-deps --root-cmd=sudo -ir build-deps/control || true
fi

# Create build directory
BUILD_DIR="build"
echo "Setting up build directory: $BUILD_DIR"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with cmake for Ubuntu 22.04
echo "Configuring build with cmake for Ubuntu 22.04..."
cmake -DOCPN_TARGET_TUPLE='ubuntu-wx32-x86_64;22.04;x86_64' ..

# Build the tarball
echo "Building tarball..."
make tarball

# Find and display the generated tarball
TARBALL=$(find . -name "*.tar.gz" | head -n 1)
if [ -n "$TARBALL" ]; then
    echo "=== Build completed successfully! ==="
    echo "Generated tarball: $TARBALL"
    echo ""
    echo "To install the plugin:"
    echo "1. Open OpenCPN"
    echo "2. Go to Settings -> Plugins -> Import Plugin"
    echo "3. Select the tarball file: $(realpath $TARBALL)"
    echo ""
    echo "Or copy to local OpenCPN directory:"
    echo "cp libropeless_pi.so ~/.local/lib/opencpn/"
else
    echo "Warning: No tarball found in build directory"
    exit 1
fi