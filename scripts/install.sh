#!/bin/bash

# TankDuel Installation Script
# This script installs TankDuel system-wide

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}TankDuel Installation Script${NC}"
echo "============================="

# Check if running as root on Linux
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if [ "$EUID" -ne 0 ]; then
        echo -e "${RED}Please run as root (use sudo)${NC}"
        exit 1
    fi
fi

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

# Parse command line arguments
PREFIX="/usr/local"
BUILD_TYPE="Release"

while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix)
            PREFIX="$2"
            shift 2
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --prefix PREFIX    Installation prefix (default: /usr/local)"
            echo "  --debug           Install debug build"
            echo "  -h, --help        Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Build if necessary
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Building TankDuel...${NC}"
    "$SCRIPT_DIR/build.sh" $([ "$BUILD_TYPE" = "Debug" ] && echo "--debug")
fi

cd build

# Install
echo -e "${YELLOW}Installing TankDuel to $PREFIX...${NC}"
cmake --install . --prefix "$PREFIX"

# Update desktop database on Linux
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v update-desktop-database &> /dev/null; then
        echo -e "${YELLOW}Updating desktop database...${NC}"
        update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
    fi
fi

echo -e "${GREEN}Installation complete!${NC}"

# Print installation info
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "TankDuel.app installed to: $PREFIX/TankDuel.app"
    echo "You can run it from Applications or with: open $PREFIX/TankDuel.app"
else
    echo "TankDuel installed to: $PREFIX/bin/TankDuel"
    echo "You can run it with: TankDuel"
fi