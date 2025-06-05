# Pi Zero Segfault Fix

## Problem
The motion-detector-static binary causes segmentation fault on Raspberry Pi Zero when processing images.

## Root Cause
Pi Zero has limited memory (512MB) and ARMv6 architecture limitations that can cause crashes with:
- Large images (>800x600)
- Complex memory operations 
- Stack overflow from big image buffers

## Solution

### 1. Build Pi Zero Debug Version

```bash
# Build special Pi Zero debug version
make pi-debug

# This creates: motion-detector-pi-debug
# Features:
# - Conservative memory limits
# - DC-only mode disabled (stability)
# - Segfault debugging
# - Safety checks for large images
```

### 2. Test the Fix

```bash
# Run the debug test script
cd tests
./pi_zero_debug_test.sh

# This will:
# - Check your system memory
# - Test different parameter combinations
# - Recommend safe settings
```

### 3. Safe Usage on Pi Zero

**Recommended command:**
```bash
./motion-detector-pi-debug img1.jpg img2.jpg -s 8 -g
```

**Parameters explanation:**
- `-s 8` = Process every 8th pixel (much faster, less memory)
- `-g` = Grayscale mode (3x faster, less memory)
- **No `-b`** = Avoid blur filter (can cause crashes on large images)

### 4. Image Size Recommendations

| Resolution | Status | Notes |
|------------|--------|-------|
| 320x240 | ✅ Safe | Recommended for Pi Zero |
| 640x480 | ✅ Safe | Good balance |
| 800x600 | ⚠️ Caution | Use -s 4 or higher |
| 1920x1080 | ❌ Dangerous | Will likely segfault |

### 5. Fallback: Simple Version

If debug version still crashes:

```bash
# Build and use simple version
make simple
./motion-detector-simple img1.jpg img2.jpg -s 4 -g
```

### 6. Production Deployment

For reliable Pi Zero deployment:

```bash
# Copy to Pi Zero
scp motion-detector-simple pi@yourpi:~/
scp motion-detector-pi-debug pi@yourpi:~/

# Test on Pi Zero
ssh pi@yourpi
./motion-detector-simple snapshot1.jpg snapshot2.jpg -s 4 -g
```

### 7. Memory Optimization

Free up memory before running:
```bash
# Check memory
free -h

# Stop unnecessary services
sudo systemctl stop bluetooth
sudo systemctl stop avahi-daemon

# Kill heavy processes
sudo pkill -f chromium
```

### 8. Error Recovery

If you still get segfaults:

1. **Resize images smaller**:
   ```bash
   convert input.jpg -resize 640x480 output.jpg
   ```

2. **Use higher scale factors**:
   ```bash
   ./motion-detector-pi-debug img1.jpg img2.jpg -s 16 -g
   ```

3. **Check free memory**:
   ```bash
   free -m
   # You need at least 100MB free
   ```

## Technical Details

The Pi Zero debug version includes:
- `MOTION_PI_ZERO_DEBUG=1` - Debug mode
- `MOTION_DISABLE_DC_MODE=1` - Disable problematic DC-only
- `MOTION_CONSERVATIVE_MEMORY=1` - Conservative limits
- Segfault handler with detailed error messages
- Image size safety checks
- Memory-safe blur implementation

## Quick Commands Summary

```bash
# Build Pi Zero version
make pi-debug

# Safe test
./motion-detector-pi-debug img1.jpg img2.jpg -s 8 -g

# Full test suite
./tests/pi_zero_debug_test.sh

# Production use
./motion-detector-simple img1.jpg img2.jpg -s 4 -g
``` 