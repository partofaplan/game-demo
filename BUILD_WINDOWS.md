# Building Tank Duel on Windows

This guide explains how to build Tank Duel on Windows using either Visual Studio or MinGW.

## Prerequisites

### Option 1: Visual Studio (Recommended)
- Visual Studio 2019 or 2022 with C++ development tools
- CMake 3.16 or later
- SDL2 development libraries

### Option 2: MinGW
- MinGW-w64 compiler
- CMake 3.16 or later
- SDL2 development libraries

## Installing SDL2

### Method 1: Download Pre-built Libraries
1. Download SDL2 development libraries from https://libsdl.org/download-2.0.php
2. Extract to `C:\SDL2\` or another location
3. Set environment variable `SDL2DIR` to the SDL2 directory

### Method 2: Using vcpkg (Recommended for Visual Studio)
```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install sdl2:x64-windows
```

## Building

### Quick Build (Automated)
Use the provided build script:
```cmd
scripts\build.bat
```

Options:
- `-d` or `--debug` - Build in Debug mode
- `-c` or `--clean` - Clean build before building
- `-p` or `--package` - Create installer packages
- `--vs2019` - Force Visual Studio 2019
- `--vs2022` - Force Visual Studio 2022
- `--mingw` - Force MinGW

Examples:
```cmd
REM Release build with packaging
scripts\build.bat --package

REM Debug build with clean
scripts\build.bat --debug --clean

REM Force Visual Studio 2022
scripts\build.bat --vs2022
```

### Manual Build

#### Step 1: Configure
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
```

For MinGW:
```cmd
cmake .. -G "MinGW Makefiles"
```

#### Step 2: Build
```cmd
cmake --build . --config Release
```

#### Step 3: Package (Optional)
```cmd
cmake --build . --target package --config Release
```

## SDL2 Library Locations

The build system automatically searches for SDL2 in these locations:
- `%SDL2DIR%` environment variable
- `third_party/SDL2` (relative to project root)
- `C:\SDL2\`
- vcpkg installation directories
- Standard system paths

## Troubleshooting

### SDL2 Not Found
If CMake cannot find SDL2:
1. Set the `SDL2_ROOT_DIR` CMake variable:
   ```cmd
   cmake .. -DSDL2_ROOT_DIR="C:\path\to\SDL2"
   ```
2. Or set the `SDL2DIR` environment variable

### Missing DLLs at Runtime
The installer automatically includes SDL2 DLLs, but for development builds:
1. Copy `SDL2.dll` to the same directory as `TankDuel.exe`
2. Or add the SDL2 `bin` directory to your PATH

### Compiler Not Found
Ensure you have:
- Visual Studio with C++ tools installed, or
- MinGW-w64 in your PATH

## Output Files

After building:
- Executable: `build\Release\TankDuel.exe` (Visual Studio) or `build\TankDuel.exe` (MinGW)
- Installer packages (if `-p` used): `build\*.exe`, `build\*.msi`, `build\*.zip`

The executable includes the custom tank icon and Windows-specific metadata for a professional appearance.

## Running the Game

Simply double-click `TankDuel.exe` or run from command line:
```cmd
cd build\Release
TankDuel.exe
```

The game features:
- **1 Player**: Play against AI bot
- **2 Player**: Local multiplayer
- Turn-based artillery combat
- Physics-based projectile trajectories