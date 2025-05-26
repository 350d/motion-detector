// motion_detector.cpp - Advanced motion detection utility
// Optimized for video frame analysis with threshold, scaling, and blur options

#define MOTION_STB_IMAGE_IMPLEMENTATION
#include "motion_stb_image.h"

// Include standard stb_image as fallback
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <vector>
#include <sys/stat.h>

struct MotionDetectionParams {
    int pixel_threshold = 25;      // Pixel difference threshold (0-255)
    int scale_factor = 1;          // Process every N-th pixel (1=all, 2=half, etc)
    bool use_grayscale = true;     // Convert to grayscale for 3x speed boost
    bool enable_blur = false;      // Apply 3x3 blur to reduce noise
    float motion_threshold = 1.0f; // Percentage threshold for motion detection
    bool dc_only_mode = false;     // Use JPEG DC coefficients only for 10x speed
    bool dc_strict_mode = false;   // Error if DC-only fails (instead of fallback)
    bool file_size_check = false;  // Use file size comparison as fast pre-check
    float file_size_threshold = 5.0f; // File size difference threshold (%)
    bool verbose = false;          // Print detailed statistics
    bool benchmark = false;        // Show timing information
};

// Simple 3x3 blur kernel for noise reduction
void apply_blur_3x3(unsigned char* output, const unsigned char* input, 
                   int width, int height, int channels) {
    // Simple box blur kernel
    const int kernel_sum = 9;
    
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
                output[out_idx] = (unsigned char)(sum / kernel_sum);
            }
        }
    }
}

// Convert RGB to grayscale using standard luminance formula
void rgb_to_grayscale_optimized(unsigned char* grayscale, const unsigned char* rgb, 
                               int pixel_count) {
    // Use the optimized SIMD function from motion_stb_image
    motion_stbi_rgb_to_grayscale_simd(grayscale, const_cast<unsigned char*>(rgb), pixel_count);
}

// Estimate header size for different image formats
long estimate_header_size(const char* filename, long file_size) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return std::min(file_size / 4, 1024L); // Max 25% of file or 1KB
    
    ext++; // Skip the dot
    long estimated_header = 0;
    
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) {
        // JPEG headers: SOI(2) + APP0(~20) + DQT(~130) + SOF(~20) + DHT(~420) + SOS(~10) = ~600 bytes
        // Plus variable metadata (EXIF, etc.) - estimate 0.6-2KB depending on file size
        if (file_size < 2000) {
            estimated_header = 600; // Minimal JPEG headers
        } else if (file_size < 10000) {
            estimated_header = 1000; // Small JPEG with basic metadata
        } else {
            estimated_header = 1500; // Full featured JPEG
        }
    } else if (strcasecmp(ext, "png") == 0) {
        // PNG signature(8) + IHDR(25) + palette(~768) + other chunks = ~1KB
        estimated_header = file_size < 5000 ? 200 : 1000;
    } else if (strcasecmp(ext, "bmp") == 0) {
        // BMP header is typically 54 bytes + palette
        estimated_header = 1078;
    } else {
        estimated_header = std::min(file_size / 10, 1024L); // 10% or 1KB max
    }
    
    // Never let header estimate exceed 50% of file size
    return std::min(estimated_header, file_size / 2);
}

// Fast file size comparison (microsecond-level performance)
float compare_file_sizes(const char* file1, const char* file2, const MotionDetectionParams& params) {
    struct stat stat1, stat2;
    
    if (stat(file1, &stat1) != 0 || stat(file2, &stat2) != 0) {
        if (params.verbose) {
            std::cerr << "Warning: Could not get file sizes for comparison" << std::endl;
        }
        return -1.0f; // Error indicator
    }
    
    // Get raw file sizes
    long size1 = stat1.st_size;
    long size2 = stat2.st_size;
    
    // Estimate and subtract header sizes
    long header1 = estimate_header_size(file1, size1);
    long header2 = estimate_header_size(file2, size2);
    
    long content1 = std::max(1L, size1 - header1); // Avoid division by zero
    long content2 = std::max(1L, size2 - header2);
    
    // Calculate percentage difference (relative to larger file)
    long max_content = std::max(content1, content2);
    float diff_pct = 100.0f * std::abs(content1 - content2) / (float)max_content;
    
    if (params.verbose) {
        std::cout << "=== File Size Analysis ===" << std::endl;
        std::cout << "File 1: " << size1 << " bytes (" << content1 << " content after ~" << header1 << " header)" << std::endl;
        std::cout << "File 2: " << size2 << " bytes (" << content2 << " content after ~" << header2 << " header)" << std::endl;
        std::cout << "Content size difference: " << diff_pct << "%" << std::endl;
    }
    
    return diff_pct;
}

