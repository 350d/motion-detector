# Motion Detector

High-performance motion detection utility for comparing JPEG images. Optimized for Pi Zero with ARM-safe image loading and decode-time scaling.

## Features

- **Pi Zero optimized** - ARM-safe libjpeg-turbo instead of problematic stb_image
- **Decode-time scaling** - images scaled during JPEG decode (much more efficient)
- **Automatic Pi Zero protection** - large images auto-scaled to prevent crashes
- **Memory efficient** - up to 16x memory reduction with scale factor 4
- **Ultra-fast processing** - optimized for video surveillance and real-time analysis
- **File size pre-check** - ultra-fast motion detection based on file size changes
- **Flexible thresholds** - pixel-level and percentage-based motion detection
- **Grayscale processing** - 3x faster analysis option
- **Fast blur filter** - noise reduction with optimized separable filtering

## Quick Start

```bash
# Build
make

# Basic usage
./motion-detector image1.jpg image2.jpg

# With options
./motion-detector frame1.jpg frame2.jpg -t 30 -s 2 -b -v
```

## Installation

### Requirements
- **libjpeg-turbo** development headers

```bash
# On Pi Zero/Debian/Ubuntu:
sudo apt update
sudo apt install libjpeg-turbo8-dev

# On macOS:
brew install jpeg-turbo

# Compile
make

# Install to system (optional)
make install

# Test Pi Zero compatibility
make test-pi
```

## Usage

```
./motion-detector <image1> <image2> [options]
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-t&nbsp;<threshold>` | **Pixel sensitivity**: How different pixels must be to count as "changed" (0-255). Lower = more sensitive | 25 |
| `-s <scale>` | **Decode scale factor**: 1=full, 2=half, 4=quarter, 8=eighth (JPEG scaled during decode!) | 1 |
| `-m <motion_pct>` | Motion percentage threshold | 1.0 |
| `-f [threshold]` | Fast file size comparison mode | 5% |
| `-rgb` | **RGB mode**: Use RGB instead of grayscale (slower but more accurate) | - |
| `-u` | **Ultra-fast mode**: fastest IDCT + upsampling (15-25% faster, lower quality) | - |
| `-b` | **Blur mode**: Apply fast blur for noise reduction (separable filter) | - |
| `-v` | **Verbose output**: Detailed statistics with timing breakdown | - |
| `-f` | **File size mode**: Ultra-fast pre-check based on file size changes | - |

### Threshold Explanation (`-t`)

The **pixel threshold** controls how sensitive motion detection is:

- **Value range**: 0-255 (brightness difference between pixels)
- **Lower values** (5-15): Very sensitive - detects small changes, lighting shifts, noise
- **Medium values** (20-35): Balanced - detects clear movement, ignores minor changes  
- **Higher values** (40-80): Less sensitive - only detects major movements

**Examples:**
```bash
# Very sensitive - detects even small lighting changes
./motion-detector img1.jpg img2.jpg -t 10

# Default sensitivity - good for most cases  
./motion-detector img1.jpg img2.jpg -t 25

# Less sensitive - only major movements
./motion-detector img1.jpg img2.jpg -t 50
```

**How it works:** Each pixel is compared between images. If the brightness difference exceeds the threshold, it's counted as "changed". More changed pixels = more motion detected.

### Blur Filter (`-b`)

The **blur filter** helps reduce false positives from image noise and compression artifacts:

- **Separable filtering**: Optimized horizontal + vertical passes (5x faster than standard blur)
- **Grayscale optimization**: In grayscale mode, converts to grayscale first then blurs only 1 channel
- **Noise reduction**: Smooths out JPEG compression artifacts and sensor noise
- **Real-time friendly**: Only 2x slowdown on Pi Zero (vs 8x with naive implementation)

**When to use blur:**
- **Security cameras**: Reduces false alarms from compression artifacts
- **Low light**: Minimizes noise-induced false positives  
- **High sensitivity**: When using low thresholds (-t 5-15)
- **Poor quality images**: JPEG artifacts and noise cleanup

