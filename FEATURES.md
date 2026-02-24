# Features

CodePlace isn't trying to be Everything for Everyone. It's a tool for people who want a fast, native Linux editor that handles the essentials without getting in the way. Here’s a breakdown of what’s currently in the box.

## The Editor Experience

At its core, CodePlace is built for speed. It uses Qt6 for a snappy, native feel that some web-based editors struggle to match.

- **Fast Rendering:** No laggy typing. The editor handles large files smoothly.
- **Syntax Highlighting:** Support for over 20 languages and formats. It uses a complex regex-based engine that covers the most common syntax patterns.
- **Code Folding:** Keep your workspace clean by collapsing blocks of code you aren't working on.
- **Bracket Matching:** Subtle highlights help you keep track of where that closing brace actually belongs.
- **Auto-Closing Pairs:** We handle the basics like closing quotes and brackets automatically, plus specialized tag closing for HTML and XML.
- **Session Persistence:** Close the editor and pick up exactly where you left off. It remembers your open tabs and window state.
- **Global Search & Replace:** Search across your entire project or just the current file. 

## Language Intelligence (LSP)

We don't try to guess your code's structure ourselves. Instead, we support the Language Server Protocol (LSP) to give you IDE-level features without the bloat.

- **Smart Completions:** Configure an LSP server for your language in Preferences, and CodePlace provides intelligent code completion with documentation tooltips.
- **Diagnostics:** Errors and warnings show up directly in the editor. Hover over a problem to see the details, or use the global "Problems" widget for a bird's-eye view.
- **Go to Definition:** Jump straight to the source of a symbol with a right-click or a shortcut.
- **Refactoring:** Basic LSP-driven renaming and document formatting are built in.
- **Outline Widget:** A dedicated view of your file's structure (classes, functions, etc.) for quick navigation.

## Integrated Tools

One of the goals of CodePlace is to keep you from constantly switching windows.

- **Embedded Terminal:** It’s a real terminal (bash). Run build scripts, grep some logs, or manage your system without leaving the editor.
- **Git Integration:** A dedicated sidebar shows your current status. You can switch branches, stage changes, write commit messages, and view diffs side-by-side. 

## Customization & Platform

CodePlace is built by Linux developers, for Linux developers.

- **Themeable UI:** Everything is styled via QSS (Qt Style Sheets). Swap between built-in themes or write your own.
- **Configurable Shortcuts:** Most core actions have adjustable keyboard shortcuts to match your muscle memory.
- **Lightweight Footprint:** We keep dependencies to a minimum. If you have Qt6 and a compiler, you're good to go.

## What's Next?

We're currently working on a **Plugin System** to allow for community-driven extensions. We're also looking at adding a very focused **Internal AI Chat** for documentation reference, but we aren't planning to turn CodePlace into an AI-first editor. We like writing code ourselves.
