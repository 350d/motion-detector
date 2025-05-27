# Color codes for progress indication
RED=\033[0;31m
GREEN=\033[0;32m
YELLOW=\033[1;33m
BLUE=\033[0;34m
PURPLE=\033[0;35m
CYAN=\033[0;36m
NC=\033[0m # No Color

CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall -Wextra
TARGET = motion-detector
TARGET_SIMPLE = motion-detector-simple
SOURCE = motion_detector.cpp
SOURCE_SIMPLE = motion_detector_simple.cpp

# Architecture detection for optimization flags
# First check if we're cross-compiling by looking at CC/CXX variables
ifeq ($(origin CC), environment)
    COMPILER_NAME := $(notdir $(CC))
else ifeq ($(origin CXX), environment)
    COMPILER_NAME := $(notdir $(CXX))
else
    COMPILER_NAME := $(notdir $(CXX))
endif

# Detect target architecture from compiler name or system
ADVANCED_FLAGS = 

# Cross-compilation detection
ifneq ($(filter arm-linux-gnueabi%,$(COMPILER_NAME)),)
    # ARMv6 soft-float (Pi Zero)
    TARGET_ARCH = armv6-soft
    ADVANCED_FLAGS = -ftree-vectorize
else ifneq ($(filter arm-linux-gnueabihf%,$(COMPILER_NAME)),)
    # ARMv7 hard-float (Pi 3/4)
    TARGET_ARCH = armv7-hard
    ADVANCED_FLAGS = -ftree-vectorize -ffast-math
else ifneq ($(filter aarch64-linux-gnu%,$(COMPILER_NAME)),)
    # ARM64/AArch64
    TARGET_ARCH = aarch64
    ADVANCED_FLAGS = -ftree-vectorize -ffast-math
else
    # Native compilation - detect from system
    ARCH := $(shell uname -m)
    TARGET_ARCH = $(ARCH)
    
    # x86/x64 specific optimizations for native builds
    ifeq ($(ARCH),x86_64)
        ADVANCED_FLAGS = -march=native -msse2 -mavx2
    endif
    ifeq ($(ARCH),i386)
        ADVANCED_FLAGS = -march=native -msse2
    endif
    ifeq ($(ARCH),i686)
        ADVANCED_FLAGS = -march=native -msse2
    endif
    
    # ARM specific optimizations for native builds
    ifneq ($(filter arm% aarch64,$(ARCH)),)
        ADVANCED_FLAGS = -march=native -ftree-vectorize -ffast-math
        # Try to enable NEON if available, fallback gracefully
        ifeq ($(shell grep -q neon /proc/cpuinfo 2>/dev/null && echo yes),yes)
            ADVANCED_FLAGS += -mfpu=neon
        endif
    endif
    
    # macOS specific flags for better performance
    ifeq ($(shell uname), Darwin)
        ADVANCED_FLAGS += -framework Accelerate
    endif
endif

# Check if running in GitHub Actions
ifdef GITHUB_ACTIONS
    NOTICE_PREFIX = ::notice title=Build::
    ERROR_PREFIX = ::error title=Build Error::
    WARNING_PREFIX = ::warning title=Build Warning::
else
    NOTICE_PREFIX = ✅
    ERROR_PREFIX = ❌
    WARNING_PREFIX = ⚠️
endif

all: $(TARGET_SIMPLE)

# Simple version using standard stb_image (works everywhere)
simple: $(TARGET_SIMPLE)

$(TARGET_SIMPLE): $(SOURCE_SIMPLE) stb_image.h
	@echo "$(NOTICE_PREFIX)Building Motion Detector (Simple Version)"
	@echo "Checking dependencies..."
	@echo "  ✓ Source file: $(SOURCE_SIMPLE)"
	@echo "  ✓ Header file: stb_image.h"
	@echo "Configuring compiler..."
	@echo "  ✓ Compiler: $(CXX)"
	@echo "  ✓ Target: $(TARGET_ARCH)"
	@echo "  ✓ Flags: $(CXXFLAGS)"
	@echo "Compiling source code..."
	@$(CXX) $(CXXFLAGS) -o $(TARGET_SIMPLE) $(SOURCE_SIMPLE) || (echo "$(ERROR_PREFIX)Simple version compilation failed" && exit 1)
	@echo "Build complete!"
	@ls -lh $(TARGET_SIMPLE) | awk '{print "  ✓ Binary size: " $$5 " (" $$9 ")"}'
	@echo "$(NOTICE_PREFIX)Simple version built successfully!"
	@echo "Usage: ./$(TARGET_SIMPLE) image1.jpg image2.jpg -v"

# Advanced version with custom optimizations (may not compile on all systems)
advanced: $(TARGET)

$(TARGET): $(SOURCE) motion_stb_image.h stb_image.h
	@echo "$(NOTICE_PREFIX)Building Motion Detector (Advanced Version)"
	@echo "Checking dependencies..."
	@echo "  ✓ Source file: $(SOURCE)"
	@echo "  ✓ Custom header: motion_stb_image.h"
	@echo "  ✓ Standard header: stb_image.h"
	@echo "Detecting architecture..."
	@echo "  ✓ Target architecture: $(TARGET_ARCH)"
	@echo "  ✓ Compiler: $(COMPILER_NAME)"
	@echo "Configuring optimizations..."
	@echo "  ✓ Compiler: $(CXX)"
	@echo "  ✓ Base flags: $(CXXFLAGS)"
	@echo "  ✓ Advanced flags: $(ADVANCED_FLAGS)"
	@echo "Compiling with optimizations..."
	@$(CXX) $(CXXFLAGS) $(ADVANCED_FLAGS) -o $(TARGET) $(SOURCE) || (echo "$(ERROR_PREFIX)Advanced version compilation failed" && exit 1)
	@echo "Build complete!"
	@ls -lh $(TARGET) | awk '{print "  ✓ Binary size: " $$5 " (" $$9 ")"}'
	@echo "$(NOTICE_PREFIX)Advanced version built successfully!"
	@echo "Max performance: ./$(TARGET) img1.jpg img2.jpg -g -s 4 -d --benchmark"

# Debug build
debug: CXXFLAGS = -g -std=c++11 -Wall -Wextra -DDEBUG
debug: 
	@echo "$(NOTICE_PREFIX)Building debug version..."
	@$(MAKE) $(TARGET) CXXFLAGS="$(CXXFLAGS)"

# Benchmark optimized build
benchmark: CXXFLAGS = -O3 -std=c++11 -DNDEBUG -march=native -mtune=native -flto
benchmark: 
	@echo "$(NOTICE_PREFIX)Building maximum performance version..."
	@$(MAKE) $(TARGET) CXXFLAGS="$(CXXFLAGS)"

# Pi debug version (safe fallback)
pi-debug: motion_detector_pi_debug.cpp stb_image.h
	@echo "$(NOTICE_PREFIX)Building Pi Debug Version (Safe)..."
	@$(CXX) -g -std=c++11 -Wall -Wextra -DDEBUG -o motion-detector-pi-debug motion_detector_pi_debug.cpp
	@echo "$(NOTICE_PREFIX)Pi debug version built!"
	@echo "Usage: ./motion-detector-pi-debug img1.jpg img2.jpg -s 4 -m 2 -g -v"

clean:
	@echo "$(NOTICE_PREFIX)Cleaning build artifacts..."
	@rm -f $(TARGET) $(TARGET_SIMPLE) motion-detector-pi-debug
	@echo "$(NOTICE_PREFIX)Clean complete!"

install: $(TARGET_SIMPLE)
	@echo "$(NOTICE_PREFIX)Installing simple version to /usr/local/bin..."
	@cp $(TARGET_SIMPLE) /usr/local/bin/motion-detector
	@echo "$(NOTICE_PREFIX)Installation complete!"

install-advanced: $(TARGET)
	@echo "$(NOTICE_PREFIX)Installing advanced version to /usr/local/bin..."
	@cp $(TARGET) /usr/local/bin/motion-detector
	@echo "$(NOTICE_PREFIX)Installation complete!"

test: $(TARGET_SIMPLE)
	@echo "$(NOTICE_PREFIX)Running basic functionality test..."
	@echo "$(NOTICE_PREFIX)Simple version compiled successfully"
	@if [ -x "./test_motion.sh" ]; then \
		echo "Running automated tests..."; \
		./test_motion.sh; \
	else \
		echo "$(WARNING_PREFIX)No test script found. Run './test_motion.sh' manually."; \
	fi

.PHONY: all simple advanced clean install install-advanced debug benchmark test pi-debug 