#!/bin/bash
set -e

# CodePlace Tarball Installer
# This script installs CodePlace Editor to /usr/local

INSTALL_PREFIX="/usr/local"

echo "------------------------------------------"
echo "   CodePlace Editor - Tarball Installer   "
echo "------------------------------------------"

# 1. Check for Dependencies
echo "Checking for Qt6 runtime dependencies..."

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
# Qt6 runtime packages and system glue libraries
QT6_RUNTIME=(
    "libqt6widgets6" "libqt6gui6" "libqt6core6" "libqt6network6" "libqt6concurrent6"
    "qt6-wayland" "qt6-qpa-plugins" "libxkbcommon-x11-0" "libxkbcommon0" "libgl1"
    "libxcb-cursor0" "libxcb-icccm4" "libxcb-keysyms1" "libxcb-shape0"
    "libxcb-xinerama0" "libxcb-xinput0" "libxcb-randr0" "libxcb-image0"
    "libxcb-render-util0" "libxcb-util1" "libx11-xcb1" "libdbus-1-3"
)

for pkg in "${QT6_RUNTIME[@]}"; do
    check_and_install "$pkg" || MISSING_DEPS+=("$pkg")
done

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "The following Qt6 runtime dependencies are missing: ${MISSING_DEPS[*]}"
    read -p "Would you like to try installing them now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo apt-get update
        sudo apt-get install -y "${MISSING_DEPS[@]}"
    else
        echo "Warning: CodePlace Editor may not run without these libraries."
    fi
fi

# 2. Remove existing installation
echo "Checking for existing installation..."
sudo rm -f "$INSTALL_PREFIX/bin/codeplace"
sudo rm -f "$INSTALL_PREFIX/share/applications/codeplace.desktop"
sudo rm -f "$INSTALL_PREFIX/share/icons/hicolor/512x512/apps/codeplace.png"

# 2. Install files
echo "Installing to $INSTALL_PREFIX..."

# Binary
if [ -f "bin/codeplace" ]; then
    sudo cp bin/codeplace "$INSTALL_PREFIX/bin/"
    sudo chmod +x "$INSTALL_PREFIX/bin/codeplace"
else
    echo "Error: bin/codeplace not found. Are you running this from the extracted directory?"
    exit 1
fi

# Desktop Entry
if [ -f "share/applications/codeplace.desktop" ]; then
    sudo mkdir -p "$INSTALL_PREFIX/share/applications"
    sudo cp share/applications/codeplace.desktop "$INSTALL_PREFIX/share/applications/"
fi

# Icon
if [ -f "share/icons/hicolor/512x512/apps/codeplace.png" ]; then
    sudo mkdir -p "$INSTALL_PREFIX/share/icons/hicolor/512x512/apps"
    sudo cp share/icons/hicolor/512x512/apps/codeplace.png "$INSTALL_PREFIX/share/icons/hicolor/512x512/apps/"
fi

# Update desktop database
echo "Updating desktop database..."
sudo update-desktop-database 2>/dev/null || true

echo "------------------------------------------"
echo "Installation successful! You can now launch 'codeplace' from your terminal or application menu."
echo "------------------------------------------"