// Calculate motion between two images with advanced parameters
float calculate_motion_advanced(const unsigned char* img1, const unsigned char* img2,
                              int width, int height, int channels,
                              const MotionDetectionParams& params) {
    
    // Safety checks
    if (!img1 || !img2) {
        if (params.verbose) {
            std::cerr << "Error: Null image pointers in motion calculation" << std::endl;
        }
        return 0.0f;
    }
    
    if (width <= 0 || height <= 0 || channels <= 0) {
        if (params.verbose) {
            std::cerr << "Error: Invalid image dimensions: " << width << "x" << height << "x" << channels << std::endl;
        }
        return 0.0f;
    }
    
    int changed_pixels = 0;
    int total_pixels_checked = 0;
    
    // Determine step size based on scale factor
    int step = params.scale_factor;
    
    for (int y = 0; y < height; y += step) {
        for (int x = 0; x < width; x += step) {
            // Safety check for array bounds
            if (y >= height || x >= width) continue;
            
            int base_idx = (y * width + x) * channels;
            
            // Additional safety check for buffer overflow
            int max_idx = base_idx + channels - 1;
            int total_size = width * height * channels;
            if (max_idx >= total_size) {
                if (params.verbose) {
                    std::cerr << "Warning: Buffer overflow detected at (" << x << "," << y << "), skipping pixel" << std::endl;
                }
                continue;
            }
            
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

// Load image with motion detection optimizations
unsigned char* load_image_optimized(const char* filename, int* width, int* height, 
                                   int* channels, const MotionDetectionParams& params,
                                   motion_buffer_t* reuse_buffer = nullptr, bool* dc_fallback_used = nullptr) {
    
    // Check DC-only compatibility in strict mode
    if (params.dc_strict_mode) {
        if (!motion_stbi_test_dc_compatibility(filename)) {
            // Return nullptr to indicate DC-only incompatibility
            return nullptr;
        }
    }
    
    int motion_mode = MOTION_MODE_FULL;
    
    if (params.dc_only_mode) {
        motion_mode = MOTION_MODE_DC_ONLY;
    } else if (params.scale_factor >= 4) {
        motion_mode = MOTION_MODE_QUARTER;
    } else if (params.scale_factor >= 2) {
        motion_mode = MOTION_MODE_HALF;
    }
    
    // Try optimized loading first
    unsigned char* img = motion_stbi_load(filename, width, height, channels, 
                                        params.use_grayscale ? 1 : 0, 
                                        motion_mode, reuse_buffer);
    
    // Check if we used DC-only mode and if it succeeded
    if (dc_fallback_used) {
        // For now, we'll assume DC-only worked if we got an image
        // A more sophisticated implementation would modify motion_stbi_load 
        // to return status information
        *dc_fallback_used = false;
    }
    
    // Note: motion_stbi_load now uses stbi_load internally with optimizations
    
    return img;
}

void print_usage(const char* program_name) {
    std::cout << "Motion Detection Utility - Optimized for video frame analysis\n\n";
    std::cout << "Usage: " << program_name << " <image1> <image2> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t <threshold>     Pixel difference threshold (0-255, default: 25)\n";
    std::cout << "  -s <scale>         Process every N-th pixel for speed (default: 1)\n";
    std::cout << "  -m <motion_pct>    Motion percentage threshold (default: 1.0)\n";
    std::cout << "  -f [threshold]     Fast file size comparison mode (default: 5%)\n";
    std::cout << "  -g                 Force grayscale processing (3x faster)\n";
    std::cout << "  -b                 Enable 3x3 blur filter to reduce noise\n";
    std::cout << "  -d                 Use JPEG DC-only mode (10x faster, lower quality)\n";
    std::cout << "  --dc-strict        Use JPEG DC-only mode, error if not supported\n";
    std::cout << "  -v                 Verbose output with detailed statistics\n";
    std::cout << "  --benchmark        Show timing information\n";
    std::cout << "  -h, --help         Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " frame1.jpg frame2.jpg -t 30 -s 2\n";
    std::cout << "  " << program_name << " prev.jpg curr.jpg -g -b -m 2.5\n";
    std::cout << "  " << program_name << " vid1.jpg vid2.jpg -d -s 4 --benchmark\n";
    std::cout << "  " << program_name << " cam1.jpg cam2.jpg -f 10 -v  # Fast file size check\n\n";
    std::cout << "Fast Mode (-f):\n";
    std::cout << "  Compares file sizes (minus headers) for ultra-fast pre-screening.\n";
    std::cout << "  Useful for video streams where file size changes indicate motion.\n";
    std::cout << "  Speed: ~1 microsecond vs ~1 millisecond for full analysis.\n\n";
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
            params.motion_threshold = std::max(0.0f, (float)std::atof(argv[++i]));
        } else if (arg == "-f") {
            params.file_size_check = true;
            // Optional threshold parameter
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                params.file_size_threshold = std::max(0.0f, (float)std::atof(argv[++i]));
            }
        } else if (arg == "-g") {
            params.use_grayscale = true;
        } else if (arg == "-b") {
            params.enable_blur = true;
        } else if (arg == "-d") {
            params.dc_only_mode = true;
        } else if (arg == "--dc-strict") {
            params.dc_only_mode = true;
            params.dc_strict_mode = true;
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
    
    // Fast file size comparison mode
    if (params.file_size_check) {
        auto filesize_start = std::chrono::high_resolution_clock::now();
        
        float size_diff = compare_file_sizes(image1_path, image2_path, params);
        
        auto filesize_end = std::chrono::high_resolution_clock::now();
        auto filesize_time = std::chrono::duration_cast<std::chrono::nanoseconds>(filesize_end - filesize_start);
        
        if (size_diff < 0) {
            std::cerr << "Error: Could not compare file sizes" << std::endl;
            return 2;
        }
        
        bool size_motion_detected = size_diff >= params.file_size_threshold;
        
        // Output results
        if (params.verbose) {
            std::cout << "=== File Size Comparison Results ===" << std::endl;
            std::cout << "Content size difference: " << size_diff << "%" << std::endl;
            std::cout << "Size threshold: " << params.file_size_threshold << "%" << std::endl;
            std::cout << "Result: " << (size_motion_detected ? "SIZE_CHANGE" : "NO_SIZE_CHANGE") << std::endl;
        } else {
            std::cout << size_diff << std::endl;
        }
        
        if (params.benchmark) {
            std::cout << "=== Performance Metrics ===" << std::endl;
            std::cout << "File size comparison: " << filesize_time.count() / 1000.0 << " microseconds" << std::endl;
            std::cout << "Speed: ~" << (2000000.0 / filesize_time.count()) << " files/second" << std::endl;
        }
        
        return size_motion_detected ? 1 : 0;
    }
    
    // Create separate buffers for each image to avoid cache conflicts
    motion_buffer_t* buffer1 = motion_stbi_buffer_create(1920 * 1080 * 3); // HD buffer for img1
    motion_buffer_t* buffer2 = motion_stbi_buffer_create(1920 * 1080 * 3); // HD buffer for img2
    
    if (params.verbose) {
        std::cout << "=== Debug Info ===" << std::endl;
        std::cout << "Loading image 1: " << image1_path << std::endl;
        std::cout << "Loading image 2: " << image2_path << std::endl;
        std::cout << "DC-only mode: " << (params.dc_only_mode ? "Yes" : "No") << std::endl;
        std::cout << "Buffer1 created: " << (buffer1 ? "Yes" : "No") << std::endl;
        std::cout << "Buffer2 created: " << (buffer2 ? "Yes" : "No") << std::endl;
    }
    
    // Load images
    int width1, height1, channels1;
    int width2, height2, channels2;
    
    auto load_start = std::chrono::high_resolution_clock::now();
    
    if (params.verbose) {
        std::cout << "Starting image loading..." << std::endl;
    }
    
    unsigned char* img1 = load_image_optimized(image1_path, &width1, &height1, 
                                             &channels1, params, nullptr);
    
    if (params.verbose) {
        std::cout << "Image 1 loaded: " << (img1 ? "Success" : "Failed") << std::endl;
        if (img1) {
            std::cout << "Image 1 dimensions: " << width1 << "x" << height1 << "x" << channels1 << std::endl;
        }
    }
                                             
    unsigned char* img2 = load_image_optimized(image2_path, &width2, &height2, 
                                             &channels2, params, nullptr);
    
    if (params.verbose) {
        std::cout << "Image 2 loaded: " << (img2 ? "Success" : "Failed") << std::endl;
        if (img2) {
            std::cout << "Image 2 dimensions: " << width2 << "x" << height2 << "x" << channels2 << std::endl;
        }
    }
    
    auto load_end = std::chrono::high_resolution_clock::now();
    
    if (!img1 || !img2) {
        if (params.dc_strict_mode) {
            if (!img1) {
                std::cerr << "Error: Image 1 (" << image1_path << ") is not compatible with JPEG DC-only mode" << std::endl;
                std::cerr << "Possible reasons:" << std::endl;
                std::cerr << "  - Not a JPEG file" << std::endl;
                std::cerr << "  - Progressive JPEG (not supported)" << std::endl;
                std::cerr << "  - Malformed JPEG headers" << std::endl;
                std::cerr << "  - Unsupported JPEG variant" << std::endl;
            }
            if (!img2) {
                std::cerr << "Error: Image 2 (" << image2_path << ") is not compatible with JPEG DC-only mode" << std::endl;
                std::cerr << "Possible reasons:" << std::endl;
                std::cerr << "  - Not a JPEG file" << std::endl;
                std::cerr << "  - Progressive JPEG (not supported)" << std::endl;
                std::cerr << "  - Malformed JPEG headers" << std::endl;
                std::cerr << "  - Unsupported JPEG variant" << std::endl;
            }
            std::cerr << "Suggestion: Use -d instead of --dc-strict for automatic fallback" << std::endl;
        } else {
            std::cerr << "Error: Could not load images" << std::endl;
        }
        motion_stbi_image_free(img1);
        motion_stbi_image_free(img2);
        if (buffer1) motion_stbi_buffer_free(buffer1);
        if (buffer2) motion_stbi_buffer_free(buffer2);
        return 2;
    }
    
    // Verify images have same dimensions
    if (width1 != width2 || height1 != height2 || channels1 != channels2) {
        std::cerr << "Error: Images must have the same dimensions and format" << std::endl;
        std::cerr << "Image 1: " << width1 << "x" << height1 << "x" << channels1 << std::endl;
        std::cerr << "Image 2: " << width2 << "x" << height2 << "x" << channels2 << std::endl;
        motion_stbi_image_free(img1);
        motion_stbi_image_free(img2);
        if (buffer1) motion_stbi_buffer_free(buffer1);
        if (buffer2) motion_stbi_buffer_free(buffer2);
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
        std::cout << "  DC-only mode: " << (params.dc_only_mode ? "Yes" : "No") << std::endl;
        std::cout << "  File size check: " << (params.file_size_check ? "Yes" : "No") << std::endl;
        std::cout << "Motion detected: " << motion_percentage << "%" << std::endl;
        std::cout << "Result: " << (motion_detected ? "MOTION" : "NO_MOTION") << std::endl;
    } else {
        std::cout << motion_percentage << std::endl;
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
        std::cout << "Processing speed: " << (processed_pixels / (motion_time.count() / 1000000.0)) / 1000000.0 << " MP/s" << std::endl;
    }
    
    // Cleanup
    motion_stbi_image_free(img1);
    motion_stbi_image_free(img2);
    
    // Clean up buffers (safe to call even if not used)
    if (buffer1) motion_stbi_buffer_free(buffer1);
    if (buffer2) motion_stbi_buffer_free(buffer2);
    
    return motion_detected ? 1 : 0;
} 