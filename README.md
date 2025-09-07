# OpenGothic Starter

A cross-platform GUI launcher for OpenGothic, the open-source Gothic game
engine reimplementation.

## Features

- **Game Management**: Automatically discovers and lists Gothic mods from your
  Gothic installation
- **Separation of save game files**: Uses the operating systems default
  application path for save game files, separating them based on modifications.
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
. wxWidgets

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
cmake --build .
cmake -DCMAKE_BUILD_TYPE=Debug -B build
make -C build
```

## Setup

1. **First Launch**: Run the application and click "Settings"
2. **Configure Paths**:
   - **OpenGothic executable**: Point to your OpenGothic binary
   - **Gothic directory**: Point to your Gothic game installation
   - **Game version**: Select your Gothic version
3. **Install Mods**: Place mod `.ini` files in your `Gothic/system/` directory

## Usage

- **Select Mod**: Choose a mod from the list (or leave empty for vanilla)
- **Configure Options**: Use checkboxes and sliders to set launch parameters
- **Start Game**: Click "Start Game" or double-click a mod entry

## Mod Discovery

The launcher automatically scans your `Gothic/system/` directory for `.ini`
files with `[INFO]` and `[FILES]` sections, displaying them as available mods
with their titles and icons.
