// motion_detector_pi_debug.cpp - Debug version for Raspberry Pi segfault diagnosis
// Simplified version with extensive safety checks

#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <vector>
#include <sys/stat.h>

// Use only standard stb_image to avoid custom library issues
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct MotionDetectionParams {
    int pixel_threshold = 25;
    int scale_factor = 1;
    bool use_grayscale = true;
    bool enable_blur = false;
    float motion_threshold = 1.0f;
    bool dc_only_mode = false;
    bool verbose = false;
    bool benchmark = false;
};

// Safe motion calculation with extensive bounds checking
float calculate_motion_safe(const unsigned char* img1, const unsigned char* img2,
                           int width, int height, int channels,
                           const MotionDetectionParams& params) {
    
    std::cout << "Starting motion calculation..." << std::endl;
    std::cout << "Image dimensions: " << width << "x" << height << "x" << channels << std::endl;
    std::cout << "Scale factor: " << params.scale_factor << std::endl;
    
    if (!img1 || !img2) {
        std::cerr << "Error: Null image pointers" << std::endl;
        return 0.0f;
    }
    
    if (width <= 0 || height <= 0 || channels <= 0) {
        std::cerr << "Error: Invalid dimensions" << std::endl;
        return 0.0f;
    }
    
    int changed_pixels = 0;
    int total_pixels_checked = 0;
    int step = params.scale_factor;
    
    std::cout << "Processing with step size: " << step << std::endl;
    
    for (int y = 0; y < height; y += step) {
        if (y >= height) break;
        
        for (int x = 0; x < width; x += step) {
            if (x >= width) break;
            
            int base_idx = (y * width + x) * channels;
            
            // Strict bounds check
            if (base_idx < 0 || base_idx + channels - 1 >= width * height * channels) {
                std::cerr << "Buffer overflow at (" << x << "," << y << "), idx=" << base_idx << std::endl;
                continue;
            }
            
            if (params.use_grayscale && channels >= 3) {
                // Safe grayscale conversion
                int gray1 = (img1[base_idx] + img1[base_idx + 1] + img1[base_idx + 2]) / 3;
                int gray2 = (img2[base_idx] + img2[base_idx + 1] + img2[base_idx + 2]) / 3;
                
                if (abs(gray1 - gray2) > params.pixel_threshold) {
                    changed_pixels++;
                }
            } else {
                // Single channel or color comparison
                if (abs((int)img1[base_idx] - (int)img2[base_idx]) > params.pixel_threshold) {
                    changed_pixels++;
                }
            }
            
            total_pixels_checked++;
            
            // Progress indicator
            if (total_pixels_checked % 1000 == 0) {
                std::cout << "Processed " << total_pixels_checked << " pixels..." << std::endl;
            }
        }
    }
    
    std::cout << "Motion calculation complete." << std::endl;
    std::cout << "Changed pixels: " << changed_pixels << "/" << total_pixels_checked << std::endl;
    
    return total_pixels_checked > 0 ? 
           (100.0f * changed_pixels / total_pixels_checked) : 0.0f;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <image1> <image2> [-s scale] [-m threshold] [-g] [-v]" << std::endl;
        return 2;
    }
    
    const char* image1_path = argv[1];
    const char* image2_path = argv[2];
    MotionDetectionParams params;
    
    // Simple argument parsing
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-s" && i + 1 < argc) {
            params.scale_factor = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "-m" && i + 1 < argc) {
            params.motion_threshold = std::max(0.0f, (float)std::atof(argv[++i]));
        } else if (arg == "-g") {
            params.use_grayscale = true;
        } else if (arg == "-v") {
            params.verbose = true;
        } else if (arg == "-d") {
            std::cout << "Warning: DC-only mode disabled in debug version" << std::endl;
        }
    }
    
    if (params.verbose) {
        std::cout << "=== Debug Mode Active ===" << std::endl;
        std::cout << "Loading image 1: " << image1_path << std::endl;
        std::cout << "Loading image 2: " << image2_path << std::endl;
    }
    
    // Load images with standard stbi
    int width1, height1, channels1;
    int width2, height2, channels2;
    
    std::cout << "Loading first image..." << std::endl;
    unsigned char* img1 = stbi_load(image1_path, &width1, &height1, &channels1, 0);
    
    if (!img1) {
        std::cerr << "Error loading image 1: " << stbi_failure_reason() << std::endl;
        return 2;
    }
    
    std::cout << "Image 1 loaded: " << width1 << "x" << height1 << "x" << channels1 << std::endl;
    
    std::cout << "Loading second image..." << std::endl;
    unsigned char* img2 = stbi_load(image2_path, &width2, &height2, &channels2, 0);
    
    if (!img2) {
        std::cerr << "Error loading image 2: " << stbi_failure_reason() << std::endl;
        stbi_image_free(img1);
        return 2;
    }
    
    std::cout << "Image 2 loaded: " << width2 << "x" << height2 << "x" << channels2 << std::endl;
    
    // Check dimensions
    if (width1 != width2 || height1 != height2 || channels1 != channels2) {
        std::cerr << "Error: Image dimensions don't match" << std::endl;
        stbi_image_free(img1);
        stbi_image_free(img2);
        return 2;
    }
    
    std::cout << "Images loaded successfully, starting motion detection..." << std::endl;
    
    // Calculate motion
    float motion_percentage = calculate_motion_safe(img1, img2, width1, height1, channels1, params);
    
    std::cout << "Motion calculation completed successfully!" << std::endl;
    
    // Determine result
    bool motion_detected = motion_percentage >= params.motion_threshold;
    
    if (params.verbose) {
        std::cout << "Motion detected: " << motion_percentage << "%" << std::endl;
        std::cout << "Threshold: " << params.motion_threshold << "%" << std::endl;
        std::cout << "Result: " << (motion_detected ? "MOTION" : "NO_MOTION") << std::endl;
    } else {
        std::cout << motion_percentage << std::endl;
    }
    
    // Cleanup
    std::cout << "Cleaning up..." << std::endl;
    stbi_image_free(img1);
    stbi_image_free(img2);
    
    std::cout << "Program completed successfully!" << std::endl;
    return motion_detected ? 1 : 0;
} 