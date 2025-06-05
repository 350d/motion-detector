// motion_detector.cpp - Advanced motion detection utility
// Optimized for video frame analysis with threshold, scaling, and blur options

// Pi Zero debug mode - conservative settings
#ifdef MOTION_PI_ZERO_DEBUG
    #warning "Building Pi Zero debug version with realistic settings for 512MB RAM"
    #define MOTION_DISABLE_DC_MODE 1
    #define MOTION_CONSERVATIVE_MEMORY 1
    #define MOTION_MAX_SAFE_IMAGE_SIZE (1280*720*3)  // HD for 512MB Pi Zero (~2.8MB per image, ~20MB total)
    #define MOTION_ENABLE_BOUNDS_CHECKING 1
#endif

// Pi Zero advanced mode - full features with safety fallbacks
#ifdef MOTION_PI_ZERO_ADVANCED
    #warning "Building Pi Zero advanced version with safety fallbacks"
    #define MOTION_MAX_SAFE_IMAGE_SIZE (1536*1024*3)  // 1.5MP max
    #define MOTION_ENABLE_BOUNDS_CHECKING 1
    // Keep all advanced features but with fallbacks
#endif

// Conservative memory mode
#ifdef MOTION_CONSERVATIVE_MEMORY
    #define MOTION_MAX_BLUR_SIZE (1920*1080*3)  // Full HD max for blur (safe on 512MB)
    #define MOTION_STACK_PROTECT 1
#endif

#define MOTION_STB_IMAGE_IMPLEMENTATION
#include "motion_stb_image.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <vector>
#include <sys/stat.h>
#include <iomanip>
#include <signal.h>
#include <string>

// Signal handler for segfault debugging
#ifdef MOTION_PI_ZERO_DEBUG
void segfault_handler(int sig) {
    std::cerr << "\n!!! SEGFAULT DETECTED in Pi Zero debug mode !!!" << std::endl;
    std::cerr << "Signal: " << sig << std::endl;
    std::cerr << "This usually indicates:" << std::endl;
    std::cerr << "  - Out of memory (Pi Zero has limited RAM)" << std::endl;
    std::cerr << "  - Image file corrupted or too large for stb_image" << std::endl;
    std::cerr << "  - ARM alignment issues with JPEG decoding" << std::endl;
    std::cerr << "  - Stack overflow from recursive JPEG parsing" << std::endl;
    std::cerr << "Try:" << std::endl;
    std::cerr << "  - Check image file integrity: file /path/to/image.jpg" << std::endl;
    std::cerr << "  - Reduce image size: convert -resize 800x600 input.jpg output.jpg" << std::endl;
    std::cerr << "  - Use file size mode: -f (bypasses image loading entirely)" << std::endl;
    std::cerr << "  - Test with simple images first (PNG, small JPEG)" << std::endl;
    exit(3);
}
#endif

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

#ifdef MOTION_PI_ZERO_DEBUG
// Pi Zero specific file validation
bool validate_image_file_pi_zero(const char* filename, bool verbose) {
    if (!filename) return false;
    
    // Check file exists and is readable
    FILE* f = fopen(filename, "rb");
    if (!f) {
        if (verbose) {
            std::cerr << "Pi Zero debug: Cannot open file " << filename << std::endl;
        }
        return false;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 50 * 1024 * 1024) { // Max 50MB file
        if (verbose) {
            std::cerr << "Pi Zero debug: File size " << file_size << " bytes is invalid or too large" << std::endl;
        }
        fclose(f);
        return false;
    }
    
    // Check file signature
    unsigned char sig[4] = {0};
    size_t read_bytes = fread(sig, 1, 4, f);
    fclose(f);
    
    if (read_bytes < 4) {
        if (verbose) {
            std::cerr << "Pi Zero debug: Cannot read file signature" << std::endl;
        }
        return false;
    }
    
    // Check for supported formats
    bool is_valid = false;
    if (sig[0] == 0xFF && sig[1] == 0xD8) {
        is_valid = true; // JPEG
        if (verbose) std::cout << "Pi Zero debug: JPEG file detected (" << file_size << " bytes)" << std::endl;
    } else if (sig[0] == 0x89 && sig[1] == 0x50 && sig[2] == 0x4E && sig[3] == 0x47) {
        is_valid = true; // PNG
        if (verbose) std::cout << "Pi Zero debug: PNG file detected (" << file_size << " bytes)" << std::endl;
    } else if (sig[0] == 0x42 && sig[1] == 0x4D) {
        is_valid = true; // BMP
        if (verbose) std::cout << "Pi Zero debug: BMP file detected (" << file_size << " bytes)" << std::endl;
    }
    
    if (!is_valid && verbose) {
        std::cerr << "Pi Zero debug: Unknown file format (signature: 0x" 
                  << std::hex << (int)sig[0] << (int)sig[1] << (int)sig[2] << (int)sig[3] << std::dec << ")" << std::endl;
    }
    
    return is_valid;
}
#endif

