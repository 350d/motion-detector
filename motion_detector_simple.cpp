// motion_detector_simple.cpp - Simplified motion detection utility
// Using standard stb_image with basic optimizations

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <vector>

struct MotionDetectionParams {
    int pixel_threshold = 25;      // Pixel difference threshold (0-255)
    int scale_factor = 1;          // Process every N-th pixel (1=all, 2=half, etc)
    bool use_grayscale = true;     // Convert to grayscale for 3x speed boost
    bool enable_blur = false;      // Apply 3x3 blur to reduce noise
    float motion_threshold = 1.0f; // Percentage threshold for motion detection
    bool verbose = false;          // Print detailed statistics
    bool benchmark = false;        // Show timing information
};

// Simple 3x3 blur kernel for noise reduction
void apply_blur_3x3(unsigned char* output, const unsigned char* input, 
                   int width, int height, int channels) {
    // Simple box blur kernel
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            for (int c = 0; c < channels; c++) {
                int sum = 0;
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int idx = ((y + ky) * width + (x + kx)) * channels + c;
                        sum += input[idx];
                    }
                }
                int out_idx = (y * width + x) * channels + c;
                output[out_idx] = (unsigned char)(sum / 9);
            }
        }
    }
}

// Calculate motion between two images with advanced parameters
float calculate_motion_advanced(const unsigned char* img1, const unsigned char* img2,
                              int width, int height, int channels,
                              const MotionDetectionParams& params) {
    
    int changed_pixels = 0;
    int total_pixels_checked = 0;
    
    // Determine step size based on scale factor
    int step = params.scale_factor;
    
    for (int y = 0; y < height; y += step) {
        for (int x = 0; x < width; x += step) {
            int base_idx = (y * width + x) * channels;
            
            if (params.use_grayscale && channels == 3) {
                // Convert to grayscale on the fly for comparison
                int gray1 = (77 * img1[base_idx] + 150 * img1[base_idx + 1] + 29 * img1[base_idx + 2]) >> 8;
                int gray2 = (77 * img2[base_idx] + 150 * img2[base_idx + 1] + 29 * img2[base_idx + 2]) >> 8;
                
                if (abs(gray1 - gray2) > params.pixel_threshold) {
                    changed_pixels++;
                }
            } else {
                // Process all channels
                bool pixel_changed = false;
                for (int c = 0; c < channels; c++) {
                    if (abs((int)img1[base_idx + c] - (int)img2[base_idx + c]) > params.pixel_threshold) {
                        pixel_changed = true;
                        break;
                    }
                }
                if (pixel_changed) {
                    changed_pixels++;
                }
            }
            total_pixels_checked++;
        }
    }
    
    return total_pixels_checked > 0 ? 
           (100.0f * changed_pixels / total_pixels_checked) : 0.0f;
}

