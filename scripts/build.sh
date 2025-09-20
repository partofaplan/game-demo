#!/bin/bash

# TankDuel Build Script
# This script builds TankDuel for the current platform

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}TankDuel Build Script${NC}"
echo "=========================="

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Parse command line arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
PACKAGE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -p|--package)
            PACKAGE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug     Build in Debug mode (default: Release)"
            echo "  -c, --clean     Clean build directory before building"
            echo "  -p, --package   Create distribution packages"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

cd "$PROJECT_ROOT"

BUILD_DIR="build"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
echo -e "${YELLOW}Building TankDuel...${NC}"
cmake --build . --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Test if executable was created
if [[ "$OSTYPE" == "darwin"* ]]; then
    EXECUTABLE="TankDuel.app/Contents/MacOS/TankDuel"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    EXECUTABLE="TankDuel.exe"
else
    EXECUTABLE="TankDuel"
fi

if [ -f "$EXECUTABLE" ]; then
    echo -e "${GREEN}Build successful! Executable created: $EXECUTABLE${NC}"
else
    echo -e "${RED}Build failed! Executable not found.${NC}"
    exit 1
fi

# Package if requested
if [ "$PACKAGE" = true ]; then
    echo -e "${YELLOW}Creating distribution packages...${NC}"
    cmake --build . --target package
    echo -e "${GREEN}Packages created successfully!${NC}"
    echo "Available packages:"
    ls -la *.{dmg,pkg,deb,rpm,tar.gz,zip} 2>/dev/null || echo "No packages found"
fi

echo -e "${GREEN}Build complete!${NC}"