// Check if image is safe to process on Pi Zero with scale factor consideration
bool is_image_safe_for_pi_zero(int width, int height, int channels, bool verbose, int scale_factor = 1) {
    (void)verbose; (void)scale_factor; // Suppress unused parameter warnings
    size_t image_size = (size_t)width * (size_t)height * (size_t)channels;
    (void)image_size; // Suppress unused variable warning
    
    #ifdef MOTION_PI_ZERO_DEBUG
    // Calculate effective memory usage with scale factor
    // Scale factor reduces memory usage quadratically: -s 2 = 4x less, -s 4 = 16x less
    size_t effective_memory = image_size / (scale_factor * scale_factor);
    
    // Realistic limits for Pi Zero 512MB RAM - loading memory constraints  
    if (width > 1280 || height > 720) {
        if (verbose) {
            std::cerr << "Pi Zero Warning: Image " << width << "x" << height << 
                " exceeds safe loading resolution (1280x720)." << std::endl;
            std::cerr << "Note: Images are loaded fully into memory regardless of scale factor." << std::endl;
        }
        return false;
    }
    
    // Memory check with scale factor consideration
    // Allow up to ~150MB effective memory usage (with 2x images + buffers = ~450MB total)
    size_t safe_memory_limit = 150 * 1024 * 1024; // 150MB effective
    if (effective_memory > safe_memory_limit) {
        if (verbose) {
            std::cerr << "Pi Zero Warning: Effective memory usage " << (effective_memory/1024/1024) << 
                "MB (with scale factor " << scale_factor << ") exceeds safe limit (" << 
                (safe_memory_limit/1024/1024) << "MB)." << std::endl;
            std::cerr << "Try higher scale factor: -s " << (scale_factor * 2) << " or -s " << (scale_factor * 4) << std::endl;
        }
        return false;
    }
    
    if (verbose && effective_memory > 50 * 1024 * 1024) {
        std::cerr << "Pi Zero Info: Large image detected (" << (effective_memory/1024/1024) << 
            "MB effective with -s " << scale_factor << "). Consider -s " << (scale_factor * 2) << 
            " for better performance." << std::endl;
    }
    #endif
    
    return true;
}

