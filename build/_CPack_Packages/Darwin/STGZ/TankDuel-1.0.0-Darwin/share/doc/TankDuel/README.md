# Tank Duel

An exciting turn-based artillery combat game featuring tank battles, destructible terrain, and physics-based projectiles.

![Tank Duel](tankduel.png)

## Features

- **Single Player Mode**: Battle against AI with adjustable difficulty (Easy, Medium, Hard)
- **Multiplayer Mode**: Local 2-player battles with turn-based or free-for-all gameplay
- **Multiple Weapon Types**: Mortar, Cluster bombs, Napalm, and Dirtgun
- **Destructible Environment**: Terrain erodes from explosions, towers fall realistically
- **Force Field System**: Activate protective shields that bounce projectiles
- **Physics Simulation**: Realistic ballistic trajectories and gravity effects
- **Dynamic AI**: Smart bot opponents that adapt strategy based on difficulty

## Game Controls

### Help Screen
Press **ESC** during gameplay to pause, or select **HELP** from the main menu for detailed control instructions.

### Basic Controls

#### Single Player Mode
- **Q/A**: Aim turret up/down
- **W/S**: Adjust projectile power
- **SPACE**: Fire weapon
- **E**: Cycle through weapon types
- **R**: Activate force field (when available)

#### Two Player Mode
- **Player 1**: Q/A (aim), W/S (power), SPACE (fire), E (ammo), R (shield)
- **Player 2**: I/K (aim), O/L (power), J (fire), U (shield)

## Installation

### Windows

#### Option 1: Installer (Recommended)
1. Download the latest `TankDuel-Setup.exe` from the [Releases](../../releases) page
2. Run the installer and follow the setup wizard
3. Launch Tank Duel from the Start Menu or Desktop shortcut

#### Option 2: Portable ZIP
1. Download `TankDuel-Windows.zip` from the [Releases](../../releases) page
2. Extract the ZIP file to your desired location
3. Double-click `TankDuel.exe` to play

#### Building from Source
See [BUILD_WINDOWS.md](BUILD_WINDOWS.md) for detailed build instructions.

**Requirements:**
- Windows 10 or later (64-bit recommended)
- DirectX 9.0c or later
- 50 MB free disk space

### macOS

#### Option 1: DMG Package (Recommended)
1. Download `TankDuel.dmg` from the [Releases](../../releases) page
2. Open the DMG file
3. Drag Tank Duel to your Applications folder
4. Launch from Applications or Launchpad

#### Option 2: ZIP Archive
1. Download `TankDuel-macOS.zip` from the [Releases](../../releases) page
2. Extract the ZIP file
3. Move `TankDuel.app` to your Applications folder
4. Launch the application

#### Building from Source
```bash
# Install dependencies (using Homebrew)
brew install cmake sdl2

# Clone and build
git clone <repository-url>
cd tank-duel
mkdir build && cd build
cmake ..
make

# Run
open TankDuel.app
```

**Requirements:**
- macOS 10.15 (Catalina) or later
- 50 MB free disk space

### Linux

#### Building from Source
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install cmake libsdl2-dev build-essential

# Fedora/RHEL
sudo dnf install cmake SDL2-devel gcc-c++

# Arch Linux
sudo pacman -S cmake sdl2 base-devel

# Build
git clone <repository-url>
cd tank-duel
mkdir build && cd build
cmake ..
make

# Install (optional)
sudo make install

# Run
./TankDuel
```

## Gameplay

### Game Modes
- **1 Player**: Face off against AI bots with three difficulty levels
- **2 Player**: Local multiplayer with turn-based or simultaneous play modes

### Weapons
- **Mortar**: Standard explosive projectile
- **Cluster**: Splits into multiple smaller explosives
- **Napalm**: Creates burning patches that damage over time
- **Dirtgun**: Builds terrain instead of destroying it

### Strategy Tips
- Use terrain to your advantage - hide behind hills and towers
- Force fields recharge after several shots - use them wisely
- Napalm creates area denial zones
- Dirtgun can create defensive positions or escape routes
- Watch for falling towers when terrain erodes beneath them

## Development

### Building Installers

#### Windows
```cmd
# Install dependencies
scripts\build.bat --package

# Output files:
# - build\TankDuel-1.0.0-win64.exe (NSIS installer)
# - build\TankDuel-1.0.0-win64.zip (Portable)
```

#### macOS
```bash
# Build package
mkdir build && cd build
cmake ..
make
make package

# Output files:
# - TankDuel-1.0.0-Darwin.dmg
# - TankDuel-1.0.0-Darwin.zip
```

### Technical Details
- **Engine**: Custom C++ engine with SDL2
- **Graphics**: Software-rendered pixel art style
- **Physics**: Custom ballistics and collision system
- **AI**: Difficulty-scaled bot intelligence
- **Cross-platform**: Windows, macOS, and Linux support

## Troubleshooting

### Windows
- **"Missing DLL" error**: Download and install Visual C++ Redistributable
- **Game won't start**: Try running as administrator
- **Performance issues**: Update graphics drivers

### macOS
- **"App can't be opened"**: Right-click → Open, then click "Open" in dialog
- **Permission denied**: Check System Preferences → Security & Privacy
- **Crashes on startup**: Try running from Terminal to see error messages

### General
- **Controls not working**: Check Help screen for correct key mappings
- **Audio issues**: Ensure system audio is not muted
- **Display problems**: Try running in windowed mode

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Credits

- Game Design & Programming: Tank Duel Development Team
- Graphics: Custom pixel art sprites and icons
- Built with SDL2 cross-platform library

---

**Enjoy the battle!** 🎮💥🚀