void print_usage(const char* program_name) {
    std::cout << "Motion Detection Utility - Simplified version\n\n";
    std::cout << "Usage: " << program_name << " <image1> <image2> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t <threshold>     Pixel difference threshold (0-255, default: 25)\n";
    std::cout << "  -s <scale>         Process every N-th pixel for speed (default: 1)\n";
    std::cout << "  -m <motion_pct>    Motion percentage threshold (default: 1.0)\n";
    std::cout << "  -g                 Force grayscale processing (3x faster)\n";
    std::cout << "  -b                 Enable 3x3 blur filter to reduce noise\n";
    std::cout << "  -v                 Verbose output with detailed statistics\n";
    std::cout << "  --benchmark        Show timing information\n";
    std::cout << "  -h, --help         Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " frame1.jpg frame2.jpg -t 30 -s 2\n";
    std::cout << "  " << program_name << " prev.jpg curr.jpg -g -b -m 2.5\n";
    std::cout << "  " << program_name << " vid1.jpg vid2.jpg -s 4 --benchmark\n\n";
    std::cout << "Exit codes:\n";
    std::cout << "  0: No motion detected\n";
    std::cout << "  1: Motion detected\n";
    std::cout << "  2: Error\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 2;
    }
    
    const char* image1_path = argv[1];
    const char* image2_path = argv[2];
    MotionDetectionParams params;
    
    // Parse command line arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-t" && i + 1 < argc) {
            params.pixel_threshold = std::max(0, std::min(255, std::atoi(argv[++i])));
        } else if (arg == "-s" && i + 1 < argc) {
            params.scale_factor = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "-m" && i + 1 < argc) {
            params.motion_threshold = std::max(0.0f, static_cast<float>(std::atof(argv[++i])));
        } else if (arg == "-g") {
            params.use_grayscale = true;
        } else if (arg == "-b") {
            params.enable_blur = true;
        } else if (arg == "-v") {
            params.verbose = true;
        } else if (arg == "--benchmark") {
            params.benchmark = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 2;
        }
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Load images
    int width1, height1, channels1;
    int width2, height2, channels2;
    
    auto load_start = std::chrono::high_resolution_clock::now();
    
    unsigned char* img1 = stbi_load(image1_path, &width1, &height1, &channels1, 0);
    unsigned char* img2 = stbi_load(image2_path, &width2, &height2, &channels2, 0);
    
    auto load_end = std::chrono::high_resolution_clock::now();
    
    if (!img1 || !img2) {
        std::cerr << "Error: Could not load images" << std::endl;
        if (img1) stbi_image_free(img1);
        if (img2) stbi_image_free(img2);
        return 2;
    }
    
    // Verify images have same dimensions
    if (width1 != width2 || height1 != height2 || channels1 != channels2) {
        std::cerr << "Error: Images must have the same dimensions and format" << std::endl;
        std::cerr << "Image 1: " << width1 << "x" << height1 << "x" << channels1 << std::endl;
        std::cerr << "Image 2: " << width2 << "x" << height2 << "x" << channels2 << std::endl;
        stbi_image_free(img1);
        stbi_image_free(img2);
        return 2;
    }
    
    // Apply blur if requested
    std::vector<unsigned char> blurred1, blurred2;
    unsigned char* proc_img1 = img1;
    unsigned char* proc_img2 = img2;
    
    if (params.enable_blur) {
        blurred1.resize(width1 * height1 * channels1);
        blurred2.resize(width2 * height2 * channels2);
        
        apply_blur_3x3(blurred1.data(), img1, width1, height1, channels1);
        apply_blur_3x3(blurred2.data(), img2, width2, height2, channels2);
        
        proc_img1 = blurred1.data();
        proc_img2 = blurred2.data();
    }
    
    auto motion_start = std::chrono::high_resolution_clock::now();
    
    // Calculate motion
    float motion_percentage = calculate_motion_advanced(proc_img1, proc_img2,
                                                      width1, height1, channels1,
                                                      params);
    
    auto motion_end = std::chrono::high_resolution_clock::now();
    auto total_end = std::chrono::high_resolution_clock::now();
    
    // Determine if motion detected
    bool motion_detected = motion_percentage >= params.motion_threshold;
    
    // Output results
    if (params.verbose) {
        std::cout << "=== Motion Detection Results ===" << std::endl;
        std::cout << "Image dimensions: " << width1 << "x" << height1 << "x" << channels1 << std::endl;
        std::cout << "Parameters:" << std::endl;
        std::cout << "  Pixel threshold: " << params.pixel_threshold << std::endl;
        std::cout << "  Scale factor: " << params.scale_factor << std::endl;
        std::cout << "  Motion threshold: " << params.motion_threshold << "%" << std::endl;
        std::cout << "  Grayscale: " << (params.use_grayscale ? "Yes" : "No") << std::endl;
        std::cout << "  Blur filter: " << (params.enable_blur ? "Yes" : "No") << std::endl;
        std::cout << "Motion detected: " << motion_percentage << "%" << std::endl;
        std::cout << "Result: " << (motion_detected ? "MOTION" : "NO_MOTION") << std::endl;
    } else {
        // In non-verbose mode, output only 1 (motion detected) or 0 (no motion)
        std::cout << (motion_detected ? 1 : 0) << std::endl;
    }
    
    if (params.benchmark) {
        auto load_time = std::chrono::duration_cast<std::chrono::microseconds>(load_end - load_start);
        auto motion_time = std::chrono::duration_cast<std::chrono::microseconds>(motion_end - motion_start);
        auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(total_end - start_time);
        
        std::cout << "=== Performance Metrics ===" << std::endl;
        std::cout << "Load time: " << load_time.count() / 1000.0 << " ms" << std::endl;
        std::cout << "Motion calculation: " << motion_time.count() / 1000.0 << " ms" << std::endl;
        std::cout << "Total time: " << total_time.count() / 1000.0 << " ms" << std::endl;
        
        int total_pixels = width1 * height1;
        int processed_pixels = total_pixels / (params.scale_factor * params.scale_factor);
        std::cout << "Processed pixels: " << processed_pixels << " / " << total_pixels << std::endl;
        
        if (motion_time.count() > 0) {
            std::cout << "Processing speed: " << (processed_pixels / (motion_time.count() / 1000000.0)) / 1000000.0 << " MP/s" << std::endl;
        }
    }
    
    // Cleanup
    stbi_image_free(img1);
    stbi_image_free(img2);
    
    return motion_detected ? 1 : 0;
} 