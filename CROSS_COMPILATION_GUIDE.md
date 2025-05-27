# 🍓 Cross-Compilation Guide for Motion Detector

## 🎯 Overview

This project now includes automated GitHub Actions workflows for cross-compiling motion detector binaries for various ARM platforms, based on the proven approach from the v4l2rtspserver project.

## 🚀 Available Workflows

### 1. `pi-zero.yml` - Raspberry Pi Zero Specific
- **Target**: Raspberry Pi Zero/Zero W (ARMv6)
- **Optimization**: ARMv6 + VFP + soft-float ABI  
- **Features**: 
  - Builds both simple and advanced versions
  - Automated compatibility testing
  - Ready-to-deploy packages with installation scripts
  - Comprehensive error handling and fallbacks

### 2. `cross-compile-arm.yml` - Multi-Platform Matrix
- **Targets**: 
  - Pi Zero (ARMv6 soft-float)
  - Pi 3/4 (ARMv7 hard-float + NEON)
  - ARM64 (AArch64)
- **Features**:
  - Parallel builds for all platforms
  - Platform-specific optimizations
  - Individual packages for each target
  - Binary verification and stripping

### 3. `test-pi-zero.yml` - Quick Validation
- **Purpose**: Fast compatibility check for Pi Zero
- **Duration**: ~2 minutes
- **Use**: Pull request validation

## 📦 Build Artifacts

Each successful build generates downloadable packages:

```
motion-detector-pi-zero-build/
├── motion-detector-simple          # Pi Zero compatible binary
├── motion-detector-advanced        # Optimized version (if successful)
├── install.sh                      # Automated installer
├── README-pi-zero.md               # Platform-specific docs
├── build_for_pi.sh                 # Build script
└── Makefile                        # Build system
```

## 🔧 Manual Cross-Compilation Commands

### Pi Zero (ARMv6)
```bash
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
export CC=arm-linux-gnueabihf-gcc
export CXX=arm-linux-gnueabihf-g++
export CXXFLAGS="-march=armv6 -mfpu=vfp -mfloat-abi=softfp -O2 -std=c++11"
make simple CC="$CC" CXX="$CXX" CXXFLAGS="$CXXFLAGS"
```

### Pi 3/4 (ARMv7)
```bash
export CXXFLAGS="-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -O2 -std=c++11"
make advanced CC="$CC" CXX="$CXX" CXXFLAGS="$CXXFLAGS"
```

### ARM64 (AArch64)
```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
export CXXFLAGS="-march=armv8-a -O2 -std=c++11"
make advanced CC="$CC" CXX="$CXX" CXXFLAGS="$CXXFLAGS"
```

## 🚀 Deployment on Raspberry Pi

1. **Download from GitHub Actions**:
   - Go to repository → Actions tab
   - Select latest successful build
   - Download `motion-detector-pi-zero-build` artifact

2. **Extract and install**:
   ```bash
   unzip motion-detector-pi-zero-build.zip
   cd pi-zero-build/
   ./install.sh
   ```

3. **Test installation**:
   ```bash
   motion-detector img1.jpg img2.jpg -g -s 2 -v
   ```

## 📊 Performance Recommendations

| Platform | Best Flags | Notes |
|----------|------------|-------|
| **Pi Zero** | `-g -s 2 -b` | Use blur filter, scale factor 2 |
| **Pi 3/4** | `-g -s 2 --benchmark` | Can handle advanced optimizations |
| **Pi 4 64-bit** | `-d -g --benchmark` | DC-only mode works well |

## 🔍 Binary Verification

Each workflow verifies binary compatibility:
```bash
# Check architecture
file motion-detector-simple
# Example output: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV)

# Check specific ARM flags
readelf -h motion-detector-simple | grep -E "(Machine|Flags)"
# Example: Machine: ARM, Flags: 0x5000400, Version5 EABI, soft-float ABI
```

## 🎯 Key Features

✅ **100% Based on v4l2rtspserver proven approach**  
✅ **Automated Pi Zero ARMv6 soft-float compilation**  
✅ **Multi-platform ARM support (Pi Zero, Pi 3/4, ARM64)**  
✅ **Smart fallback strategies (advanced → simple)**  
✅ **Ready-to-deploy packages with install scripts**  
✅ **Comprehensive compatibility testing**  
✅ **GitHub Actions native notices and error handling**  
✅ **Binary verification and optimization**  

## 🛠️ Compatibility

The workflows use the exact same compiler flags and strategies that are proven to work in the v4l2rtspserver project, ensuring maximum compatibility with:

- Raspberry Pi Zero / Zero W
- Raspberry Pi 3 / 3B+ / 4 (32-bit)
- Raspberry Pi 4 (64-bit)
- Other ARMv6/ARMv7/AArch64 devices running Debian/Raspbian

## 📝 Notes

- Simple version always builds successfully
- Advanced version includes SIMD optimizations when possible
- Binaries are automatically stripped for minimal size
- Installation scripts include architecture verification
- All workflows include detailed logging and error reporting 