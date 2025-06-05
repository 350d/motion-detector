# Motion Detector - libjpeg-turbo Version
# Optimized for Pi Zero with ARM-safe image loading and decode-time scaling

CXX = c++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra
LIBS = -lm

# Source files
MAIN_SRC = motion_detector.cpp

# Default target: libjpeg-turbo version
motion-detector: $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) `pkg-config --cflags libjpeg` -o $@ $(MAIN_SRC) `pkg-config --libs libjpeg` $(LIBS)

# Static version (Pi Zero - may not work on all systems)
static: $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) `pkg-config --cflags libjpeg` -static -o motion-detector-static $(MAIN_SRC) `pkg-config --libs libjpeg` $(LIBS)

# Development build with debug symbols
debug: $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) `pkg-config --cflags libjpeg` -g -DDEBUG -o motion-detector-debug $(MAIN_SRC) `pkg-config --libs libjpeg` $(LIBS)

# Install to system (requires sudo)
install: motion-detector
	sudo cp motion-detector /usr/local/bin/
	sudo chmod +x /usr/local/bin/motion-detector

# Test Pi Zero compatibility
test-pi: motion-detector
	./test_pi_zero.sh

# Clean build artifacts
clean:
	rm -f motion-detector motion-detector-static motion-detector-debug

# Default target
.DEFAULT_GOAL := motion-detector

.PHONY: clean install static debug test-pi 