**Examples:**
```bash
# Security camera with noise reduction
./motion-detector cam_prev.jpg cam_curr.jpg -t 15 -b

# High sensitivity with blur to prevent false positives
./motion-detector img1.jpg img2.jpg -t 10 -b -s 2

# Ultra-fast with blur on Pi Zero
./motion-detector frame1.jpg frame2.jpg -s 4 -u -b
```

### Examples

```bash
# Basic motion detection (auto-optimized for Pi Zero)
./motion-detector prev.jpg curr.jpg

# High sensitivity with RGB processing
./motion-detector frame1.jpg frame2.jpg -t 15 -rgb

# Fast processing with 1/4 scale (16x memory reduction)
./motion-detector large1.jpg large2.jpg -s 4

# Ultra-fast processing with 1/8 scale (64x memory reduction)
./motion-detector vid1.jpg vid2.jpg -s 8

# Ultra-fast decode with fastest IDCT + upsampling
./motion-detector img1.jpg img2.jpg -u

# Ultra-fast file size check
./motion-detector cam1.jpg cam2.jpg -f

# Extreme speed: ultra-fast + quarter scale
./motion-detector large1.jpg large2.jpg -u -s 4

# Detailed analysis with timing
./motion-detector img1.jpg img2.jpg -v -t 20

# With blur for noise reduction
./motion-detector noisy1.jpg noisy2.jpg -b -t 15

# Pi Zero optimized for FullHD images
./motion-detector hd1.jpg hd2.jpg -s 4 -v

# Use in scripts
if ./motion-detector img1.jpg img2.jpg -s 2; then
    echo "Motion detected!"
fi

# Capture percentage
result=$(./motion-detector img1.jpg img2.jpg -v | grep "Motion:" | cut -d' ' -f2)
```

## Output

- **Default mode**: Outputs `1` (motion detected) or `0` (no motion)
- **Verbose mode** (`-v`): Detailed statistics and percentages
- **Exit codes**: `0` = no motion, `1` = motion detected, `2` = error

## Performance Modes

### Decode-Time Scaling (`-s`)
Images are scaled **during JPEG decode** - much more efficient than pixel skipping:

- `-s 1`: Full resolution (default)
- `-s 2`: Half resolution (4x less memory, 2x faster)
- `-s 4`: Quarter resolution (16x less memory, 4x faster)
- `-s 8`: Eighth resolution (64x less memory, 8x faster)

### Pi Zero Auto-Protection
Large images are automatically scaled to prevent crashes:
- Images > 1280x720 → automatically scaled to 1/2 during decode
- Prevents segfaults while maintaining functionality

### Processing Options
- **Decode scaling** (`-s`): Real memory reduction during JPEG decode
- **Grayscale** (default): 3x faster than RGB, use `-rgb` to enable RGB
- **Ultra-fast mode** (`-u`): Fastest IDCT + upsampling (15-25% faster, lower quality)
- **Blur filter** (`-b`): Noise reduction with separable filtering (2x slowdown, better accuracy)
- **File size** (`-f`): ~1000x faster than pixel analysis
- **Verbose** (`-v`): Detailed timing breakdown and statistics

## Fast Mode (`-f`)

Ultra-fast motion detection based on file size changes:

```bash
# Check if file content changed (ignoring headers)
./motion-detector prev.jpg curr.jpg -f 5

# More sensitive file size detection
./motion-detector cam1.jpg cam2.jpg -f 2
```

Speed: ~1 microsecond vs ~1 millisecond for full pixel analysis.

## Performance Results

### libjpeg-turbo Test Results (640x480 images)

