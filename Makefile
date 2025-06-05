# Motion Detector - libjpeg-turbo Version
# Optimized for Pi Zero with ARM-safe image loading and decode-time scaling

CXX = c++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
LIBS = -lm

# Source files
MAIN_SRC = motion_detector.cpp

# Try pkg-config first, fallback to manual flags
PKG_CONFIG_EXISTS = $(shell pkg-config --exists libjpeg && echo yes)
JPEGLIB_H_EXISTS = $(shell [ -f /usr/include/jpeglib.h ] || [ -f /usr/local/include/jpeglib.h ] || [ -f /opt/homebrew/include/jpeglib.h ] && echo yes)

ifeq ($(PKG_CONFIG_EXISTS),yes)
	JPEG_CFLAGS = `pkg-config --cflags libjpeg`
	JPEG_LIBS = `pkg-config --libs libjpeg`
else
	# Fallback paths for common installations
	JPEG_CFLAGS = -I/usr/include -I/usr/local/include -I/opt/homebrew/include
	JPEG_LIBS = -ljpeg
endif

# Check if libjpeg headers are available
check-deps:
	@echo "Checking dependencies..."
	@if [ "$(PKG_CONFIG_EXISTS)" = "yes" ]; then \
		echo "✓ pkg-config found libjpeg: `pkg-config --modversion libjpeg`"; \
	elif [ "$(JPEGLIB_H_EXISTS)" = "yes" ]; then \
		echo "✓ jpeglib.h found (manual mode)"; \
	else \
		echo "✗ jpeglib.h not found!"; \
		echo ""; \
		echo "Install libjpeg-turbo development package:"; \
		echo "  Ubuntu/Debian: sudo apt install libjpeg-turbo8-dev pkg-config"; \
		echo "  CentOS/RHEL:   sudo yum install libjpeg-turbo-devel pkg-config"; \
		echo "  macOS:         brew install jpeg-turbo"; \
		echo ""; \
		exit 1; \
	fi

# Auto-install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing libjpeg-turbo development packages..."
	@if command -v apt >/dev/null 2>&1; then \
		echo "Detected Ubuntu/Debian - installing via apt..."; \
		sudo apt update && sudo apt install -y libjpeg-turbo8-dev build-essential pkg-config; \
	elif command -v yum >/dev/null 2>&1; then \
		echo "Detected RHEL/CentOS - installing via yum..."; \
		sudo yum install -y libjpeg-turbo-devel gcc-c++ make pkg-config; \
	elif command -v dnf >/dev/null 2>&1; then \
		echo "Detected Fedora - installing via dnf..."; \
		sudo dnf install -y libjpeg-turbo-devel gcc-c++ make pkg-config; \
	elif command -v brew >/dev/null 2>&1; then \
		echo "Detected macOS - installing via brew..."; \
		brew install jpeg-turbo; \
	else \
		echo "Unknown package manager. Please install manually:"; \
		echo "  libjpeg-turbo development headers"; \
		echo "  C++ compiler (gcc/clang)"; \
		echo "  pkg-config"; \
		exit 1; \
	fi

# Default target: libjpeg-turbo version
motion-detector: check-deps $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) $(JPEG_CFLAGS) -o $@ $(MAIN_SRC) $(JPEG_LIBS) $(LIBS)

# Static version (Pi Zero/Linux only - does not work on macOS)
static: $(MAIN_SRC)
	@echo "Building static binary for Linux/Pi Zero..."
	@if [ "$$(uname)" = "Darwin" ]; then \
		echo "Error: Static linking not supported on macOS"; \
		echo "Use 'make' for normal build instead"; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(JPEG_CFLAGS) -static -o motion-detector-static $(MAIN_SRC) $(JPEG_LIBS) $(LIBS)

# Pi Zero specific build (optimized, static)
pi-zero: $(MAIN_SRC)
	@echo "Building optimized static binary for Pi Zero..."
	$(CXX) $(CXXFLAGS) -Os -march=armv6 -mfpu=vfp -mfloat-abi=hard $(JPEG_CFLAGS) -static -o motion-detector-pi $(MAIN_SRC) $(JPEG_LIBS) $(LIBS)

# Development build with debug symbols
debug: $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) $(JPEG_CFLAGS) -g -DDEBUG -o motion-detector-debug $(MAIN_SRC) $(JPEG_LIBS) $(LIBS)

# Install to system (requires sudo)
install: motion-detector
	sudo cp motion-detector /usr/local/bin/
	sudo chmod +x /usr/local/bin/motion-detector

# Test Pi Zero compatibility
test-pi: motion-detector
	./test_pi_zero.sh

# Clean build artifacts
clean:
	rm -f motion-detector motion-detector-static motion-detector-debug motion-detector-pi

# Default target
.DEFAULT_GOAL := motion-detector

.PHONY: clean install static debug test-pi pi-zero check-deps install-deps 