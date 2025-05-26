#!/bin/bash

echo "=== Large Image Motion Detection Test ==="

# Create two large test images if they don't exist
if [ ! -f "images/large_test1.jpg" ] || [ ! -f "images/large_test2.jpg" ]; then
    echo "Creating large test images (1920x1080)..."
    
    # Create large test images using ImageMagick (if available)
    if command -v convert >/dev/null 2>&1; then
        echo "Using ImageMagick to create test images..."
        # Create a 1920x1080 image with some pattern
        convert -size 1920x1080 xc:black -fill white -draw "rectangle 100,100 200,200" images/large_test1.jpg
        convert -size 1920x1080 xc:black -fill white -draw "rectangle 150,150 250,250" images/large_test2.jpg
        echo "✓ Large test images created"
    else
        echo "ImageMagick not available, using existing large images if any..."
        # Try to find any existing large images
        for img in images/*.jpg; do
            if [ -f "$img" ]; then
                echo "Using $img as test image"
                cp "$img" images/large_test1.jpg 2>/dev/null || true
                cp "$img" images/large_test2.jpg 2>/dev/null || true
                break
            fi
        done
    fi
fi

# Test with verbose mode and blur (conditions that caused segfault)
echo ""
echo "Testing motion detection with blur on large images..."

# First test without blur
echo "1. Testing without blur:"
timeout 10s ../motion-detector images/large_test1.jpg images/large_test2.jpg -s 4 -m 4 -g -v 2>&1
exit_code1=$?
echo "Exit code: $exit_code1"

echo ""
echo "2. Testing with blur (this previously caused segfault):"
timeout 10s ../motion-detector images/large_test1.jpg images/large_test2.jpg -s 4 -m 4 -g -b -v 2>&1
exit_code2=$?
echo "Exit code: $exit_code2"

echo ""
if [ $exit_code2 -eq 139 ]; then
    echo "❌ Segfault still occurs (exit code 139)"
elif [ $exit_code2 -eq 124 ]; then
    echo "⚠️ Timeout occurred (10 seconds)"
elif [ $exit_code2 -eq 0 ] || [ $exit_code2 -eq 1 ]; then
    echo "✅ Success! No segfault detected"
else
    echo "❓ Unexpected exit code: $exit_code2"
fi

echo ""
echo "=== Test completed ===" 