#!/bin/bash

echo "=== Segfault Fix Test ==="
echo "Testing motion detector with HD images and blur filter"
echo "This test checks if the buffer overflow issues are fixed."
echo ""

# Function to test with given images
test_images() {
    local img1="$1"
    local img2="$2"
    local desc="$3"
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Testing: $desc"
    echo "Image 1: $img1"
    echo "Image 2: $img2"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "$img1" ] || [ ! -f "$img2" ]; then
        echo "❌ Images not found, skipping test"
        return 1
    fi
    
    # Check image dimensions
    echo "Checking image properties..."
    file "$img1" 2>/dev/null
    file "$img2" 2>/dev/null
    echo ""
    
    # Test the exact command that caused segfault
    echo "Running: ../motion-detector $img1 $img2 -s 4 -m 4 -g -b -v"
    echo ""
    
    # Run motion detector
    ../motion-detector "$img1" "$img2" -s 4 -m 4 -g -b -v
    exit_code=$?
    
    echo ""
    echo "Exit code: $exit_code"
    
    case $exit_code in
        0)
            echo "✅ SUCCESS: No motion detected, no segfault"
            ;;
        1)
            echo "✅ SUCCESS: Motion detected, no segfault"
            ;;

        139)
            echo "❌ SEGFAULT: Still getting segmentation fault (exit code 139)"
            echo "   This indicates the buffer overflow fix didn't work"
            ;;
        *)
            echo "❓ UNEXPECTED: Exit code $exit_code"
            ;;
    esac
    
    echo ""
    return $exit_code
}

# Test with generated large images
echo "1. Testing with generated 1920x1080 images:"
test_images "images/real_large_1.jpg" "images/real_large_2.jpg" "Generated HD test images"

# Test with any user-provided images in current directory
echo ""
echo "2. Looking for user images to test..."

# Check for common snapshot filenames
snapshot_files=(
    "/mnt/ramdisk/www/snapshot.jpg"
    "/mnt/ramdisk/www/snapshot_prev.jpg" 
    "snapshot.jpg"
    "snapshot_prev.jpg"
    "image1.jpg"
    "image2.jpg"
    "test1.jpg"
    "test2.jpg"
)

found_user_images=false
for ((i=0; i<${#snapshot_files[@]}; i+=2)); do
    img1="${snapshot_files[i]}"
    img2="${snapshot_files[i+1]}"
    
    if [ -f "$img1" ] && [ -f "$img2" ]; then
        echo ""
        echo "Found user images: $img1 and $img2"
        test_images "$img1" "$img2" "User snapshot images"
        found_user_images=true
        break
    fi
done

if [ "$found_user_images" = false ]; then
    echo "No user images found. To test with your own images:"
    echo "1. Copy your images to this directory, or"
    echo "2. Run: ../motion-detector /path/to/img1.jpg /path/to/img2.jpg -s 4 -m 4 -g -b -v"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "=== Test Complete ==="
echo ""
echo "If you're still getting segfaults with your specific images:"
echo "1. Try without blur filter: ../motion-detector img1.jpg img2.jpg -s 4 -m 4 -g -v"
echo "2. Try lower scale factor: ../motion-detector img1.jpg img2.jpg -s 2 -m 4 -g -b -v"
echo "3. Try file size mode: ../motion-detector img1.jpg img2.jpg -f -v"
echo ""
echo "For debugging, run with maximum verbosity:"
echo "../motion-detector img1.jpg img2.jpg -s 4 -m 4 -g -b -v --benchmark"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" 