// Simple 3x3 blur kernel for noise reduction - Pi Zero safe version
void apply_blur_3x3(unsigned char* output, const unsigned char* input, 
                   int width, int height, int channels) {
    // Safety checks
    if (!output || !input || width <= 0 || height <= 0 || channels <= 0) {
        return;
    }
    
    #ifdef MOTION_CONSERVATIVE_MEMORY
    // Skip blur for large images on Pi Zero
    size_t total_size = (size_t)width * (size_t)height * (size_t)channels;
    if (total_size > MOTION_MAX_BLUR_SIZE) {
        // Just copy input to output without blur for safety
        std::memcpy(output, input, total_size);
        return;
    }
    #endif
    
    // For very large images, skip blur processing entirely to avoid crashes
    if (width > 4096 || height > 4096) {
        // Just copy input to output without blur for safety
        size_t total_size = (size_t)width * (size_t)height * (size_t)channels;
        size_t chunk_size = 1024 * 1024; // 1MB chunks
        for (size_t offset = 0; offset < total_size; offset += chunk_size) {
            size_t copy_size = (total_size - offset > chunk_size) ? chunk_size : (total_size - offset);
            std::memcpy(output + offset, input + offset, copy_size);
        }
        return;
    }
    
    // Calculate total size safely
    size_t row_size = (size_t)width * (size_t)channels;
    
    // Copy input to output first (handle edges by keeping original values)
    for (int y = 0; y < height; y++) {
        size_t row_offset = (size_t)y * row_size;
        std::memcpy(output + row_offset, input + row_offset, row_size);
    }
    
    // Apply blur only to interior pixels (1 pixel border remains unblurred)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            for (int c = 0; c < channels; c++) {
                // Calculate 3x3 average manually for safety
                int sum = 0;
                
                // Top row
                sum += input[((size_t)(y-1) * (size_t)width + (size_t)(x-1)) * (size_t)channels + (size_t)c];
                sum += input[((size_t)(y-1) * (size_t)width + (size_t)x) * (size_t)channels + (size_t)c];
                sum += input[((size_t)(y-1) * (size_t)width + (size_t)(x+1)) * (size_t)channels + (size_t)c];
                
                // Middle row
                sum += input[((size_t)y * (size_t)width + (size_t)(x-1)) * (size_t)channels + (size_t)c];
                sum += input[((size_t)y * (size_t)width + (size_t)x) * (size_t)channels + (size_t)c];
                sum += input[((size_t)y * (size_t)width + (size_t)(x+1)) * (size_t)channels + (size_t)c];
                
                // Bottom row
                sum += input[((size_t)(y+1) * (size_t)width + (size_t)(x-1)) * (size_t)channels + (size_t)c];
                sum += input[((size_t)(y+1) * (size_t)width + (size_t)x) * (size_t)channels + (size_t)c];
                sum += input[((size_t)(y+1) * (size_t)width + (size_t)(x+1)) * (size_t)channels + (size_t)c];
                
                // Store result
                output[((size_t)y * (size_t)width + (size_t)x) * (size_t)channels + (size_t)c] = 
                    (unsigned char)(sum / 9);
            }
        }
    }
}

