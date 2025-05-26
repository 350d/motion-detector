#!/bin/bash
# build_for_pi.sh - Smart build script for Raspberry Pi
# Automatically detects capabilities and builds the best version possible

echo "🍓 Raspberry Pi Smart Build Script"
echo "=================================="

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect architecture
ARCH=$(uname -m)
echo -e "${BLUE}Detected architecture:${NC} $ARCH"

# Detect Raspberry Pi model
if [ -f /proc/device-tree/model ]; then
    PI_MODEL=$(cat /proc/device-tree/model)
    echo -e "${BLUE}Raspberry Pi model:${NC} $PI_MODEL"
fi

# Check available memory
TOTAL_MEM=$(cat /proc/meminfo | grep MemTotal | awk '{print $2}')
TOTAL_MEM_MB=$((TOTAL_MEM / 1024))
echo -e "${BLUE}Available memory:${NC} ${TOTAL_MEM_MB}MB"

# Check for NEON support
if grep -q neon /proc/cpuinfo; then
    echo -e "${GREEN}✓ NEON SIMD support detected${NC}"
    NEON_SUPPORT=true
else
    echo -e "${YELLOW}⚠ No NEON support detected${NC}"
    NEON_SUPPORT=false
fi

# Check compiler version
GCC_VERSION=$(gcc --version | head -n1)
echo -e "${BLUE}Compiler:${NC} $GCC_VERSION"

echo ""
echo "🔨 Starting build process..."

# Build strategy for Raspberry Pi
echo "Strategy 1: Trying advanced version..."
if make advanced 2>/dev/null; then
    echo -e "${GREEN}✅ Advanced version built successfully!${NC}"
    
    # Test the advanced version with a simple case
    echo "Testing advanced version..."
    if [ -f "tests/images/test_640_1.jpg" ] && [ -f "tests/images/test_640_2.jpg" ]; then
        if timeout 10s ./motion-detector tests/images/test_640_1.jpg tests/images/test_640_2.jpg -v 2>/dev/null; then
            echo -e "${GREEN}✅ Advanced version test passed${NC}"
            
            # Test DC-only mode (potential segfault source)
            echo "Testing DC-only mode..."
            if timeout 10s ./motion-detector tests/images/test_640_1.jpg tests/images/test_640_2.jpg -d -v 2>/dev/null; then
                echo -e "${GREEN}✅ DC-only mode works${NC}"
                echo -e "${GREEN}🎯 Recommended usage: ./motion-detector img1.jpg img2.jpg -d -g -s 2 --benchmark${NC}"
            else
                echo -e "${RED}❌ DC-only mode fails (segfault risk)${NC}"
                echo -e "${YELLOW}⚠ Recommend avoiding -d flag on this system${NC}"
                echo -e "${GREEN}🎯 Safe usage: ./motion-detector img1.jpg img2.jpg -g -s 2 --benchmark${NC}"
            fi
        else
            echo -e "${RED}❌ Advanced version test failed${NC}"
            echo "Falling back to safer version..."
            make clean >/dev/null 2>&1
        fi
    else
        echo -e "${YELLOW}⚠ No test images found, cannot test advanced version${NC}"
        echo -e "${GREEN}✅ Advanced version built - use with caution${NC}"
    fi
else
    echo -e "${RED}❌ Advanced version failed to build${NC}"
    echo "Strategy 2: Trying simple version..."
    
    if make simple 2>/dev/null; then
        echo -e "${GREEN}✅ Simple version built successfully!${NC}"
        echo -e "${GREEN}🎯 Usage: ./motion-detector-simple img1.jpg img2.jpg -g -s 2 -v${NC}"
    else
        echo -e "${RED}❌ Simple version failed to build${NC}"
        echo "Strategy 3: Building Pi debug version (most compatible)..."
        
        if make pi-debug 2>/dev/null; then
            echo -e "${GREEN}✅ Pi debug version built successfully!${NC}"
            echo -e "${GREEN}🎯 Usage: ./motion-detector-pi-debug img1.jpg img2.jpg -g -s 4 -v${NC}"
            echo -e "${YELLOW}Note: DC-only mode disabled for safety${NC}"
        else
            echo -e "${RED}❌ All build strategies failed${NC}"
            echo "Please check compiler installation and dependencies"
            exit 1
        fi
    fi
fi

echo ""
echo "📋 Build Summary"
echo "==============="

if [ -f "motion-detector" ]; then
    BINARY_SIZE=$(ls -lh motion-detector | awk '{print $5}')
    echo -e "${GREEN}✅ Advanced version: motion-detector (${BINARY_SIZE})${NC}"
    
    # Check if DC-only mode is safe
    if [ -f "tests/images/test_640_1.jpg" ]; then
        if timeout 5s ./motion-detector tests/images/test_640_1.jpg tests/images/test_640_1.jpg -d 2>/dev/null; then
            echo -e "   ${GREEN}DC-only mode: Safe${NC}"
        else
            echo -e "   ${RED}DC-only mode: UNSAFE (segfault risk)${NC}"
        fi
    fi
fi

if [ -f "motion-detector-simple" ]; then
    BINARY_SIZE=$(ls -lh motion-detector-simple | awk '{print $5}')
    echo -e "${GREEN}✅ Simple version: motion-detector-simple (${BINARY_SIZE})${NC}"
fi

if [ -f "motion-detector-pi-debug" ]; then
    BINARY_SIZE=$(ls -lh motion-detector-pi-debug | awk '{print $5}')
    echo -e "${GREEN}✅ Pi debug version: motion-detector-pi-debug (${BINARY_SIZE})${NC}"
    echo -e "   ${YELLOW}Safest option for testing${NC}"
fi

echo ""
echo "🚀 Recommended Usage for Raspberry Pi:"

if [ -f "motion-detector" ]; then
    # Test if DC-only is safe
    if [ -f "tests/images/test_640_1.jpg" ]; then
        if timeout 5s ./motion-detector tests/images/test_640_1.jpg tests/images/test_640_1.jpg -d 2>/dev/null; then
            echo -e "${GREEN}# Fast mode (with DC-only):${NC}"
            echo "./motion-detector img1.jpg img2.jpg -d -g -s 2 --benchmark"
        else
            echo -e "${YELLOW}# Fast mode (DC-only disabled for safety):${NC}"
            echo "./motion-detector img1.jpg img2.jpg -g -s 2 --benchmark"
        fi
    else
        echo -e "${YELLOW}# Fast mode (DC-only untested):${NC}"
        echo "./motion-detector img1.jpg img2.jpg -g -s 3 --benchmark"
    fi
elif [ -f "motion-detector-simple" ]; then
    echo -e "${GREEN}# Reliable mode:${NC}"
    echo "./motion-detector-simple img1.jpg img2.jpg -g -s 2 -v"
elif [ -f "motion-detector-pi-debug" ]; then
    echo -e "${GREEN}# Debug mode:${NC}"
    echo "./motion-detector-pi-debug img1.jpg img2.jpg -g -s 4 -v"
fi

echo ""
echo -e "${GREEN}✅ Build complete!${NC}" 