#!/bin/bash
# create_images/test_images.sh - Create simple test images for motion detection

echo "🖼️  Creating test images for motion detection..."

# Check if we have imagemagick
if command -v convert >/dev/null 2>&1; then
    echo "Using ImageMagick..."
    
    # Create identical blue images (no motion)
    convert -size 320x240 xc:blue images/test_no_motion_1.jpg
    convert -size 320x240 xc:blue images/test_no_motion_2.jpg
    
    # Create different images (motion)
    convert -size 320x240 xc:blue images/test_motion_1.jpg
    convert -size 320x240 xc:red -fill white -pointsize 50 -gravity center -annotate +0+0 "MOTION" images/test_motion_2.jpg
    
    echo "✅ Test images created:"
    echo "  - images/test_no_motion_1.jpg & images/test_no_motion_2.jpg (identical - should show 0% motion)"
    echo "  - images/test_motion_1.jpg & images/test_motion_2.jpg (different - should show high motion %)"

elif command -v ffmpeg >/dev/null 2>&1; then
    echo "Using FFmpeg..."
    
    # Create identical blue images
    ffmpeg -f lavfi -i color=blue:size=320x240:duration=1 -vframes 1 -y images/test_no_motion_1.jpg 2>/dev/null
    ffmpeg -f lavfi -i color=blue:size=320x240:duration=1 -vframes 1 -y images/test_no_motion_2.jpg 2>/dev/null
    
    # Create different images
    ffmpeg -f lavfi -i color=blue:size=320x240:duration=1 -vframes 1 -y images/test_motion_1.jpg 2>/dev/null
    ffmpeg -f lavfi -i color=red:size=320x240:duration=1 -vframes 1 -y images/test_motion_2.jpg 2>/dev/null
    
    echo "✅ Test images created:"
    echo "  - images/test_no_motion_1.jpg & images/test_no_motion_2.jpg (blue - should show low motion)"
    echo "  - images/test_motion_1.jpg & images/test_motion_2.jpg (blue vs red - should show high motion %)"

elif command -v raspistill >/dev/null 2>&1; then
    echo "Using Raspberry Pi camera..."
    echo "📸 Taking photos for motion detection test..."
    
    # Take first photo
    echo "Taking first photo in 3 seconds..."
    sleep 3
    raspistill -o images/test_photo_1.jpg -w 640 -h 480 -q 75 -t 1000
    
    echo "Now move something in front of the camera!"
    echo "Taking second photo in 5 seconds..."
    sleep 5
    raspistill -o images/test_photo_2.jpg -w 640 -h 480 -q 75 -t 1000
    
    echo "✅ Photos taken:"
    echo "  - images/test_photo_1.jpg & images/test_photo_2.jpg (should show motion if you moved something)"

else
    echo "❌ No image creation tools found."
    echo "Please install one of:"
    echo "  - ImageMagick: sudo apt install imagemagick"
    echo "  - FFmpeg: sudo apt install ffmpeg"
    echo "  - Or use Raspberry Pi camera module"
    exit 1
fi

echo
echo "🧪 To test motion detection:"
echo
if [ -f "images/test_no_motion_1.jpg" ]; then
    echo "No motion test:"
    echo "  ../motion-detector-simple images/test_no_motion_1.jpg images/test_no_motion_2.jpg -v"
    echo "  ../motion-detector images/test_no_motion_1.jpg images/test_no_motion_2.jpg -v"
    echo
    echo "Motion test:"
    echo "  ../motion-detector-simple images/test_motion_1.jpg images/test_motion_2.jpg -v"
    echo "  ../motion-detector images/test_motion_1.jpg images/test_motion_2.jpg -v"
elif [ -f "images/test_photo_1.jpg" ]; then
    echo "Camera motion test:"
    echo "  ../motion-detector-simple images/test_photo_1.jpg images/test_photo_2.jpg -v"
    echo "  ../motion-detector images/test_photo_1.jpg images/test_photo_2.jpg -v"
fi

echo
echo "🚀 Performance tests:"
echo "  ../motion-detector-simple images/img1.jpg images/img2.jpg -g -s 2 --benchmark"
echo "  ../motion-detector images/img1.jpg images/img2.jpg -g -s 4 --benchmark" 