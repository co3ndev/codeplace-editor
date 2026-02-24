#!/bin/bash
set -e

# CodePlace Source Installer
# This script builds and installs CodePlace Editor from source.

echo "------------------------------------------"
echo "   CodePlace Editor - Source Installer    "
echo "------------------------------------------"

# 1. Clean up old installation
echo "Cleaning up old installation..."
sudo rm -f /usr/local/bin/codeplace
sudo rm -f /usr/local/share/applications/codeplace.desktop
sudo rm -f /usr/local/share/icons/hicolor/512x512/apps/codeplace.png

# 2. Build the project
echo "Building project..."
mkdir -p build
cd build

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed. Please install it first."
    exit 1
fi

cmake ..
make -j$(nproc)

# 3. Install
echo "Installing..."
sudo make install

# Update desktop database
echo "Updating desktop database..."
sudo update-desktop-database 2>/dev/null || true

echo "------------------------------------------"
echo "Build and installation complete!"
echo "------------------------------------------"