| Scale Factor | Memory Usage | Load Time | Motion Time | Total Time | Final Size |
|-------------|-------------|-----------|-------------|------------|------------|
| **1x (full)** | 900 KB | 1.54 ms | 0.42 ms | 1.96 ms | 640x480 |
| **1x + ultra** | 900 KB | 1.29 ms | 0.36 ms | 1.65 ms | 640x480 |
| **2x (half)** | 225 KB | 0.87 ms | 0.07 ms | 0.95 ms | 320x240 |
| **4x (quarter)** | 56 KB | 0.89 ms | 0.03 ms | 0.92 ms | 160x120 |
| **4x + ultra** | 56 KB | 0.68 ms | 0.02 ms | 0.70 ms | 160x120 |

### Benefits of libjpeg-turbo Optimizations:
- **Decode scaling**: 16x memory reduction with 4x scale, real memory savings
- **Ultra-fast mode**: 15-25% speed boost with fastest IDCT + upsampling
- **No segfaults**: ARM-safe libjpeg-turbo instead of problematic stb_image
- **Real scaling**: Images actually smaller in memory, not just pixel skipping
- **Pi Zero ready**: Automatic protection for large images

## Platform Support

### Pi Zero Optimized
This version is specifically optimized for Pi Zero and ARM systems:

- **ARM-safe**: Uses libjpeg-turbo instead of problematic stb_image
- **Memory efficient**: Automatic scaling for large images
- **No segfaults**: Tested and stable on Pi Zero hardware
- **Real-time capable**: Fast enough for video surveillance

### Supported Formats
- **JPEG**: Full support with decode-time scaling
- **Other formats**: Not supported (JPEG focus for Pi Zero optimization)

### Quick Start

```bash
git clone <repository>
cd motion-detector
make install-deps  # Auto-install dependencies
make               # Build
./motion-detector image1.jpg image2.jpg
```

## Installation

**Ubuntu/Debian/Pi Zero:**
```bash
git clone <repository>
cd motion-detector

# Auto-install dependencies
make install-deps

# Or manual install:
# sudo apt update
# sudo apt install libjpeg-turbo8-dev build-essential pkg-config

make
./test_pi_zero.sh  # Test compatibility
```

**CentOS/RHEL/Fedora:**
```bash
git clone <repository>
cd motion-detector

# Auto-install dependencies
make install-deps

# Or manual install:
# sudo yum install libjpeg-turbo-devel gcc-c++ make pkg-config
# sudo dnf install libjpeg-turbo-devel gcc-c++ make pkg-config  # Fedora

make
```

**macOS (Homebrew):**
```bash
brew install jpeg-turbo
git clone <repository>
cd motion-detector
make
```

**Static build (Linux/Pi Zero only):**
```bash
# For deployment without dependencies
make static

# Pi Zero optimized static build
make pi-zero
```

**Manual installation (if pkg-config fails):**
```bash
# Install libjpeg-turbo manually, then:
make JPEG_LIBS="-ljpeg" JPEG_CFLAGS="-I/usr/include"
```

### Troubleshooting

**Quick fix for all dependency issues:**
```bash
# Auto-install dependencies for your system
make install-deps

# Check if everything is installed correctly
make check-deps
```

**Error: `Package libjpeg was not found` or `jpeglib.h: No such file or directory`**
```bash
# Step 1: Check what's missing
make check-deps

# Step 2: Auto-install (recommended)
make install-deps

# Step 3: Manual install if needed
# Ubuntu/Debian:
sudo apt install libjpeg-turbo8-dev pkg-config

# CentOS/RHEL:
sudo yum install libjpeg-turbo-devel pkg-config

# Fedora:
sudo dnf install libjpeg-turbo-devel pkg-config

# macOS:
brew install jpeg-turbo
```

**Still not working? Manual path override:**
```bash
# Find libjpeg installation:
find /usr -name "jpeglib.h" 2>/dev/null
find /opt -name "jpeglib.h" 2>/dev/null

# Compile with manual path:
make JPEG_CFLAGS="-I/path/to/include" JPEG_LIBS="-ljpeg"
```

### Build Options

