# Installing CodePlace Editor (Tarball)

If you've downloaded the `.tar.gz` distribution, follow these steps to install CodePlace Editor on your Linux system.

### Prerequisites

Ensure you have the following packages installed (Ubuntu/Debian):
- `libqt6widgets6`
- `libqt6gui6`
- `libqt6core6`
- `libqt6network6`
- `libqt6concurrent6`
- `libqt6svg6`

### Installation Steps

1. **Extract the archive:**
   ```bash
   tar -xzf codeplace-editor-*.tar.gz
   cd codeplace-editor-*
   ```

2. **Run the install script:**
   The included `install.sh` script will remove any existing version and install the new binary, desktop entry, and icon to `/usr/local`.

   ```bash
   chmod +x install.sh
   ./install.sh
   ```

### Uninstallation

To remove the application manually:
```bash
sudo rm /usr/local/bin/codeplace
sudo rm /usr/local/share/applications/codeplace.desktop
sudo rm /usr/local/share/icons/hicolor/512x512/apps/codeplace.png
```
