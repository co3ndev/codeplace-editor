# Contributing to CodePlace Editor

Thanks for your interest in contributing! CodePlace is a community-driven project and contributions of all kinds are welcome — whether that's fixing a bug, improving documentation, or proposing a new feature. This guide covers everything you need to get started.

---

## Before You Start

For anything beyond a minor fix (typos, small bugs), please open an issue before writing code. This gives us a chance to discuss the approach and make sure the work aligns with the project's direction. Pull requests without a linked issue may be closed if the change isn't something we're looking to merge.

If an issue already exists for what you want to work on, leave a comment so others know it's being picked up.

---

## Setting Up Locally

CodePlace is built from source using CMake and Qt6. The full setup instructions are in the [README](README.md) under **Getting Started** — follow those to get a working build on your machine before making any changes.

---

## Making Changes

### Fork and branch

Fork the repository and create a branch off `main` for your work. Branch names should follow this convention:

- `feature/short-description` — for new functionality
- `fix/short-description` — for bug fixes
- `docs/short-description` — for documentation changes

Keep branch names lowercase and use hyphens instead of spaces.

### Writing code

There's no enforced formatter or strict style guide, but please try to match the style of the surrounding code. A few general principles:

- Keep changes focused. A PR that does one thing well is much easier to review than one that does five things.
- Don't leave commented-out code or debug statements behind.
- If you're touching something that isn't self-explanatory, a brief comment goes a long way.

### Commit messages

Write clear, descriptive commit messages. The subject line should summarize what the commit does, not what you changed. For example:

- `Add session restore on startup` — good
- `session_manager.cpp edits` — not helpful

---

## Submitting a Pull Request

1. Make sure your branch is up to date with `main` before opening the PR.
2. Open the pull request against `main` and fill out the description. Include a reference to the issue it addresses (e.g. `Closes #42`).
3. Describe what you changed and why. If there's anything non-obvious about your approach, explain it.
4. Commits will be squashed on merge, so don't worry about keeping a perfectly clean commit history during development.

We'll review your PR as soon as we can. We may leave feedback or ask questions — please don't take this as discouragement, it's just part of the process.

---

## Reporting Bugs

If you've found a bug but aren't up for fixing it yourself, opening an issue is still a valuable contribution. When reporting a bug, please include:

- Your operating system and version
- How to reproduce the problem (steps, sample files, etc.)
- What you expected to happen vs. what actually happened
- Any relevant output from the terminal or editor logs

The more detail you provide, the faster we can track it down.

---

## Feature Requests

Feature requests are welcome via GitHub Issues. Before opening one, have a quick look through existing issues to see if someone's already suggested it. If they have, add a comment or a reaction rather than opening a duplicate.

Keep in mind that CodePlace is intentionally lightweight. Feature requests that add significant complexity or drift from that philosophy may not be accepted, but we'll always try to explain why.

---

## License

By contributing to CodePlace, you agree that your contributions will be licensed under the same [GPLv3 license](LICENSE) that covers the project.