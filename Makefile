# Motion Detector - Universal Version
# Supports all platforms with automatic optimizations and fallbacks

CXX = c++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
LIBS = -lm

# Source files
MAIN_SRC = motion_detector.cpp
HEADERS = motion_stb_image.h stb_image.h

# Default target: Universal version
motion-detector: $(MAIN_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(MAIN_SRC) $(LIBS)

# Pi Zero debug version (for troubleshooting only)
pi-debug: $(MAIN_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DMOTION_PI_ZERO_DEBUG -o motion-detector-pi-debug $(MAIN_SRC) $(LIBS)

# Development build with debug symbols
debug: $(MAIN_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -g -DDEBUG -o motion-detector-debug $(MAIN_SRC) $(LIBS)

# Install to system (requires sudo)
install: motion-detector
	sudo cp motion-detector /usr/local/bin/
	sudo chmod +x /usr/local/bin/motion-detector

# Clean build artifacts
clean:
	rm -f motion-detector motion-detector-pi-debug motion-detector-debug

# Default target
.DEFAULT_GOAL := motion-detector

.PHONY: clean install pi-debug debug 