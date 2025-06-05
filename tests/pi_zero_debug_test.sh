#!/bin/bash
# pi_zero_debug_test.sh - Pi Zero specific debugging and testing

echo "🍓 Pi Zero Debug Test Script"
echo "============================"

# Check if we're on a Pi Zero
if [ -f /proc/device-tree/model ]; then
    PI_MODEL=$(cat /proc/device-tree/model)
    echo "Detected: $PI_MODEL"
    
    if [[ $PI_MODEL == *"Pi Zero"* ]]; then
        echo "✅ Running on Pi Zero - applying safety measures"
        PI_ZERO=true
    else
        echo "ℹ️  Not a Pi Zero, but using conservative settings anyway"
        PI_ZERO=false
    fi
fi

# Check available memory
TOTAL_MEM=$(cat /proc/meminfo | grep MemTotal | awk '{print $2}')
TOTAL_MEM_MB=$((TOTAL_MEM / 1024))
FREE_MEM=$(cat /proc/meminfo | grep MemAvailable | awk '{print $2}')
FREE_MEM_MB=$((FREE_MEM / 1024))

echo "Memory: ${FREE_MEM_MB}MB free / ${TOTAL_MEM_MB}MB total"

if [ $FREE_MEM_MB -lt 100 ]; then
    echo "⚠️  WARNING: Very low memory (${FREE_MEM_MB}MB free)"
    echo "   Segfault risk is HIGH. Free up memory before testing."
fi

# Check if test images exist
cd "$(dirname "$0")"

if [ ! -d "images" ]; then
    echo "❌ Test images directory not found"
    echo "Creating small test images for Pi Zero..."
    mkdir -p images
    
    # Create tiny test images using ImageMagick if available
    if command -v convert >/dev/null 2>&1; then
        echo "📷 Creating 320x240 test images..."
        convert -size 320x240 xc:red images/test_320_1.jpg
        convert -size 320x240 xc:blue images/test_320_2.jpg
        echo "✅ Created small test images"
    else
        echo "❌ ImageMagick not found. Cannot create test images."
        echo "Please copy small (320x240 or smaller) JPEG images to tests/images/"
        exit 1
    fi
fi

# Find test images
TEST_IMG1=""
TEST_IMG2=""

for size in 320 640 800; do
    if [ -f "images/test_${size}_1.jpg" ] && [ -f "images/test_${size}_2.jpg" ]; then
        TEST_IMG1="images/test_${size}_1.jpg"
        TEST_IMG2="images/test_${size}_2.jpg"
        echo "📷 Using ${size}x* test images"
        break
    fi
done

if [ -z "$TEST_IMG1" ]; then
    # Look for any JPG files
    TEST_IMG1=$(find images -name "*.jpg" | head -n1)
    TEST_IMG2=$(find images -name "*.jpg" | tail -n1)
    
    if [ -z "$TEST_IMG1" ]; then
        echo "❌ No test images found in images/ directory"
        exit 1
    fi
fi

echo "Test images: $TEST_IMG1, $TEST_IMG2"

# Check image sizes
if command -v identify >/dev/null 2>&1; then
    IMG1_SIZE=$(identify "$TEST_IMG1" | awk '{print $3}')
    echo "Image 1 size: $IMG1_SIZE"
    
    # Parse width and height
    WIDTH=$(echo $IMG1_SIZE | cut -d'x' -f1)
    HEIGHT=$(echo $IMG1_SIZE | cut -d'x' -f2)
    
    if [ $WIDTH -gt 800 ] || [ $HEIGHT -gt 600 ]; then
        echo "⚠️  WARNING: Images are large (${IMG1_SIZE}). High segfault risk on Pi Zero."
        echo "   Recommended: Resize to 640x480 or smaller"
    elif [ $WIDTH -gt 1024 ] || [ $HEIGHT -gt 768 ]; then
        echo "❌ ERROR: Images are too large (${IMG1_SIZE}). Will likely segfault on Pi Zero."
        echo "   Must resize to 800x600 or smaller"
        exit 1
    fi
fi

echo ""
echo "🔨 Building Pi Zero debug version..."

# Build pi-debug version
cd ..
if make pi-debug; then
    echo "✅ Pi Zero debug version built successfully"
    ls -lh motion-detector-pi-debug
