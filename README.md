# OpenGothic Starter

A cross-platform GUI launcher for OpenGothic, the open-source Gothic game
engine reimplementation.

## Features

- **Game Management**: Automatically discovers and lists Gothic mods from your
  Gothic installation
- **Separation of save game files**: Uses Gothic-local save directories:
  `Gothic/Saves` for no-mod runs and `Gothic/Saves/<MOD_NAME>` for mod runs.
- **Launch Options**: Configure various OpenGothic parameters including:
  - Window/Fullscreen mode
  - Ray tracing and global illumination
  - Marvin mode (developer console)
  - FXAA anti-aliasing levels
  - Virtual shadowmaps and meshlets
- **Multi-Version Support**: Compatible with Gothic 1, Gothic 2 Classic, and
  Gothic 2 Night of the Raven
- **Cross-Platform**: Works on Windows, Linux, and macOS

## Prerequisites

- [OpenGothic](https://github.com/Try/OpenGothic) engine
- Original Gothic game files
- CMake 3.16 or higher
- C++17 compatible compiler
- wxWidgets

### Debian based systems

```bash
sudo apt-get install -y libwxgtk3.2-dev
```

### MacOS

```bash
brew install wxwidgets
```

### Windows

```bash
vcpkg install
```

## Building
```bash
git clone <repository-url>
cd OpenGothicStarter
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

## Development

### Formatting

This repository uses `clang-format` with the checked-in `.clang-format`.

Format all C/C++ sources:

```bash
./scripts/format.sh
```

### Pre-commit Hook

Enable the repository hook that formats staged C/C++ files before commit:

```bash
git config core.hooksPath .githooks
```

## Setup

1. Copy `OpenGothicStarter` into your `Gothic/system/` directory.
2. Ensure the OpenGothic executable is present in `Gothic/system/`.
3. Place mod `.ini` files in `Gothic/system/`.

## Usage

- **Select Mod**: Choose a mod from the list (or leave empty for vanilla)
- **Configure Options**: Use checkboxes and sliders to set launch parameters
- **Start Game**: Click "Start Game" or double-click a mod entry

## Mod Discovery

The launcher automatically scans your `Gothic/system/` directory for `.ini`
files with `[INFO]` and `[FILES]` sections, displaying them as available mods
with their titles and icons.

## Runtime Layout

- Launcher location: `Gothic/system/OpenGothicStarter(.exe)`
- OpenGothic executable: `Gothic/system/Gothic2Notr(.exe)` (or `OpenGothic(.exe)`)
- No-mod working directory: `Gothic/Saves`
- Mod working directory: `Gothic/Saves/<MOD_NAME>`
