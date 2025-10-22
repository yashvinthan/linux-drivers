#!/bin/bash
#
# Quick Build Script for Accusys ACS6x Driver
# Modernized for Kernel 6.1-6.12 (Debian 12 / TrueNAS SCALE / Proxmox)
#
# Usage:
#   ./quick-build.sh [kernel-version]
#
# Examples:
#   ./quick-build.sh                          # Build for current kernel
#   ./quick-build.sh 6.12.15-1-pve           # Build for specific kernel
#   ./quick-build.sh 6.1.0-28-pve            # Build for 6.1 LTS
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ACS6x Driver Quick Build Script${NC}"
echo -e "${BLUE}========================================${NC}"

# Determine target kernel
if [ -n "$1" ]; then
    TARGET_KERNEL="$1"
    echo -e "${GREEN}Building for specified kernel: $TARGET_KERNEL${NC}"
else
    TARGET_KERNEL="$(uname -r)"
    echo -e "${GREEN}Building for current kernel: $TARGET_KERNEL${NC}"
fi

KERNEL_DIR="/lib/modules/$TARGET_KERNEL/build"

# Check if kernel headers exist
if [ ! -d "$KERNEL_DIR" ]; then
    echo -e "${RED}ERROR: Kernel headers not found at $KERNEL_DIR${NC}"
    echo -e "${YELLOW}Install headers with:${NC}"
    echo -e "  ${YELLOW}sudo apt install pve-headers-$TARGET_KERNEL${NC}"
    echo -e "  ${YELLOW}OR${NC}"
    echo -e "  ${YELLOW}sudo apt install linux-headers-$TARGET_KERNEL${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Kernel headers found${NC}"

# Check if Makefile.new exists, use it
if [ -f "Makefile.new" ]; then
    echo -e "${YELLOW}Using modernized Makefile.new${NC}"
    cp Makefile.new Makefile
fi

# Check required files
echo -e "${BLUE}Checking required files...${NC}"
REQUIRED_FILES=("compat.h" "acs_ame.c" "acs_ame.h" "Makefile" "AME_module.c" "AME_Queue.c" "MPIO.c" "ACS_MSG.c" "AME_Raid.c")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}ERROR: Required file missing: $file${NC}"
        exit 1
    fi
done
echo -e "${GREEN}✓ All required files present${NC}"

# Clean previous build
echo -e "${BLUE}Cleaning previous build artifacts...${NC}"
make clean 2>/dev/null || true
echo -e "${GREEN}✓ Clean complete${NC}"

# Build
echo -e "${BLUE}Building ACS6x module...${NC}"
echo -e "${YELLOW}Command: make KERNELDIR=$KERNEL_DIR${NC}"

if make KERNELDIR="$KERNEL_DIR"; then
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ BUILD SUCCESSFUL${NC}"
    echo -e "${GREEN}========================================${NC}"
else
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}✗ BUILD FAILED${NC}"
    echo -e "${RED}========================================${NC}"
    echo -e "${YELLOW}Check error messages above.${NC}"
    echo -e "${YELLOW}Common issues:${NC}"
    echo -e "  - Kernel headers not fully installed"
    echo -e "  - Compiler version mismatch"
    echo -e "  - Missing dependencies (build-essential)"
    exit 1
fi

# Verify module
if [ ! -f "ACS6x.ko" ]; then
    echo -e "${RED}ERROR: Module file ACS6x.ko not created${NC}"
    exit 1
fi

MODULE_SIZE=$(stat -c%s ACS6x.ko)
echo -e "${GREEN}Module size: $(numfmt --to=iec-i --suffix=B $MODULE_SIZE)${NC}"

# Show module info
echo -e "${BLUE}Module information:${NC}"
modinfo ./ACS6x.ko | head -10

# Next steps
echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Next Steps:${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Module built successfully: ${GREEN}ACS6x.ko${NC}"
echo ""
echo -e "For testing on this machine:"
echo -e "  ${YELLOW}sudo insmod ./ACS6x.ko${NC}"
echo -e "  ${YELLOW}dmesg | tail -30${NC}"
echo -e "  ${YELLOW}lsmod | grep ACS6x${NC}"
echo -e "  ${YELLOW}sudo rmmod ACS6x${NC}"
echo ""
echo -e "For deployment to TrueNAS SCALE:"
echo -e "  1. Copy module to TrueNAS:"
echo -e "     ${YELLOW}scp ACS6x.ko root@<truenas-ip>:/mnt/tank/system/drivers/${NC}"
echo -e "  2. Follow instructions in:"
echo -e "     ${YELLOW}TRUENAS-DEPLOYMENT.md${NC}"
echo ""
echo -e "For DKMS installation:"
echo -e "  ${YELLOW}sudo ./dkms-install.sh${NC}"
echo ""
echo -e "${GREEN}Build completed at: $(date)${NC}"