// Convert RGB to grayscale using standard luminance formula
void rgb_to_grayscale_optimized(unsigned char* grayscale, const unsigned char* rgb, 
                               int pixel_count) {
    // Simple RGB to grayscale conversion
    for (int i = 0; i < pixel_count; i++) {
        int r = rgb[i * 3];
        int g = rgb[i * 3 + 1];
        int b = rgb[i * 3 + 2];
        grayscale[i] = (unsigned char)((77 * r + 150 * g + 29 * b) >> 8);
    }
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
        std::cout << "Content size difference: " << std::fixed << std::setprecision(2) << diff_pct << "%" << std::endl;
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
    
    // Calculate total image size for bounds checking
    size_t total_size = (size_t)width * (size_t)height * (size_t)channels;
    
    for (int y = 0; y < height; y += step) {
        for (int x = 0; x < width; x += step) {
            // Safety check for array bounds
            if (y >= height || x >= width) continue;
            
            size_t base_idx = ((size_t)y * (size_t)width + (size_t)x) * (size_t)channels;
            
            // Additional safety check for buffer overflow
            size_t max_idx = base_idx + (size_t)channels - 1;
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

// Optimized image loader with intelligent mode selection
unsigned char* load_image_optimized(const char* filename, int* width, int* height, 
                                   int* channels, const MotionDetectionParams& params,
                                   motion_buffer_t* reuse_buffer = nullptr) {
    
    unsigned char* img = nullptr;  // Declare once for both paths
    
    #ifdef MOTION_PI_ZERO_DEBUG
    // Pi Zero debug mode: additional safety checks before loading
    if (params.verbose) {
        std::cout << "Pi Zero debug mode: performing safety checks" << std::endl;
    }
    
    // First validate file format and accessibility
    if (!validate_image_file_pi_zero(filename, params.verbose)) {
        if (params.verbose) {
            std::cerr << "Pi Zero debug: File validation failed for " << filename << std::endl;
        }
        return nullptr;
    }
    
    // Check file existence and basic info
    int test_w, test_h, test_c;
    if (!stbi_info(filename, &test_w, &test_h, &test_c)) {
        if (params.verbose) {
            std::cerr << "Pi Zero debug: Cannot read image info from " << filename << std::endl;
            std::cerr << "Pi Zero debug: This might indicate corrupted JPEG headers or unsupported format" << std::endl;
        }
        return nullptr;
    }
    
    if (params.verbose) {
        std::cout << "Pi Zero debug: Image info " << test_w << "x" << test_h << "x" << test_c << std::endl;
    }
    
    // Strict memory check for Pi Zero
    size_t required_memory = (size_t)test_w * test_h * test_c;
    if (required_memory > MOTION_MAX_SAFE_IMAGE_SIZE) {
        if (params.verbose) {
            std::cerr << "Pi Zero debug: Image too large - " << (required_memory / 1024 / 1024) 
                      << "MB exceeds " << (MOTION_MAX_SAFE_IMAGE_SIZE / 1024 / 1024) << "MB limit" << std::endl;
        }
        return nullptr;
    }
    
    if (params.verbose) {
        std::cout << "Pi Zero debug: Loading with standard stb_image (" 
                  << (required_memory / 1024 / 1024) << "MB)" << std::endl;
    }
    
    // Use only standard stb_image for maximum compatibility
    img = stbi_load(filename, width, height, channels, 
                   params.use_grayscale ? 1 : 0);
                                  
    if (params.verbose) {
        std::cout << "Pi Zero debug: Load result " << (img ? "success" : "failed") << std::endl;
        if (img) {
            std::cout << "Pi Zero debug: Final dimensions " << *width << "x" << *height << "x" << *channels << std::endl;
        }
    }
    
    return img;
    #endif
    
    // Intelligent mode selection based on parameters and JPEG compatibility
    int motion_mode = MOTION_MODE_FULL;
    
    // First check if it's a JPEG and predict its size
    int predicted_width = 0, predicted_height = 0;
    bool is_jpeg = motion_stbi_test_jpeg_compatibility(filename, &predicted_width, &predicted_height);
    
    if (params.verbose && is_jpeg) {
        std::cout << "JPEG detected: " << predicted_width << "x" << predicted_height 
                  << " (estimated " << (predicted_width * predicted_height * 3 / 1024) << " KB)" << std::endl;
    }
    
    // Select optimal mode based on requirements and image characteristics
    if (params.dc_only_mode && is_jpeg) {
        motion_mode = MOTION_MODE_DC_ONLY;
        if (params.verbose) {
            std::cout << "Using DC-only mode for ultra-fast JPEG preview" << std::endl;
        }
    } else if (params.scale_factor >= 8 || (is_jpeg && predicted_width > 2560)) {
        motion_mode = MOTION_MODE_EIGHTH;
        if (params.verbose) {
            std::cout << "Using 1/8 scale mode for large image optimization" << std::endl;
        }
    } else if (params.scale_factor >= 4 || (is_jpeg && predicted_width > 1280)) {
        motion_mode = MOTION_MODE_QUARTER;
        if (params.verbose) {
            std::cout << "Using 1/4 scale mode for memory efficiency" << std::endl;
        }
    } else if (params.scale_factor >= 2 || (is_jpeg && predicted_width > 640)) {
        motion_mode = MOTION_MODE_HALF;
        if (params.verbose) {
            std::cout << "Using 1/2 scale mode for balanced performance" << std::endl;
        }
    }
    
    // Use optimized loader with JPEG-specific optimizations
    img = motion_stbi_load(filename, width, height, channels, 
                          params.use_grayscale ? 1 : 0, 
                          motion_mode, reuse_buffer);
    
    if (!img && params.dc_only_mode && params.dc_strict_mode) {
        if (params.verbose) {
            std::cerr << "Error: DC-only mode failed and strict mode enabled" << std::endl;
        }
        return nullptr;
    }
    
    if (params.verbose && img) {
        std::cout << "Image loaded successfully (" << *width << "x" << *height 
                  << ", " << *channels << " channels)" << std::endl;
        if (motion_mode != MOTION_MODE_FULL) {
            std::cout << "Applied optimization mode: " << 
                (motion_mode == MOTION_MODE_HALF ? "1/2 scale" :
                 motion_mode == MOTION_MODE_QUARTER ? "1/4 scale" :
                 motion_mode == MOTION_MODE_EIGHTH ? "1/8 scale" :
                 motion_mode == MOTION_MODE_DC_ONLY ? "DC-only preview" : "full") << std::endl;
        }
    }
    
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
    std::cout << "  " << program_name << " cam1.jpg cam2.jpg -f 10 -v  # Fast file size check\n";
    std::cout << "  " << program_name << " img1.jpg img2.jpg -g -s 4 && echo \"Motion!\"\n";
    std::cout << "  result=$(" << program_name << " img1.jpg img2.jpg -g); echo \"Status: $result\"\n\n";
    std::cout << "Fast Mode (-f):\n";
    std::cout << "  Compares file sizes (minus headers) for ultra-fast pre-screening.\n";
    std::cout << "  Useful for video streams where file size changes indicate motion.\n";
    std::cout << "  Speed: ~1 microsecond vs ~1 millisecond for full analysis.\n\n";
    std::cout << "Output:\n";
    std::cout << "  Default mode: Outputs 1 (motion detected) or 0 (no motion)\n";
    std::cout << "  Verbose mode (-v): Detailed statistics and percentages\n\n";
    std::cout << "Script Integration:\n";
    std::cout << "  Check exit codes: $? in bash (0=no motion, 1=motion, 2=error)\n";
    std::cout << "  Capture output: result=$(./motion-detector img1.jpg img2.jpg)\n";
    std::cout << "  Silent mode: Use default mode (no -v) for clean 1/0 output\n";
    std::cout << "  Example: if [ $? -eq 1 ]; then echo \"Motion detected!\"; fi\n\n";
    std::cout << "Exit codes:\n";
    std::cout << "  0: No motion detected\n";
    std::cout << "  1: Motion detected\n";
    std::cout << "  2: Error\n";
}

int main(int argc, char* argv[]) {
    #ifdef MOTION_PI_ZERO_DEBUG
    // Install segfault handler for debugging
    signal(SIGSEGV, segfault_handler);
    signal(SIGBUS, segfault_handler);
    signal(SIGILL, segfault_handler);
    
    std::cout << "=== Pi Zero Debug Mode Active ===" << std::endl;
    std::cout << "Conservative memory limits enabled" << std::endl;
    std::cout << "Max safe image size: " << (MOTION_MAX_SAFE_IMAGE_SIZE/1024/1024) << "MB" << std::endl;
    std::cout << "Automatic fallback to standard stb_image" << std::endl;
    std::cout << "===============================\n" << std::endl;
    #endif

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
            #ifdef MOTION_DISABLE_DC_MODE
            if (params.verbose) {
                std::cout << "Warning: DC-only mode disabled in Pi Zero debug build" << std::endl;
            }
            params.dc_only_mode = false;
            #else
            params.dc_only_mode = true;
            #endif
        } else if (arg == "--dc-strict") {
            #ifdef MOTION_DISABLE_DC_MODE
            if (params.verbose) {
                std::cout << "Warning: DC-strict mode disabled in Pi Zero debug build" << std::endl;
            }
            params.dc_only_mode = false;
            params.dc_strict_mode = false;
            #else
            params.dc_only_mode = true;
            params.dc_strict_mode = true;
            #endif
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
            std::cout << "Content size difference: " << std::fixed << std::setprecision(2) << size_diff << "%" << std::endl;
            std::cout << "Size threshold: " << std::fixed << std::setprecision(2) << params.file_size_threshold << "%" << std::endl;
            std::cout << "Result: " << (size_motion_detected ? "SIZE_CHANGE" : "NO_SIZE_CHANGE") << std::endl;
        } else {
            // In non-verbose mode, output only 1 (size change detected) or 0 (no size change)
            std::cout << (size_motion_detected ? 1 : 0) << std::endl;
        }
        
        if (params.benchmark) {
            std::cout << "=== Performance Metrics ===" << std::endl;
            std::cout << "File size comparison: " << filesize_time.count() / 1000.0 << " microseconds" << std::endl;
            std::cout << "Speed: ~" << (2000000.0 / filesize_time.count()) << " files/second" << std::endl;
        }
        
        return size_motion_detected ? 1 : 0;
    }
    
    // Create buffers - they will be used if optimization is available
    size_t default_buffer_size = 1920 * 1080 * 3; // HD buffer default
    motion_buffer_t* buffer1 = motion_stbi_buffer_create(default_buffer_size);
    motion_buffer_t* buffer2 = motion_stbi_buffer_create(default_buffer_size);
    
    if (params.verbose) {
        std::cout << "=== Debug Info ===" << std::endl;
        std::cout << "Loading image 1: " << image1_path << std::endl;
        std::cout << "Loading image 2: " << image2_path << std::endl;
        std::cout << "DC-only mode: " << (params.dc_only_mode ? "Yes" : "No") << std::endl;
        std::cout << "Buffers created: " << (buffer1 && buffer2 ? "Yes" : "No") << std::endl;
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
            size_t img1_size = (size_t)width1 * height1 * channels1;
            std::cout << "Image 1 size: " << img1_size << " bytes (" << (img1_size / 1024.0 / 1024.0) << " MB)" << std::endl;
        }
    }
                                             
    unsigned char* img2 = load_image_optimized(image2_path, &width2, &height2, 
                                             &channels2, params, nullptr);
    
    if (params.verbose) {
        std::cout << "Image 2 loaded: " << (img2 ? "Success" : "Failed") << std::endl;
        if (img2) {
            std::cout << "Image 2 dimensions: " << width2 << "x" << height2 << "x" << channels2 << std::endl;
            size_t img2_size = (size_t)width2 * height2 * channels2;
            std::cout << "Image 2 size: " << img2_size << " bytes (" << (img2_size / 1024.0 / 1024.0) << " MB)" << std::endl;
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
        // Universal cleanup
        if (img1) motion_stbi_image_free(img1);
        if (img2) motion_stbi_image_free(img2);
        if (buffer1) motion_stbi_buffer_free(buffer1);
        if (buffer2) motion_stbi_buffer_free(buffer2);
        return 2;
    }
    
    // Verify images have same dimensions
    if (width1 != width2 || height1 != height2 || channels1 != channels2) {
        std::cerr << "Error: Images must have the same dimensions and format" << std::endl;
        std::cerr << "Image 1: " << width1 << "x" << height1 << "x" << channels1 << std::endl;
        std::cerr << "Image 2: " << width2 << "x" << height2 << "x" << channels2 << std::endl;
        // Universal cleanup
        if (img1) motion_stbi_image_free(img1);
        if (img2) motion_stbi_image_free(img2);
        if (buffer1) motion_stbi_buffer_free(buffer1);
        if (buffer2) motion_stbi_buffer_free(buffer2);
        return 2;
    }

    #ifdef MOTION_PI_ZERO_DEBUG
    // Pi Zero safety check BEFORE loading images (actual memory usage)
    // Get image info without loading full image
    int test_width, test_height, test_channels;
    if (stbi_info(image1_path, &test_width, &test_height, &test_channels)) {
        if (!is_image_safe_for_pi_zero(test_width, test_height, test_channels, params.verbose, 1)) {
            std::cerr << "Error: Image 1 too large for Pi Zero 512MB RAM" << std::endl;
            std::cerr << "Image size: " << test_width << "x" << test_height << "x" << test_channels << std::endl;
            std::cerr << "Memory needed: " << ((size_t)test_width * test_height * test_channels / 1024 / 1024) << "MB per image" << std::endl;
            std::cerr << "Note: Scale factor (-s) does NOT reduce loading memory, only processing" << std::endl;
            std::cerr << "Recommendations:" << std::endl;
            std::cerr << "  - Resize images before processing (recommended max: 1024x768)" << std::endl;
            std::cerr << "  - Use file size mode: -f for ultra-fast processing" << std::endl;
            std::cerr << "  - Try JPEG quality reduction to decrease file complexity" << std::endl;
            return 2;
        }
    }
    
    // After successful loading, do final safety check
    if (!is_image_safe_for_pi_zero(width1, height1, channels1, params.verbose, params.scale_factor)) {
        std::cerr << "Error: Image too large for Pi Zero 512MB RAM" << std::endl;
        std::cerr << "Recommendations:" << std::endl;
        std::cerr << "  - Resize images to 8K (7680x4320) or smaller" << std::endl;
        std::cerr << "  - For large images use scale factor: -s 2, -s 4, or -s 8" << std::endl;
        std::cerr << "  - Scale factor reduces memory usage by 4x, 16x, or 64x respectively" << std::endl;
        std::cerr << "  - Try file size mode: -f for ultra-fast processing" << std::endl;
        // Universal cleanup
        if (img1) motion_stbi_image_free(img1);
        if (img2) motion_stbi_image_free(img2);
        if (buffer1) motion_stbi_buffer_free(buffer1);
        if (buffer2) motion_stbi_buffer_free(buffer2);
        return 2;
    }
    #endif
    
    // Apply blur if requested
    std::vector<unsigned char> blurred1, blurred2;
    unsigned char* proc_img1 = img1;
    unsigned char* proc_img2 = img2;
    
    if (params.enable_blur) {
        size_t blur_buffer_size = (size_t)width1 * (size_t)height1 * (size_t)channels1;
        
        // Check if we should skip blur for safety (HD images are at the limit)
        if (width1 > 4096 || height1 > 4096) {
            if (params.verbose) {
                std::cout << "Warning: Image dimensions (" << width1 << "x" << height1 << 
                    ") too large for safe blur processing, skipping blur filter" << std::endl;
            }
            // Skip blur entirely for very large images
        } else {
            if (params.verbose) {
                std::cout << "Applying blur filter..." << std::endl;
                std::cout << "Blur buffer size: " << blur_buffer_size << " bytes (" << (blur_buffer_size / 1024.0 / 1024.0) << " MB)" << std::endl;
            }
            
            try {
                // Pre-allocate with zeroes for safety
                blurred1.assign(blur_buffer_size, 0);
                blurred2.assign(blur_buffer_size, 0);
        
                if (params.verbose) {
                    std::cout << "Blur buffers allocated and zeroed" << std::endl;
                    std::cout << "Applying blur to image 1..." << std::endl;
                }
                
                apply_blur_3x3(blurred1.data(), img1, width1, height1, channels1);
                
                if (params.verbose) {
                    std::cout << "Image 1 blur completed, applying blur to image 2..." << std::endl;
                }
                
                apply_blur_3x3(blurred2.data(), img2, width2, height2, channels2);
                
                if (params.verbose) {
                    std::cout << "Blur processing completed successfully" << std::endl;
                }
                proc_img1 = blurred1.data();
                proc_img2 = blurred2.data();
                
            } catch (const std::exception& e) {
                if (params.verbose) {
                    std::cerr << "Exception during blur processing: " << e.what() << std::endl;
                    std::cerr << "Falling back to non-blurred images" << std::endl;
                }
                // Fall back to non-blurred images
                proc_img1 = img1;
                proc_img2 = img2;
            } catch (...) {
                if (params.verbose) {
                    std::cerr << "Unknown exception during blur processing" << std::endl;
                    std::cerr << "Falling back to non-blurred images" << std::endl;
                }
                // Fall back to non-blurred images
                proc_img1 = img1;
                proc_img2 = img2;
            }
        }
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
        std::cout << "  Motion threshold: " << std::fixed << std::setprecision(2) << params.motion_threshold << "%" << std::endl;
        std::cout << "  Grayscale: " << (params.use_grayscale ? "Yes" : "No") << std::endl;
        std::cout << "  Blur filter: " << (params.enable_blur ? "Yes" : "No") << std::endl;
        std::cout << "  DC-only mode: " << (params.dc_only_mode ? "Yes" : "No") << std::endl;
        std::cout << "  File size check: " << (params.file_size_check ? "Yes" : "No") << std::endl;
        std::cout << "Motion detected: " << std::fixed << std::setprecision(2) << motion_percentage << "%" << std::endl;
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
        std::cout << "Processing speed: " << (processed_pixels / (motion_time.count() / 1000000.0)) / 1000000.0 << " MP/s" << std::endl;
    }
    
    // Cleanup - universal approach
    // Always try motion cleanup first, fallback to stb cleanup
    if (img1) {
        motion_stbi_image_free(img1);
    }
    if (img2) {
        motion_stbi_image_free(img2);
    }
    
    // Clean up buffers (safe to call even if not used)
    if (buffer1) motion_stbi_buffer_free(buffer1);
    if (buffer2) motion_stbi_buffer_free(buffer2);
    
    return motion_detected ? 1 : 0;
} 