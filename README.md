# CodePlace Editor

A lightweight, fast code editor built with C++ and Qt. Whether you're working on a quick script or managing a larger project, CodePlace provides a clean, distraction-free environment with powerful features to boost your productivity.

---

## Screenshots

![Screenshot 1](screenshots/screenshot1.png)
![Screenshot 2](screenshots/screenshot2.png)

---

## Features

| Feature | Description |
|---|---|
| **File Browser** | Browse your files and folders. |
| **Session Management** | Automatically save and restore your editing sessions. Pick up right where you left off. |
| **Theme Management** | Choose from multiple themes or create your own using QSS (Qt Style Sheets). |
| **Syntax Highlighting** | Support for 23 programming languages and file formats. |
| **Multi-Tab Editing** | Work with multiple files simultaneously using an intuitive tab interface. |
| **Integrated Terminal** | Run commands and scripts directly in the editor. Uses your user default shell (e.g., bash, zsh). |
| **Git Integration** | Built-in Source Control Management via Git. |
| **LSP Support** | Language Server Protocol support for intelligent code completion, errors, and warnings. Note: you must provide the path to the LSP server for each language. |

---

## Supported Languages

CodePlace supports syntax highlighting and LSP integration across a wide range of languages and formats:

| Category | Languages |
|---|---|
| **Core Programming & Scripting** | C, C++, C#, Dart, Go, Java, Kotlin, Lua, Perl, PHP, Python, Ruby, Rust, Shell, Swift |
| **Web & Frontend** | CSS, HTML, JavaScript, TypeScript |
| **Data, Markup & Query** | JSON, Markdown, SQL, XML, YAML |

---

## Getting Started

### Prerequisites
```
A note about building from source: CodePlace is developed and built on Linux, but it should be possible to build it on other platforms as well. If you encounter any issues, please open an issue on GitHub. Linux is our priority, but we aim to remain platform-agnostic.
```
Before building CodePlace, ensure you have the following installed:

- **CMake** (version 3.16 or higher)
- **Qt6** (Core, Gui, Widgets, Network modules)
- **C++ Compiler** (supporting C++17 standard)
- **Make** or another CMake-supported build system
On Ubuntu/Debian:
```bash
sudo apt-get install cmake qt6-base-dev qt6-tools-dev build-essential
```

On macOS (with Homebrew):
```bash
brew install cmake qt@6
```

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/yourusername/codeplace-editor.git
cd codeplace-editor
```

2. Create a build directory and compile:
```bash
mkdir -p build
cd build
cmake ..
make
```

The compiled binary will be at `./build/codeplace_editor`.

3. Launch the editor:
```bash
./build/codeplace_editor
```

On first launch, CodePlace will create a configuration directory in your home folder to store settings, themes, and session data.

---

## Installation

Pre-built files are available via [GitHub Packages](https://github.com/co3ndev/codeplace-editor/packages) for Linux (x86_64, arm64. These contain the Qt6 runtime, which is needed for the application to run. 

---

## Project Structure

Check out the [Module Reference](MODULE_REFERENCE.md) for more information regarding project structure.

## Configuration

CodePlace stores its configuration at `~/.config/codeplace/` on Linux/macOS. You can manually edit these files to customize editor behavior and keybindings, theme colors and fonts, default file associations, and session restoration preferences.

---

## Troubleshooting

**Editor won't start**
- Ensure Qt6 libraries are properly installed.
- Check that your system meets the minimum requirements.
- Try deleting `~/.config/codeplace/` to reset to defaults.

**Syntax highlighting not working**
- Verify the file extension is recognized.
- Check that the language appears in the supported list above.
- Try switching themes to see if it's theme-specific.

**Performance issues**
- Close unused tabs to reduce memory usage.
- Disable syntax highlighting for very large files.
- Check available system memory.
- If the problem persists, open an issue with as much detail as possible.

For more help, view our [Issues](https://github.com/co3ndev/codeplace-editor/issues) page.

---

## Contributing

We welcome contributions from the community — bug fixes, new features, documentation improvements, whatever you're up for. See the [Contribution Guide](CONTRIBUTING_GUIDE.md) to get started.

---

## Roadmap

A few things we're actively working on:

- **Plugin system** *(high priority)* — extend the editor with custom functionality.
- **Internal AI chat** — a focused tool for documentation and code referencing. Generative AI is not planned to be a core pillar of this editor.

Have a feature request? [Open an issue](https://github.com/co3ndev/codeplace-editor/issues). CodePlace is meant to be a tool for developers, by developers.

---

## License

CodePlace Editor is licensed under the **GNU General Public License v3.0 (GPLv3)**. See [LICENSE](LICENSE) for the full text. In short, you're free to use, modify, and distribute this software, provided you maintain the same license for any derivative works.

---

## Acknowledgments

CodePlace is built on the Qt6 framework and benefits from the broader open-source community. Special thanks to everyone who has helped improve the editor, whether through feedback, code contributions or just bouncing ideas.