else
    echo "❌ Pi Zero debug build failed"
    echo "Trying simple version as fallback..."
    
    if make simple; then
        echo "✅ Simple version built as fallback"
        ln -sf motion-detector-simple motion-detector-pi-debug
    else
        echo "❌ All builds failed"
        exit 1
    fi
fi

cd tests

echo ""
echo "🧪 Testing Pi Zero debug version..."

# Test 1: Basic functionality with verbose output
echo "Test 1: Basic functionality (-v)"
echo "Command: ../motion-detector-pi-debug $TEST_IMG1 $TEST_IMG2 -v"
echo "---"

if timeout 30s ../motion-detector-pi-debug "$TEST_IMG1" "$TEST_IMG2" -v; then
    echo "✅ Test 1 PASSED"
else
    echo "❌ Test 1 FAILED (exit code: $?)"
    echo "This indicates a segfault or timeout"
fi

echo ""

# Test 2: Conservative settings for Pi Zero
echo "Test 2: Conservative settings (-s 4 -g)"
echo "Command: ../motion-detector-pi-debug $TEST_IMG1 $TEST_IMG2 -s 4 -g"
echo "---"

if timeout 30s ../motion-detector-pi-debug "$TEST_IMG1" "$TEST_IMG2" -s 4 -g; then
    echo "✅ Test 2 PASSED"
else
    echo "❌ Test 2 FAILED (exit code: $?)"
fi

echo ""

# Test 3: High scale factor (very safe)
echo "Test 3: High scale factor (-s 8 -g)"
echo "Command: ../motion-detector-pi-debug $TEST_IMG1 $TEST_IMG2 -s 8 -g"
echo "---"

if timeout 30s ../motion-detector-pi-debug "$TEST_IMG1" "$TEST_IMG2" -s 8 -g; then
    echo "✅ Test 3 PASSED"
else
    echo "❌ Test 3 FAILED (exit code: $?)"
fi

echo ""

# Test 4: Blur filter (potential crash source)
echo "Test 4: Blur filter test (-b -s 4 -g)"
echo "Command: ../motion-detector-pi-debug $TEST_IMG1 $TEST_IMG2 -b -s 4 -g"
echo "---"

if timeout 30s ../motion-detector-pi-debug "$TEST_IMG1" "$TEST_IMG2" -b -s 4 -g; then
    echo "✅ Test 4 PASSED - Blur filter is safe"
else
    echo "❌ Test 4 FAILED - Blur filter causes issues"
    echo "   Recommendation: Avoid -b flag on Pi Zero"
fi

echo ""
echo "📊 Test Summary"
echo "==============="

# Test simple version for comparison
echo "Testing simple version for comparison..."
if [ -f "../motion-detector-simple" ]; then
    if timeout 30s ../motion-detector-simple "$TEST_IMG1" "$TEST_IMG2" -s 4 -g >/dev/null 2>&1; then
        echo "✅ Simple version works correctly"
    else
        echo "❌ Even simple version fails - hardware/memory issue"
    fi
fi

echo ""
echo "🎯 Recommendations for Pi Zero:"
echo "================================"

if [ $FREE_MEM_MB -lt 200 ]; then
    echo "❗ CRITICAL: Free up memory (only ${FREE_MEM_MB}MB available)"
    echo "   - Close other applications"
    echo "   - Use smaller images"
fi

echo "✅ Safe command for Pi Zero:"
echo "   ../motion-detector-pi-debug img1.jpg img2.jpg -s 8 -g"
echo ""
echo "✅ If blur needed (test first):"
echo "   ../motion-detector-pi-debug img1.jpg img2.jpg -s 4 -g -b"
echo ""
echo "✅ For production (most reliable):"
echo "   ../motion-detector-simple img1.jpg img2.jpg -s 4 -g"

echo ""
echo "🔧 Troubleshooting:"
echo "==================="
echo "If segfaults persist:"
echo "  1. Use smaller images (320x240 recommended)"
echo "  2. Increase scale factor (-s 8 or higher)"
echo "  3. Avoid blur filter (-b)"
echo "  4. Free up system memory"
echo "  5. Use motion-detector-simple instead" 