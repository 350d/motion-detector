# Motion Detector Test Suite

This directory contains comprehensive test scripts for the motion detection utility.

## 📁 Test Structure

```
tests/
├── images/                    # Test images directory
│   ├── test_motion_*.jpg     # Basic motion test images  
│   ├── real_large_*.jpg      # HD test images (1920x1080)
│   └── ...                   # Various test images
├── simple_test.sh            # Quick functionality test
├── test_large_images.sh      # HD image tests (segfault detection)
├── test_motion.sh           # Comprehensive motion detection tests
├── create_large_test_images.py # Generate large test images
├── benchmark_filesize.sh    # File size comparison performance
├── benchmark_dc.sh          # JPEG DC-only mode performance
├── create_test_images.sh    # Create basic test images
└── example_pipeline.sh      # 3-stage pipeline demonstration
```

## 🚀 Quick Start

```bash
# Run basic functionality test
./simple_test.sh

# Run comprehensive test suite
./test_motion.sh

# Test large images (HD resolution)
./test_large_images.sh
```

## 📋 Test Scripts Overview

### Core Functionality Tests

| Script | Purpose | Description |
|--------|---------|-------------|
| `simple_test.sh` | Quick test | Basic functionality check with existing images |
| `test_motion.sh` | Full suite | Comprehensive motion detection testing |
| `test_large_images.sh` | HD tests | Tests 1920x1080 images, checks for segfaults |

### Performance Benchmarks

| Script | Purpose | Description |
|--------|---------|-------------|
| `benchmark_filesize.sh` | File size speed | Ultra-fast file size comparison performance |
| `benchmark_dc.sh` | JPEG DC-only | JPEG DC coefficient extraction benchmarks |
| `example_pipeline.sh` | Pipeline demo | 3-stage smart pipeline demonstration |

### Image Creation

| Script | Purpose | Description |
|--------|---------|-------------|
| `create_test_images.sh` | Basic images | Create simple test images using ImageMagick |
| `create_large_test_images.py` | HD images | Generate 1920x1080 test images using PIL/sips |

## 🧪 Detailed Test Descriptions

### `simple_test.sh`
- **Purpose**: Quick functionality verification
- **Images used**: Existing `test_motion_*.jpg` files
- **Tests**: Basic motion detection with blur filter
- **Expected**: No segfaults, proper exit codes
- **Runtime**: ~1 second

### `test_large_images.sh`
- **Purpose**: Large image stability testing
- **Images used**: 1920x1080 generated images
- **Tests**: HD images with/without blur filter
- **Expected**: No segmentation faults (exit code 139)
- **Runtime**: ~10 seconds (includes timeout)

### `test_motion.sh`
- **Purpose**: Comprehensive motion detection testing
- **Images used**: Generated test images (100x100)
- **Tests**: Identical vs different images, performance benchmarks
- **Expected**: Proper motion detection, help output
- **Runtime**: ~5 seconds

### `benchmark_filesize.sh`
- **Purpose**: File size comparison performance analysis
- **Tests**: Ultra-fast pre-screening vs full analysis
- **Metrics**: Processing time, throughput, speedup ratios
- **Expected**: 100x+ speedup for file size mode
- **Runtime**: ~30 seconds

### `benchmark_dc.sh`
- **Purpose**: JPEG DC-only mode performance testing
- **Tests**: DC-only vs standard JPEG loading
- **Metrics**: Loading speed, motion detection accuracy
- **Expected**: 3x+ speedup for compatible JPEG files
- **Runtime**: ~20 seconds

## 🎯 Running Specific Tests

### Test Motion Detection Accuracy
```bash
# Create and test with identical images
./test_motion.sh

# Expected: Shows motion detection on different images
```

### Test Performance
```bash
# File size comparison speed
./benchmark_filesize.sh

# JPEG DC-only speed (if compatible JPEGs available)
./benchmark_dc.sh
```

### Test Large Image Stability
```bash
# Generate large test images first
python3 create_large_test_images.py

# Then test for segfaults
./test_large_images.sh
```