| Target | Description | Platform |
|--------|-------------|----------|
| `make` | Standard build with dynamic linking | All |
| `make static` | Static build (no dependencies) | Linux/Pi Zero only |
| `make pi-zero` | Pi Zero optimized static build | Pi Zero/ARM |
| `make debug` | Debug build with symbols | All |
| `make clean` | Clean build artifacts | All |
| `make install` | Install to system | Linux/macOS |
| `make test-pi` | Run Pi Zero compatibility tests | All |
| `make check-deps` | Check if dependencies are installed | All |
| `make install-deps` | Auto-install dependencies | Linux/macOS |

### Static Build for Deployment

Static builds include all dependencies and can run on systems without libjpeg-turbo installed:

```bash
# On build machine (with libjpeg-turbo-dev):
make static

# Copy to target Pi Zero:
scp motion-detector-static pi@raspberrypi:~/motion-detector

# Run on Pi Zero (no dependencies needed):
./motion-detector image1.jpg image2.jpg
```

**Pi Zero optimized build:**
```bash
# Cross-compile for Pi Zero (on x86_64):
make pi-zero

# Or compile directly on Pi Zero:
make static
```

## Integration Examples

### Bash Scripts
```bash
#!/bin/bash
# Motion detection pipeline
for i in {1..100}; do
    if ./motion-detector frame$((i-1)).jpg frame$i.jpg -g -s 4; then
        echo "Motion in frame $i"
        # Process motion event
    fi
done
```

### Python Integration
```python
import subprocess
import sys

def detect_motion(img1, img2, threshold=25):
    result = subprocess.run([
        './motion-detector', img1, img2, 
        '-t', str(threshold), '-g'
    ], capture_output=True)
    return result.returncode == 1

# Usage
if detect_motion('prev.jpg', 'curr.jpg'):
    print("Motion detected!")
```

### Cron Job (Pi Zero deployment)
```bash
# Use static build for reliable deployment
*/5 * * * * /home/pi/motion-detector-static /home/pi/cam/prev.jpg /home/pi/cam/curr.jpg -s 4 && echo "Motion detected at $(date)" >> /var/log/motion.log
```

### Systemd Service
```bash
# Add to crontab for periodic checking
* * * * * /path/to/motion-detector /tmp/prev.jpg /tmp/curr.jpg -f && /path/to/alert.sh
```

## Build Options

```bash
# Default universal build
make

# Static version (no dependencies) - Linux only
make static

# Pi Zero debug version (troubleshooting)
make pi-debug

# Pi Zero static debug version - Linux only  
make pi-debug-static

# Development build with debug symbols
make debug

# Clean build files
make clean
```

### Static Builds for Pi Zero

For Pi Zero deployment, use the **static builds** from GitHub Actions to avoid library version conflicts:

1. Download the static binary from GitHub Actions artifacts: `motion-detector-pi-zero-static`
2. Copy to Pi Zero: `scp motion-detector-pi-zero-static pi@doorcamera:~/motion-detector`  
3. Make executable: `chmod +x motion-detector`
4. Run without any library dependencies

**Note:** Static linking is only available on Linux. macOS builds are dynamic only.

## Supported Formats

- **JPEG** (.jpg, .jpeg) - optimized processing
- **PNG** (.png) - full support
- **BMP** (.bmp) - basic support
- **Other formats** - via stb_image library

## Performance Tips

1. **Use grayscale** (`-g`) for 3x speed boost
2. **Scale down** (`-s 2` or `-s 4`) for faster processing
3. **File size mode** (`-f`) for ultra-fast detection
4. **Appropriate thresholds** - lower = more sensitive
5. **Blur filtering** (`-b`) for noisy images

## Technical Details

- **Memory safe**: Automatic bounds checking and safe allocations
- **Error handling**: Graceful fallbacks for unsupported operations
- **Cross-platform**: Works on x86, ARM, and embedded systems
- **Optimized**: Smart caching and processing optimizations
- **Lightweight**: Single binary with minimal dependencies

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Contributing

1. Fork the repository
2. Create your feature branch
3. Make your changes
4. Test on multiple platforms
5. Submit a pull request

The project automatically builds and tests on multiple platforms via GitHub Actions. 