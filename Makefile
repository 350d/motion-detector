# Makefile for motion-detector
# Support for regular, simple, static, and Pi Zero debug builds

# Default compiler
CC ?= gcc
CXX ?= g++

# Default flags
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra

# Libraries
LIBS = -lm

# Source files
MAIN_SRC = motion_detector.cpp
SIMPLE_SRC = motion_detector_simple.cpp

# Binary names
BINARY = motion-detector
SIMPLE_BINARY = motion-detector-simple
STATIC_BINARY = motion-detector-static
PI_DEBUG_BINARY = motion-detector-pi-debug

# Default target
all: advanced

# Advanced version (optimized)
advanced: $(BINARY)

$(BINARY): $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

# Simple version (basic functionality)
simple: $(SIMPLE_BINARY)

$(SIMPLE_BINARY): $(SIMPLE_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

# Static version (no dependencies)
static: $(STATIC_BINARY)

$(STATIC_BINARY): $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -static -o $@ $< $(LIBS)

# Pi Zero debug version (maximum compatibility)
pi-debug: $(PI_DEBUG_BINARY)

$(PI_DEBUG_BINARY): $(MAIN_SRC)
	$(CXX) -std=c++11 -Os -g -Wall -Wextra \
		-fno-strict-aliasing \
		-fno-aggressive-loop-optimizations \
		-fstack-protector-strong \
		-DMOTION_PI_ZERO_DEBUG=1 \
		-DMOTION_DISABLE_DC_MODE=1 \
		-DMOTION_CONSERVATIVE_MEMORY=1 \
		-o $@ $< $(LIBS)

# Clean all binaries
clean:
	rm -f $(BINARY) $(SIMPLE_BINARY) $(STATIC_BINARY) $(PI_DEBUG_BINARY)

# Install (copy to /usr/local/bin)
install: $(BINARY)
	cp $(BINARY) /usr/local/bin/

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build advanced version (default)"
	@echo "  advanced  - Build optimized version"
	@echo "  simple    - Build basic version"
	@echo "  static    - Build static version (no dependencies)"
	@echo "  pi-debug  - Build Pi Zero debug version (safest)"
	@echo "  clean     - Remove all binaries"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  help      - Show this help"

.PHONY: all advanced simple static pi-debug clean install help 