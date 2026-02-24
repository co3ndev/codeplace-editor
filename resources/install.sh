#!/bin/bash
set -e

# CodePlace Tarball Installer
# This script installs CodePlace Editor to /usr/local

INSTALL_PREFIX="/usr/local"

echo "------------------------------------------"
echo "   CodePlace Editor - Tarball Installer   "
echo "------------------------------------------"

# 1. Remove existing installation
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
