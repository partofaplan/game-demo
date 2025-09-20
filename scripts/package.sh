#!/bin/bash

# TankDuel Packaging Script
# This script creates distribution packages for TankDuel

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}TankDuel Packaging Script${NC}"
echo "============================"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Parse command line arguments
PLATFORM="all"
OUTPUT_DIR=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --platform PLATFORM    Target platform (all, macos, linux, windows)"
            echo "  --output DIR           Output directory for packages"
            echo "  -h, --help             Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

cd "$PROJECT_ROOT"

# Set output directory
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="dist"
fi

mkdir -p "$OUTPUT_DIR"

# Build and package for current platform
echo -e "${YELLOW}Building and packaging for current platform...${NC}"
"$SCRIPT_DIR/build.sh" --clean --package

# Move packages to output directory
echo -e "${YELLOW}Moving packages to $OUTPUT_DIR...${NC}"
mv build/*.{dmg,pkg,deb,rpm,tar.gz,zip} "$OUTPUT_DIR/" 2>/dev/null || echo "Some package types may not be available on this platform"

# Create source package
echo -e "${YELLOW}Creating source package...${NC}"
cd build
cmake --build . --target package_source
mv *.{tar.gz,zip} "../$OUTPUT_DIR/" 2>/dev/null || echo "Source packages moved"

cd "$PROJECT_ROOT"

echo -e "${GREEN}Packaging complete!${NC}"
echo "Packages available in: $OUTPUT_DIR"
ls -la "$OUTPUT_DIR"