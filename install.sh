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

# 2. Check and Install Dependencies
echo "Checking dependencies..."

# Function to check if a package is installed (Debian/Ubuntu)
check_and_install() {
    local pkg=$1
    if ! dpkg -l "$pkg" &> /dev/null; then
        echo "Missing dependency: $pkg"
        return 1
    fi
    return 0
}

MISSING_DEPS=()
# Basic build tools
for pkg in cmake build-essential; do
    check_and_install "$pkg" || MISSING_DEPS+=("$pkg")
done

# Qt6 development packages and system glue libraries
QT6_DEPS=(
    "qt6-base-dev" "qt6-tools-dev" "qt6-svg-dev" "qt6-wayland" "qt6-qpa-plugins"
    "libxkbcommon-x11-0" "libxkbcommon-dev" "libgl1-mesa-dev"
    "libxcb-cursor0" "libxcb-icccm4" "libxcb-keysyms1" "libxcb-shape0"
    "libxcb-xinerama0" "libxcb-xinput0" "libxcb-randr0" "libxcb-image0"
    "libxcb-render-util0" "libxcb-util1" "libx11-xcb1" "libdbus-1-3"
)

for pkg in "${QT6_DEPS[@]}"; do
    check_and_install "$pkg" || MISSING_DEPS+=("$pkg")
done

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "The following dependencies are missing: ${MISSING_DEPS[*]}"
    read -p "Would you like to install them now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo apt-get update
        sudo apt-get install -y "${MISSING_DEPS[@]}"
    else
        echo "Error: Cannot proceed without dependencies. Please install them manually and try again."
        exit 1
    fi
fi

# 3. Build the project
echo "Building project..."
mkdir -p build
cd build

cmake ..
make -j$(nproc)

# 3. Install
echo "Installing..."
sudo make install

# Update desktop database and icon cache
echo "Updating desktop database and icon cache..."
sudo update-desktop-database 2>/dev/null || true
sudo gtk-update-icon-cache -f -t /usr/local/share/icons/hicolor 2>/dev/null || true

echo "------------------------------------------"
echo "Build and installation complete!"
echo "------------------------------------------"
