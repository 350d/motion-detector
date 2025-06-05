#!/bin/bash
# Pi Zero Debug Test Script
# Helps diagnose segfault issues step by step

echo "=== Pi Zero Motion Detector Debug Script ==="
echo "This script will help diagnose segfault issues"
echo ""

IMAGE1="${1:-/mnt/ramdisk/www/snapshot1.jpg}"
IMAGE2="${2:-/mnt/ramdisk/www/snapshot2.jpg}"
BINARY="${3:-./motion-detector-debug}"

echo "Testing with:"
echo "  Image 1: $IMAGE1" 
echo "  Image 2: $IMAGE2"
echo "  Binary: $BINARY"
echo ""

# Step 1: Check binary exists
echo "Step 1: Checking binary..."
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary $BINARY not found!"
    echo "Build with: make pi-debug"
    exit 1
fi
echo "✓ Binary exists"

# Step 2: Check file existence 
echo ""
echo "Step 2: Checking image files..."
if [ ! -f "$IMAGE1" ]; then
    echo "ERROR: Image 1 not found: $IMAGE1"
    exit 1
fi
if [ ! -f "$IMAGE2" ]; then
    echo "ERROR: Image 2 not found: $IMAGE2"
    exit 1
fi
echo "✓ Both image files exist"

# Step 3: Check file types and sizes
echo ""
echo "Step 3: File analysis..."
echo "Image 1:"
file "$IMAGE1"
ls -lh "$IMAGE1"
echo ""
echo "Image 2:"  
file "$IMAGE2"
ls -lh "$IMAGE2"

# Step 4: Test ultra-safe file size mode first
echo ""
echo "Step 4: Testing ultra-safe file size mode (-f)..."
echo "Command: $BINARY \"$IMAGE1\" \"$IMAGE2\" -f -v"

# Use timeout if available, otherwise run directly
if command -v timeout >/dev/null 2>&1; then
    timeout 10s "$BINARY" "$IMAGE1" "$IMAGE2" -f -v
else
    "$BINARY" "$IMAGE1" "$IMAGE2" -f -v
fi
result=$?
echo "File size mode result: $result"

if [ $result -eq 0 ]; then
    echo "✓ File size mode: No motion"
elif [ $result -eq 1 ]; then
    echo "✓ File size mode: Motion detected" 
elif [ $result -eq 124 ]; then
    echo "⚠ File size mode: Timeout (process hung)"
else
    echo "✗ File size mode: Error/crash"
fi

# Step 5: Test verbose mode (this is where segfault likely occurs)
echo ""
echo "Step 5: Testing full image processing with verbose debug..."
echo "Command: $BINARY \"$IMAGE1\" \"$IMAGE2\" -g -v"
echo "(This is likely where segfault occurs)"

# Use timeout if available, otherwise run directly
if command -v timeout >/dev/null 2>&1; then
    timeout 15s "$BINARY" "$IMAGE1" "$IMAGE2" -g -v
else
    "$BINARY" "$IMAGE1" "$IMAGE2" -g -v
fi
result=$?
echo "Full processing result: $result"

if [ $result -eq 0 ]; then
    echo "✓ Full processing: No motion"
elif [ $result -eq 1 ]; then
    echo "✓ Full processing: Motion detected"
elif [ $result -eq 124 ]; then
    echo "⚠ Full processing: Timeout"
elif [ $result -eq 139 ]; then
    echo "✗ Full processing: Segmentation fault"
elif [ $result -eq 3 ]; then
    echo "✗ Full processing: Caught segfault (custom handler)"
else
    echo "✗ Full processing: Error code $result"
fi

echo ""
echo "=== Recommendations ==="
if [ $result -eq 139 ] || [ $result -eq 3 ]; then
    echo "Segfault detected! Try these solutions:"
    echo ""
    echo "1. Use file size mode only (bypasses image loading):"
    echo "   $BINARY \"$IMAGE1\" \"$IMAGE2\" -f"
    echo ""
    echo "2. Reduce image sizes:"
    echo "   convert -resize 640x480 input.jpg output.jpg"
    echo ""
    echo "3. Check for corrupted files:"
    echo "   jpeginfo -c \"$IMAGE1\""
    echo "   jpeginfo -c \"$IMAGE2\""
    echo ""
    echo "4. Test with simple PNG files:"
    echo "   convert \"$IMAGE1\" test1.png"
    echo "   convert \"$IMAGE2\" test2.png" 
    echo "   $BINARY test1.png test2.png -v"
    echo ""
    echo "5. Check available memory:"
    echo "   free -h"
    echo "   cat /proc/meminfo | grep Available"
fi

echo ""
echo "Debug complete!" 