# OpenGothic Starter

A cross-platform GUI launcher for OpenGothic, the open-source Gothic game
engine reimplementation.

## Quick Start

### Overview

- Discovers installed Gothic mods from your `system/` directory.
- Auto-detects Gothic version (Gothic 1, Gothic 2 Classic, Gothic 2 NotR).
- Launches OpenGothic with UI-configurable flags.
- Uses Gothic-local save directories:
  - no mod: `Gothic/Saves`
  - mod run: `Gothic/Saves/<MOD_NAME>`

### Requirements

- A Gothic installation (Steam or GOG) with a `Data/` and `system/` directory.
- OpenGothic binary (`Gothic2Notr(.exe)`).
- OpenGothicStarter release artifact for your platform:
  https://github.com/versable/OpenGothicStarter/releases
- Windows only: Microsoft Visual C++ Redistributable (x64):
  - Direct download: https://aka.ms/vc14/vc_redist.x64.exe
  - Official page:
    https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version

### Install Steps

1. Download the OpenGothicStarter release artifact for your platform from:
   https://github.com/versable/OpenGothicStarter/releases
2. Locate your Gothic installation folder (`Data/` and `system/` must exist).
3. Typical locations:
   - Steam (Windows): `C:\Program Files (x86)\Steam\steamapps\common\Gothic II`
   - Steam (Linux/Proton): `<SteamLibrary>/steamapps/common/Gothic II`
   - GOG (Windows): `C:\GOG Games\Gothic 2 Gold`
4. Download OpenGothic binaries:
   - Releases: https://github.com/Try/OpenGothic/releases
   - Latest AppVeyor builds: https://ci.appveyor.com/project/Try/opengothic/history?branch=master
5. Copy binaries into your Gothic `system/` directory:
   - `OpenGothicStarter(.exe)` from this project
   - `Gothic2Notr(.exe)` from OpenGothic
6. Start `OpenGothicStarter` from the same `system/` directory.

### macOS Data Extraction

If your Gothic data is from GOG installers, use the Offline Backup installer files
and extract with `innoextract` (or follow Mac Source Ports extraction guidance):
https://www.macsourceports.com/faq/

Place extracted game data where OpenGothic expects it, commonly under:
`~/Library/Application Support/OpenGothic`

### First Launch Behavior

- If Gothic version is detected, launcher stores it in:
  `system/OpenGothicStarter.ini`
- If detection fails, launcher asks you to choose Gothic version once and stores it
  in the same file.
- Language selection uses system locale by default.
- Optional install-level override in `system/OpenGothicStarter.ini`:

```ini
[GENERAL]
language=de
```

Use `language[_REGION]` codes (ISO 639 language + optional ISO 3166 region),
for example: `de`, `en`, `en_US`.

### Mods

- Install mods with their normal installer/process.
- The launcher auto-detects installed mod `.ini` files in `Gothic/system/`.
- Use "Start game without mods" for vanilla runs.

### Runtime Layout

- Launcher: `Gothic/system/OpenGothicStarter(.exe)`
- Engine binary: `Gothic/system/Gothic2Notr(.exe)`
- No-mod working directory: `Gothic/Saves`
- Mod working directory: `Gothic/Saves/<MOD_NAME>`

## Developer Setup

### Build Requirements

- [OpenGothic](https://github.com/Try/OpenGothic) engine
- Original Gothic game files
- CMake 3.16 or higher
- C++17 compatible compiler
- wxWidgets
- gettext tools (`msgfmt`, `xgettext`, `msgcat`) for localization checks

### Platform Dependencies

#### Debian-based systems

```bash
sudo apt-get install -y libwxgtk3.2-dev
```

#### macOS

```bash
brew install wxwidgets
```

#### Windows

```bash
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg.exe install --triplet x64-windows

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --parallel
```

### Build From Source

```bash
git clone <repository-url>
cd OpenGothicStarter
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

### Debug Build

#### Linux / macOS

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DOGS_WARNINGS_AS_ERRORS=ON
cmake --build build-debug --parallel
```

`Debug` builds on non-Windows automatically enable AddressSanitizer in this
project.

#### Windows (Visual Studio + vcpkg)

```bash
cmake -S . -B build-debug -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake \
  -DOGS_WARNINGS_AS_ERRORS=ON
cmake --build build-debug --config Debug --parallel
```

### Development Workflow

#### Formatting

This repository uses `clang-format` with the checked-in `.clang-format`.

```bash
./scripts/format.sh
```

#### Pre-commit Hook

Enable repository-local hooks:

```bash
git config --local core.hooksPath .githooks
```

#### Localization Checks

The i18n checks require gettext tools:
- `msgfmt` (validate `.po` catalogs)
- `xgettext` and `msgcat` (validate `.pot` sync against source strings)
