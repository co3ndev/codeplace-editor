# Module Reference

Project structure and core component breakdown.

### `src/browser`
File navigation and project tree.
- `FileBrowserSidebar`: Sidebar widget for browsing and managing files.

### `src/core`
Shared services and foundational utilities.
- `SettingsManager`: Handles user configuration persistence.
- `ThemeManager`: Manages application styling and color schemes.
- `ProjectManager`: Tracks workspace state and project root.
- `SessionManager`: Manages open tabs and restores editor state.
- `ProcessHelper`: Utility for spawning and managing external processes.
- `SimpleHighlighter`: Regex-based syntax highlighting engine.
- `default_shortcuts.h`: Keyboard shortcut definitions.

### `src/editor`
Core editing functionality and tab management.
- `EditorView`: Main text rendering and input handling component.
- `TabContainer`: Manages document tabs and view switching.
- `FindReplaceBar`: In-document search and replace interface.
- `SymbolInfoWidget`: Tooltips for definitions and language information.
- `QuickStartWidget`: Default view when no files are open.

### `src/gitscm`
Integrated Git version control.
- `GitManager`: Singleton managing Git backend operations.
- `GitScmWidget`: Sidebar for viewing status, staging, and commits.
- `DiffView`: Displays file changes and diffs.

### `src/lspclient`
Language Server Protocol integration.
- `LspManager`: Central coordinator for server lifecycles and requests.
- `ProblemsWidget`: Global diagnostic list for open documents.
- `LspClientWidget`: LSP configuration and status UI.

### `src/main`
Application entry point and window orchestration.
- `MainWindow`: Primary window layout and component initialization.
- `PreferencesDialog`: Configuration interface for editor settings.

### `src/outline`
Document structure navigation.
- `OutlineWidget`: Tree view of classes, functions, and symbols.
- `SymbolParser`: Helper for extracting symbols from the active file.

### `src/search`
Workspace-wide search.
- `SearchEngine`: High-performance search across project files.
- `SearchWidget`: Interface for global search queries and results.

### `src/terminal`
Embedded shell.
- `TerminalWidget`: Integrated terminal for command-line access.