### Test 3-Stage Pipeline
```bash
# Interactive pipeline demonstration
./example_pipeline.sh
```

## 🔧 Test Configuration

### Image Requirements
- **Basic tests**: Any JPEG/PNG images work
- **Performance tests**: Prefer JPEG for DC-only testing
- **Large image tests**: Requires 1920x1080 images

### Dependencies
- **ImageMagick**: For `create_test_images.sh` (optional)
- **Python PIL**: For `create_large_test_images.py` (optional)
- **sips**: macOS built-in tool (fallback for image creation)

### Creating Custom Test Images
```bash
# Using ImageMagick
convert -size 640x480 xc:black -fill white -draw "rectangle 100,100 200,200" test1.jpg
convert -size 640x480 xc:black -fill white -draw "rectangle 150,150 250,250" test2.jpg

# Using Python PIL
python3 create_large_test_images.py

# Using sips (macOS)
sips -z 1080 1920 input.jpg --out large_output.jpg
```

## ✅ Expected Results

### Successful Test Run
```
✅ Success! Program completed normally
Exit code: 0 or 1 (motion detected/not detected)
No segmentation faults (exit code 139)
```

### Performance Expectations
- **File size comparison**: 50,000+ files/second
- **Standard motion detection**: 500+ files/second  
- **DC-only mode**: 1,000+ files/second (when compatible)

### Common Issues and Solutions

**"Could not copy test image"**
- Create images first: `./create_test_images.sh`
- Or use: `python3 create_large_test_images.py`

**"ImageMagick not available"**
- Install: `brew install imagemagick` (macOS)
- Or use Python PIL version

**"Timeout occurred"**
- Normal for very large images on slow systems
- Indicates no infinite loops or hangs

**Exit code 139 (segfault)**
- Should not occur in latest version
- Report as bug if persistent

## 📊 Test Results Interpretation

### Exit Codes
- **0**: No motion detected (success)
- **1**: Motion detected (success)  
- **2**: Error (file not found, format issue)
- **139**: Segmentation fault (failure)
- **124**: Timeout (may indicate performance issue)

### Performance Metrics
- **Load time**: Image loading duration
- **Motion calculation**: Algorithm processing time
- **Total time**: End-to-end processing
- **Processing speed**: Megapixels per second

## 🔍 Debugging Failed Tests

### Enable Verbose Output
```bash
# Run tests with verbose motion detector output
../motion-detector images/test1.jpg images/test2.jpg -v

# Check image properties
file images/test1.jpg
identify images/test1.jpg  # if ImageMagick available
```

### Check Image Compatibility
```bash
# Test JPEG DC-only compatibility
../motion-detector images/test1.jpg images/test2.jpg --dc-strict -v

# If DC-only fails, use standard mode
../motion-detector images/test1.jpg images/test2.jpg -g -v
```

### Memory and Timing Analysis
```bash
# Run with timing information
../motion-detector images/test1.jpg images/test2.jpg --benchmark -v

# Monitor memory usage (macOS)
time ../motion-detector images/test1.jpg images/test2.jpg -v
```

## 🎪 Advanced Testing

### Custom Test Scenarios
```bash
# Test different thresholds
../motion-detector img1.jpg img2.jpg -t 10 -v  # sensitive
../motion-detector img1.jpg img2.jpg -t 50 -v  # less sensitive

# Test scaling factors
../motion-detector img1.jpg img2.jpg -s 2 -v   # faster
../motion-detector img1.jpg img2.jpg -s 4 -v   # much faster

# Test blur filter
../motion-detector img1.jpg img2.jpg -b -v     # noise reduction
```

### Batch Testing
```bash
# Test all image pairs in directory
for img1 in images/*.jpg; do
    for img2 in images/*.jpg; do
        if [ "$img1" != "$img2" ]; then
            echo "Testing: $img1 vs $img2"
            ../motion-detector "$img1" "$img2" -v
        fi
    done
done
```

This test suite ensures the motion detector works correctly across various scenarios and provides performance benchmarks for optimization verification. 