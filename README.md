# Motion Detector

Universal motion detection utility for comparing two images. Optimized for video frame analysis with automatic platform detection and smart optimizations.

## Features

- **Universal compatibility** - works on all platforms (x86, ARM, Pi Zero, Pi 4)
- **Intelligent JPEG optimizations** - automatic scaling during decode for memory efficiency
- **DC-only mode** - ultra-fast JPEG preview mode (270x memory reduction)
- **Automatic mode selection** - detects image size and applies optimal processing
- **Smart image caching** - mode-aware caching with memory limits
- **Flexible thresholds** - pixel-level and percentage-based motion detection
- **Fast file size mode** - ultra-fast pre-screening based on file size changes
- **Grayscale processing** - 3x faster analysis option
- **Blur filtering** - reduces noise for better motion detection

## Quick Start

```bash
# Build
make

# Basic usage
./motion-detector image1.jpg image2.jpg

# With options
./motion-detector frame1.jpg frame2.jpg -t 30 -s 2 -g -v
```

## Installation

```bash
# Compile
make

# Install to system (optional)
make install
```

## Usage

```
./motion-detector <image1> <image2> [options]
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-t&nbsp;<threshold>` | **Pixel sensitivity**: How different pixels must be to count as "changed" (0-255). Lower = more sensitive | 25 |
| `-s <scale>` | Process every N-th pixel for speed | 1 |
| `-m <motion_pct>` | Motion percentage threshold | 1.0 |
| `-f [threshold]` | Fast file size comparison mode | 5% |
| `-d` | **JPEG DC-only mode** - ultra-fast preview (270x memory reduction) | - |
| `--dc-strict` | DC-only mode, error if not supported | - |
| `-g` | Force grayscale processing (3x faster) | - |
| `-b` | Enable 3x3 blur filter to reduce noise | - |
| `-v` | Verbose output with detailed statistics | - |
| `--benchmark` | Show timing information | - |

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

### Examples

```bash
# Basic motion detection (auto-optimized for JPEG)
./motion-detector prev.jpg curr.jpg

# High sensitivity with grayscale
./motion-detector frame1.jpg frame2.jpg -t 15 -g

# Ultra-fast JPEG DC-only mode (270x memory reduction)
./motion-detector large1.jpg large2.jpg -d

# Fast processing for video streams
./motion-detector vid1.jpg vid2.jpg -s 4 -g

# Ultra-fast file size check
./motion-detector cam1.jpg cam2.jpg -f

# Detailed analysis with blur filtering
./motion-detector img1.jpg img2.jpg -b -v -t 20

# Performance testing
./motion-detector img1.jpg img2.jpg -d --benchmark

# Use in scripts
if ./motion-detector img1.jpg img2.jpg -g -s 4; then
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

### JPEG Optimizations (Automatic)
- **Auto-detection**: Automatically detects JPEG files and applies optimizations
- **Memory prediction**: Pre-checks image size to prevent Pi Zero crashes
- **Intelligent scaling**: Automatically selects optimal scale based on image size:
  - Images > 2560px wide → 1/8 scale (64x faster, 1/64 memory)
  - Images > 1280px wide → 1/4 scale (16x faster, 1/16 memory)  
  - Images > 640px wide → 1/2 scale (4x faster, 1/4 memory)

### DC-Only Mode (`-d`)
Ultra-fast JPEG preview mode:
- **Memory**: 270x reduction (0.01 MB vs 2.7 MB for 1280x720)
- **Speed**: 68x faster motion detection
- **Quality**: Sufficient for motion detection
- **Example**: 1280x720 → 80x45 preview

### Scale Factor (`-s`)
- `-s 1`: Full resolution (default)
- `-s 2`: Half resolution (4x faster)
- `-s 4`: Quarter resolution (16x faster)
- `-s 8`: Eighth resolution (64x faster)

### Processing Options
- **DC-only** (`-d`): 270x memory reduction for JPEG
- **Grayscale** (`-g`): 3x faster than RGB
- **File size** (`-f`): ~1000x faster than pixel analysis
- **Blur filter** (`-b`): Better accuracy, slightly slower

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

### JPEG Optimization Test (1280x720 images)

| Mode | Memory Usage | Load Time | Motion Time | Total Time | Image Size |
|------|-------------|-----------|-------------|------------|------------|
| **Original** | 2.7 MB | - | - | - | 1280x720 |
| **Auto (1/2 scale)** | 0.66 MB | 9.2 ms | 0.48 ms | 9.7 ms | 640x360 |
| **DC-only** | 0.01 MB | 7.8 ms | 0.007 ms | 7.8 ms | 80x45 |

**DC-only benefits:**
- 270x memory reduction
- 68x faster motion detection  
- 20% faster overall processing
- Perfect for Pi Zero and video streams

## Platform Support

### Automatic Detection
The program automatically detects your platform and applies optimal settings:

- **x86/x64**: Full optimizations enabled
- **ARM/Pi**: Conservative settings with fallbacks
- **Pi Zero**: Memory-safe processing with automatic downsampling

### Pi Zero Support
Special Pi Zero debug version available for troubleshooting:

```bash
# Build debug version for Pi Zero
make pi-debug

# Use for segfault troubleshooting
./motion-detector-pi-debug img1.jpg img2.jpg -v
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

### Cron Job
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