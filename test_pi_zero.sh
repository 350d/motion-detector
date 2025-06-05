#!/bin/bash
# test_pi_zero.sh - Test script for Pi Zero compatibility
# Usage: ./test_pi_zero.sh

echo "Motion Detector libjpeg-turbo version - Pi Zero Test"
echo "===================================================="

# Check if binary exists
if [ ! -f "./motion-detector" ]; then
    echo "Error: motion-detector not found!"
    echo "Build it with: make"
    exit 1
fi

# Create test images if they don't exist
if [ ! -f "test1.jpg" ] || [ ! -f "test2.jpg" ]; then
    echo "Creating test images..."
    if command -v convert >/dev/null; then
        convert -size 640x480 xc:red test1.jpg
        convert -size 640x480 xc:blue test2.jpg
        echo "Test images created: test1.jpg (red), test2.jpg (blue)"
    elif command -v magick >/dev/null; then
        magick -size 640x480 xc:red test1.jpg
        magick -size 640x480 xc:blue test2.jpg
        echo "Test images created: test1.jpg (red), test2.jpg (blue)"
    else
        echo "Error: ImageMagick not found. Please install or provide test1.jpg and test2.jpg"
        exit 1
    fi
fi

echo ""
echo "Test 1: Basic functionality (should detect 100% motion)"
echo "----------------------------------------------------"
./motion-detector -g -v -b test1.jpg test2.jpg
echo ""

echo "Test 2: Scale 1/2 (should be faster, less memory)"
echo "-------------------------------------------------"
./motion-detector -s 2 -g -v -b test1.jpg test2.jpg
echo ""

echo "Test 3: Scale 1/4 (should be very fast, minimal memory)"
echo "-------------------------------------------------------"
./motion-detector -s 4 -g -v -b test1.jpg test2.jpg
echo ""

echo "Test 4: File size check mode (ultra-fast)"
echo "-----------------------------------------"
./motion-detector -f -v -b test1.jpg test2.jpg
echo ""

echo "Test 5: Ultra-fast mode (fastest IDCT + upsampling)"
echo "---------------------------------------------------"
./motion-detector -u -rgb -v -b test1.jpg test2.jpg
echo ""

echo "Test 6: Ultra-fast + Scale 1/4 (extreme speed)"
echo "-----------------------------------------------"
./motion-detector -u -s 4 -rgb -v -b test1.jpg test2.jpg
echo ""

echo "Test 7: Identical images (should detect 0% motion)"
echo "---------------------------------------------------"
./motion-detector -v -b test1.jpg test1.jpg
echo ""

echo "Pi Zero libjpeg-turbo tests completed!"
echo "If all tests passed without segfault, this version should work on Pi Zero."
echo ""
echo "Usage tips for Pi Zero:"
echo "  - Use -s 2 for HD images (1280x720+)"
echo "  - Use -s 4 for FullHD images (1920x1080+)"
echo "  - Use -f for very fast file size pre-check"
echo "  - Memory usage with -s 4 is ~16x less than full size" 