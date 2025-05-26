#!/bin/bash
# test_motion.sh - Test script for motion detection

echo "=== Motion Detection Test Script ==="
echo

# Check if motion detector exists
if [ ! -f "../motion-detector-simple" ] && [ ! -f "../motion-detector" ]; then
    echo "❌ No motion detector found. Please build first:"
    echo "   make simple"
    echo "   # or"
    echo "   make advanced"
    exit 1
fi

# Determine which version to use
if [ -f "../motion-detector" ]; then
    DETECTOR="../motion-detector"
    echo "Using advanced version: $DETECTOR"
elif [ -f "../motion-detector-simple" ]; then
    DETECTOR="../motion-detector-simple"
    echo "Using simple version: $DETECTOR"
fi

echo

# Check if we can create test images with ImageMagick
if command -v convert >/dev/null 2>&1; then
    echo "Creating test images with ImageMagick..."
    
    # Create identical images (no motion)
    convert -size 100x100 xc:blue test1.png
    convert -size 100x100 xc:blue test2.png
    
    # Test with identical images
    echo "Test 1: Identical images (should show no motion)"
    $DETECTOR test1.png test2.png -v
    echo
    
    # Create different images (motion)
    convert -size 100x100 xc:blue test3.png
    convert -size 100x100 xc:red -fill white -draw "circle 30,30 40,40" test4.png
    
    echo "Test 2: Different images (should show motion)"
    $DETECTOR test3.png test4.png -v
    echo
    
    # Performance test
    echo "Test 3: Performance benchmark"
    $DETECTOR test3.png test4.png -g -s 2 --benchmark
    echo
    
    # Cleanup
    rm -f test1.png test2.png test3.png test4.png
    
elif command -v ffmpeg >/dev/null 2>&1; then
    echo "Creating test images with FFmpeg..."
    
    # Create test images using ffmpeg
    ffmpeg -f lavfi -i color=blue:size=100x100:duration=1 -vframes 1 -y test1.jpg 2>/dev/null
    ffmpeg -f lavfi -i color=blue:size=100x100:duration=1 -vframes 1 -y test2.jpg 2>/dev/null
    
    echo "Test 1: Identical images (should show no motion)"
    $DETECTOR test1.jpg test2.jpg -v
    echo
    
    ffmpeg -f lavfi -i color=red:size=100x100:duration=1 -vframes 1 -y test3.jpg 2>/dev/null
    
    echo "Test 2: Different images (should show motion)"
    $DETECTOR test1.jpg test3.jpg -v
    echo
    
    # Cleanup
    rm -f test1.jpg test2.jpg test3.jpg
    
else
    echo "⚠️  No ImageMagick or FFmpeg found. Manual testing required."
    echo
    echo "To test manually, use any two image files:"
    echo "  $DETECTOR image1.jpg image2.jpg -v"
    echo
    echo "Performance test with scaling:"
    echo "  $DETECTOR image1.jpg image2.jpg -g -s 2 --benchmark"
    echo
    echo "Fast motion detection:"
    echo "  $DETECTOR image1.jpg image2.jpg -g -s 4 -t 20 -m 1.5"
    echo
fi

echo "=== Available options ==="
$DETECTOR --help

echo
echo "=== Test Complete ===" 