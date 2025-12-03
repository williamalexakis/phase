#!/usr/bin/env bash
set -e

BUILD_DIR="build"
BUILD_TYPE="Release"  # Default

# Colours
GREEN="\033[1;32m"
YELLOW="\033[1;38;5;220m"
BLUE="\033[1;34m"
RESET="\033[0m"

echo -e "\n${GREEN}>>> Starting Phase Build <<<${RESET}\n"

# Parse flags
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            ;;
        --clean)
            echo -e "${YELLOW}Cleaning build directory...${RESET}"
            rm -rf "$BUILD_DIR"
            exit 0
            ;;
        --run)
            RUN_AFTER_BUILD=true
            ;;
        *)
            echo "[phase] Error: Unknown flag: $1"
            exit 1
            ;;
    esac
    shift
done

# Configure
echo -e "${YELLOW}Configuring (${BUILD_TYPE})...${RESET}"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. 

# Build
echo -e "${YELLOW}Building Phase...${RESET}"
cmake --build . -- -j$(nproc || echo 4)
echo -e "\n${GREEN}>>> Build Complete <<<${GREEN}\n"

# Optionally run
if [[ "$RUN_AFTER_BUILD" == true ]]; then
    echo -e "${YELLOW}Running Phase...${RESET}\n"
    ./phase --help || echo "[phase] Error: Executable not found."
fi
