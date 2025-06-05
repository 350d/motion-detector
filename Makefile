# Motion Detector - libjpeg-turbo Version
# Optimized for Pi Zero with ARM-safe image loading and decode-time scaling

CXX = c++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
LIBS = -lm

# Source files
MAIN_SRC = motion_detector.cpp

# Try pkg-config first, fallback to manual flags
PKG_CONFIG_EXISTS = $(shell pkg-config --exists libjpeg && echo yes)

ifeq ($(PKG_CONFIG_EXISTS),yes)
	JPEG_CFLAGS = `pkg-config --cflags libjpeg`
	JPEG_LIBS = `pkg-config --libs libjpeg`
else
	# Fallback paths for common installations
	JPEG_CFLAGS = -I/usr/include -I/usr/local/include -I/opt/homebrew/include
	JPEG_LIBS = -ljpeg
endif

# Default target: libjpeg-turbo version
motion-detector: $(MAIN_SRC)
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

.PHONY: clean install static debug test-pi pi-zero 