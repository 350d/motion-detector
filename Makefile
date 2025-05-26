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

# Progress indicators
PROGRESS_WIDTH = 50

# Architecture detection for optimization flags
ARCH := $(shell uname -m)
ADVANCED_FLAGS = -march=native

# x86/x64 specific optimizations
ifeq ($(ARCH),x86_64)
    ADVANCED_FLAGS += -msse2 -mavx2
endif
ifeq ($(ARCH),i386)
    ADVANCED_FLAGS += -msse2
endif
ifeq ($(ARCH),i686)
    ADVANCED_FLAGS += -msse2
endif

# ARM specific optimizations (Raspberry Pi, etc.)
ifneq ($(filter arm% aarch64,$(ARCH)),)
    # Try to enable NEON if available, fallback gracefully
    ifeq ($(shell grep -q neon /proc/cpuinfo 2>/dev/null && echo yes),yes)
        ADVANCED_FLAGS += -mfpu=neon
    endif
    ADVANCED_FLAGS += -ftree-vectorize -ffast-math
endif

# macOS specific flags for better performance
ifeq ($(shell uname), Darwin)
    ADVANCED_FLAGS += -framework Accelerate
endif

all: $(TARGET_SIMPLE)

# Simple version using standard stb_image (works everywhere)
simple: $(TARGET_SIMPLE)

$(TARGET_SIMPLE): $(SOURCE_SIMPLE) stb_image.h
	@echo "$(CYAN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(YELLOW)🔨 Building Motion Detector (Simple Version)$(NC)"
	@echo "$(CYAN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)[1/4]$(NC) Checking dependencies..."
	@echo "  ✓ Source file: $(SOURCE_SIMPLE)"
	@echo "  ✓ Header file: stb_image.h"
	@echo "$(BLUE)[2/4]$(NC) Configuring compiler..."
	@echo "  ✓ Compiler: $(CXX)"
	@echo "  ✓ Flags: $(CXXFLAGS)"
	@echo "$(BLUE)[3/4]$(NC) Compiling source code..."
	@printf "  $(YELLOW)Progress: $(NC)["
	@for i in $$(seq 1 25); do printf "█"; sleep 0.02; done
	@$(CXX) $(CXXFLAGS) -o $(TARGET_SIMPLE) $(SOURCE_SIMPLE)
	@for i in $$(seq 26 50); do printf "█"; sleep 0.01; done
	@printf "] $(GREEN)100%%$(NC)\n"
	@echo "$(BLUE)[4/4]$(NC) Build complete!"
	@ls -lh $(TARGET_SIMPLE) | awk '{print "  ✓ Binary size: " $$5 " (" $$9 ")"}'
	@echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(GREEN)✅ Simple version built successfully!$(NC)"
	@echo "$(PURPLE)Usage: ./$(TARGET_SIMPLE) image1.jpg image2.jpg -v$(NC)"
	@echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"

# Advanced version with custom optimizations (may not compile on all systems)
advanced: $(TARGET)

$(TARGET): $(SOURCE) motion_stb_image.h stb_image.h
	@echo "$(CYAN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(YELLOW)🚀 Building Motion Detector (Advanced Version)$(NC)"
	@echo "$(CYAN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)[1/5]$(NC) Checking dependencies..."
	@echo "  ✓ Source file: $(SOURCE)"
	@echo "  ✓ Custom header: motion_stb_image.h"
	@echo "  ✓ Standard header: stb_image.h"
	@echo "$(BLUE)[2/5]$(NC) Detecting architecture..."
	@echo "  ✓ Architecture: $(ARCH)"
	@echo "$(BLUE)[3/5]$(NC) Configuring optimizations..."
	@echo "  ✓ Compiler: $(CXX)"
	@echo "  ✓ Base flags: $(CXXFLAGS)"
	@echo "  ✓ Advanced flags: $(ADVANCED_FLAGS)"
	@echo "$(BLUE)[4/5]$(NC) Compiling with optimizations..."
	@printf "  $(YELLOW)Progress: $(NC)["
	@for i in $$(seq 1 20); do printf "█"; sleep 0.03; done
	@$(CXX) $(CXXFLAGS) $(ADVANCED_FLAGS) -o $(TARGET) $(SOURCE)
	@for i in $$(seq 21 50); do printf "█"; sleep 0.02; done
	@printf "] $(GREEN)100%%$(NC)\n"
	@echo "$(BLUE)[5/5]$(NC) Build complete!"
	@ls -lh $(TARGET) | awk '{print "  ✓ Binary size: " $$5 " (" $$9 ")"}'
	@echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(GREEN)🎯 Advanced version built successfully!$(NC)"
	@echo "$(PURPLE)Max performance: ./$(TARGET) img1.jpg img2.jpg -g -s 4 -d --benchmark$(NC)"
	@echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"

# Debug build
debug: CXXFLAGS = -g -std=c++11 -Wall -Wextra -DDEBUG
debug: 
	@echo "$(YELLOW)🐛 Building debug version...$(NC)"
	@$(MAKE) $(TARGET) CXXFLAGS="$(CXXFLAGS)"

# Benchmark optimized build
benchmark: CXXFLAGS = -O3 -std=c++11 -DNDEBUG -march=native -mtune=native -flto
benchmark: 
	@echo "$(RED)⚡ Building maximum performance version...$(NC)"
	@$(MAKE) $(TARGET) CXXFLAGS="$(CXXFLAGS)"

# Pi debug version (safe fallback)
pi-debug: motion_detector_pi_debug.cpp stb_image.h
	@echo "$(YELLOW)🐛 Building Pi Debug Version (Safe)...$(NC)"
	@$(CXX) -g -std=c++11 -Wall -Wextra -DDEBUG -o motion-detector-pi-debug motion_detector_pi_debug.cpp
	@echo "$(GREEN)✅ Pi debug version built!$(NC)"
	@echo "$(PURPLE)Usage: ./motion-detector-pi-debug img1.jpg img2.jpg -s 4 -m 2 -g -v$(NC)"

clean:
	@echo "$(YELLOW)🧹 Cleaning build artifacts...$(NC)"
	@rm -f $(TARGET) $(TARGET_SIMPLE) motion-detector-pi-debug
	@echo "$(GREEN)✅ Clean complete!$(NC)"

install: $(TARGET_SIMPLE)
	@echo "$(BLUE)📦 Installing simple version to /usr/local/bin...$(NC)"
	@cp $(TARGET_SIMPLE) /usr/local/bin/motion-detector
	@echo "$(GREEN)✅ Installation complete!$(NC)"

install-advanced: $(TARGET)
	@echo "$(BLUE)📦 Installing advanced version to /usr/local/bin...$(NC)"
	@cp $(TARGET) /usr/local/bin/motion-detector
	@echo "$(GREEN)✅ Installation complete!$(NC)"

test: $(TARGET_SIMPLE)
	@echo "$(CYAN)🧪 Running basic functionality test...$(NC)"
	@echo "$(GREEN)✅ Simple version compiled successfully$(NC)"
	@if [ -x "./test_motion.sh" ]; then \
		echo "$(BLUE)Running automated tests...$(NC)"; \
		./test_motion.sh; \
	else \
		echo "$(YELLOW)⚠️  No test script found. Run './test_motion.sh' manually.$(NC)"; \
	fi

.PHONY: all simple advanced clean install install-advanced debug benchmark test pi-debug 