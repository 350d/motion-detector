// motion_detector_libjpeg.cpp - Motion detection using system libjpeg-turbo
// Optimized for Pi Zero with ARM-safe image loading and smart scaling

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

// Use system libjpeg-turbo instead of stb_image
#include <jpeglib.h>
#include <setjmp.h>

// PNG support using simple header-only library
#include <stdio.h>
#include <stdlib.h>

struct MotionDetectionParams {
    int pixel_threshold = 25;      
    int scale_factor = 1;          // Now used for decode-time scaling
    bool use_rgb = false;          // Use RGB instead of grayscale (slower)
    bool enable_blur = false;      
    float motion_threshold = 1.0f; 
    bool file_size_check = false;  
    float file_size_threshold = 5.0f; 
    bool verbose = false;          
    bool ultra_fast = false;       // Ultra-fast decode (lower quality)
};

// Custom JPEG error handler
struct jpeg_error_mgr_custom {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

void jpeg_error_exit_custom(j_common_ptr cinfo) {
    jpeg_error_mgr_custom* err = (jpeg_error_mgr_custom*)cinfo->err;
    longjmp(err->setjmp_buffer, 1);
}

// Load JPEG using libjpeg-turbo with scale factor applied during decode
unsigned char* load_jpeg_safe(const char* filename, int* width, int* height, int* channels, 
                              int scale_factor, bool verbose, bool ultra_fast = false) {
    if (verbose) {
        std::cout << "Loading JPEG with libjpeg-turbo: " << filename;
        if (scale_factor > 1) {
            std::cout << " (decode scale: 1/" << scale_factor << ")";
        }
        std::cout << std::endl;
    }
    
    FILE* infile = fopen(filename, "rb");
    if (!infile) {
        if (verbose) std::cerr << "Cannot open file: " << filename << std::endl;
        return nullptr;
    }
    
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr_custom jerr;
    
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit_custom;
    
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        if (verbose) std::cerr << "JPEG error during decompression" << std::endl;
        return nullptr;
    }
    
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    
    // Apply scale factor during decode - much more efficient!
    if (scale_factor > 1) {
        // libjpeg-turbo supports 1/2, 1/4, 1/8 scaling during decode
        if (scale_factor >= 8) {
            cinfo.scale_num = 1;
            cinfo.scale_denom = 8;  // 1/8 scale
            if (verbose) std::cout << "Decode scaling: 1/8 (requested -s " << scale_factor << ")" << std::endl;
        } else if (scale_factor >= 4) {
            cinfo.scale_num = 1;
            cinfo.scale_denom = 4;  // 1/4 scale
            if (verbose) std::cout << "Decode scaling: 1/4 (requested -s " << scale_factor << ")" << std::endl;
        } else if (scale_factor >= 2) {
            cinfo.scale_num = 1;
            cinfo.scale_denom = 2;  // 1/2 scale
            if (verbose) std::cout << "Decode scaling: 1/2 (requested -s " << scale_factor << ")" << std::endl;
        }
    }
    
    // Additional Pi Zero safety: force scaling for large images
    if ((cinfo.image_width > 1280 || cinfo.image_height > 720) && scale_factor == 1) {
        cinfo.scale_num = 1;
        cinfo.scale_denom = 2;  // Force 1/2 scale for large images on Pi Zero
        if (verbose) {
            std::cout << "Pi Zero safety: Auto-scaling " << cinfo.image_width << "x" << cinfo.image_height 
                      << " to 1/2 during decode" << std::endl;
        }
    }
    
    // Ultra-fast mode optimizations (like DC-only mode)
    if (ultra_fast) {
        cinfo.dct_method = JDCT_FASTEST;           // Fast IDCT (4-14% speedup)
        cinfo.do_fancy_upsampling = FALSE;        // Fast upsampling (15-20% speedup)
        cinfo.do_block_smoothing = FALSE;         // Disable smoothing for speed
        cinfo.two_pass_quantize = FALSE;          // Single-pass quantization
        if (verbose) {
            std::cout << " [ULTRA-FAST: fastest IDCT + upsampling]";
        }
    }
    
    jpeg_start_decompress(&cinfo);
    
    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *channels = cinfo.output_components;
    
    if (verbose) {
        std::cout << "JPEG loaded: " << *width << "x" << *height << " channels=" << *channels 
                  << " (memory: " << (*width * *height * *channels / 1024) << " KB)" << std::endl;
    }
    
    // Allocate memory for image
    size_t image_size = (size_t)(*width) * (*height) * (*channels);
    unsigned char* image_data = (unsigned char*)malloc(image_size);
    if (!image_data) {
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        if (verbose) std::cerr << "Cannot allocate memory for image (" << (image_size/1024) << " KB)" << std::endl;
        return nullptr;
    }
    
    // Read scanlines
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, 
                                                   cinfo.output_width * cinfo.output_components, 1);
    
    unsigned char* row_ptr = image_data;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(row_ptr, buffer[0], cinfo.output_width * cinfo.output_components);
        row_ptr += cinfo.output_width * cinfo.output_components;
    }
    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    
    return image_data;
}

// Load image using appropriate loader with scaling
unsigned char* load_image_safe(const char* filename, int* width, int* height, int* channels, 
                               int scale_factor, bool verbose, bool ultra_fast = false) {
    if (!filename || !width || !height || !channels) {
        return nullptr;
    }
    
    // Check file extension
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        if (verbose) std::cerr << "No file extension found" << std::endl;
        return nullptr;
    }
    
    // Convert to lowercase for comparison
    std::string ext_lower = ext;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);
    
    if (ext_lower == ".jpg" || ext_lower == ".jpeg") {
        return load_jpeg_safe(filename, width, height, channels, scale_factor, verbose, ultra_fast);
    } else {
        if (verbose) std::cerr << "Unsupported file format: " << ext << " (only JPEG supported)" << std::endl;
        return nullptr;
    }
}

// Simple 3x3 Gaussian blur (in-place)
void apply_blur_3x3(unsigned char* img, int width, int height, int channels) {
    if (!img || width < 3 || height < 3) return;
    
    // Gaussian 3x3 kernel (normalized)
    const float kernel[9] = {
        0.0625f, 0.125f, 0.0625f,
        0.125f,  0.25f,  0.125f,
        0.0625f, 0.125f, 0.0625f
    };
    
    // Create temporary buffer for one row
    std::vector<unsigned char> temp_row(width * channels);
    std::vector<unsigned char> temp_img(width * height * channels);
    
    // Copy original to temp buffer
    memcpy(temp_img.data(), img, width * height * channels);
    
    // Apply blur (skip borders for simplicity)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            for (int c = 0; c < channels; c++) {
                float sum = 0.0f;
                int kernel_idx = 0;
                
                // Apply 3x3 kernel
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int src_idx = ((y + ky) * width + (x + kx)) * channels + c;
                        sum += temp_img[src_idx] * kernel[kernel_idx++];
                    }
                }
                
                int dst_idx = (y * width + x) * channels + c;
                img[dst_idx] = (unsigned char)std::max(0.0f, std::min(255.0f, sum));
            }
        }
    }
}

// Calculate motion on already-scaled images (no pixel skipping needed!)
float calculate_motion_scaled(const unsigned char* img1, const unsigned char* img2,
                              int width, int height, int channels,
                              const MotionDetectionParams& params) {
    if (!img1 || !img2 || width <= 0 || height <= 0 || channels <= 0) {
        return 0.0f;
    }
    
    // Apply blur if enabled (creates copies for processing)
    unsigned char* processed_img1 = nullptr;
    unsigned char* processed_img2 = nullptr;
    
    if (params.enable_blur) {
        size_t img_size = width * height * channels;
        processed_img1 = (unsigned char*)malloc(img_size);
        processed_img2 = (unsigned char*)malloc(img_size);
        
        if (processed_img1 && processed_img2) {
            memcpy(processed_img1, img1, img_size);
            memcpy(processed_img2, img2, img_size);
            apply_blur_3x3(processed_img1, width, height, channels);
            apply_blur_3x3(processed_img2, width, height, channels);
            img1 = processed_img1;
            img2 = processed_img2;
        }
    }
    
    int total_pixels = width * height;
    int motion_pixels = 0;
    
    // Process all pixels (no skipping needed since we scaled during decode!)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * channels;
            
            if (!params.use_rgb && channels >= 3) {
                // Convert to grayscale and compare
                int gray1 = (img1[idx] + img1[idx+1] + img1[idx+2]) / 3;
                int gray2 = (img2[idx] + img2[idx+1] + img2[idx+2]) / 3;
                
                if (abs(gray1 - gray2) > params.pixel_threshold) {
                    motion_pixels++;
                }
            } else {
                // Compare all channels
                bool pixel_changed = false;
                for (int c = 0; c < channels; c++) {
                    if (abs((int)img1[idx + c] - (int)img2[idx + c]) > params.pixel_threshold) {
                        pixel_changed = true;
                        break;
                    }
                }
                if (pixel_changed) {
                    motion_pixels++;
                }
            }
        }
    }
    
    // Clean up blur buffers if used
    if (processed_img1) free(processed_img1);
    if (processed_img2) free(processed_img2);
    
    return total_pixels > 0 ? (float)motion_pixels / total_pixels * 100.0f : 0.0f;
}

// File size comparison
float compare_file_sizes(const char* file1, const char* file2, const MotionDetectionParams& params) {
    struct stat stat1, stat2;
    
    if (stat(file1, &stat1) != 0 || stat(file2, &stat2) != 0) {
        return -1.0f; // Error
    }
    
    if (stat1.st_size == 0 || stat2.st_size == 0) {
        return -1.0f; // Invalid file
    }
    
    float size_diff = abs((long)(stat1.st_size - stat2.st_size));
    float avg_size = (stat1.st_size + stat2.st_size) / 2.0f;
    float percentage = (size_diff / avg_size) * 100.0f;
    
    if (params.verbose) {
        std::cout << "File sizes: " << stat1.st_size << " vs " << stat2.st_size 
                  << " (diff: " << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
    }
    
    return percentage;
}

void print_usage(const char* program_name) {
    std::cout << "Motion Detector (libjpeg-turbo version) - Pi Zero optimized" << std::endl;
    std::cout << "Usage: " << program_name << " [options] <image1> <image2>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -t <threshold>   Pixel difference threshold (0-255, default: 25)" << std::endl;
    std::cout << "  -s <scale>       Decode scale factor (1=full, 2=half, 4=quarter, 8=eighth, default: 1)" << std::endl;
    std::cout << "                   JPEG: scaled during decode (very efficient!)" << std::endl;
    std::cout << "  -m <motion>      Motion threshold percentage (default: 1.0)" << std::endl;
    std::cout << "  -rgb             Use RGB mode (slower than grayscale)" << std::endl;
    std::cout << "  -u               Ultra-fast mode (fastest IDCT + upsampling, lower quality)" << std::endl;
    std::cout << "  -b               Apply 3x3 Gaussian blur for noise reduction" << std::endl;
    std::cout << "  -v               Verbose output (includes timing breakdown)" << std::endl;
    std::cout << "  -f               File size check mode (fast pre-check)" << std::endl;
    std::cout << "  --help           Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported formats: JPEG (with hardware decode scaling)" << std::endl;
    std::cout << "Pi Zero tip: Use -s 2 or -s 4 for large images to save memory!" << std::endl;
}

int main(int argc, char* argv[]) {
    MotionDetectionParams params;
    
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    int img_arg_start = 1;
    for (int i = 1; i < argc - 2; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            params.pixel_threshold = std::atoi(argv[++i]);
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            params.scale_factor = std::atoi(argv[++i]);
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            params.motion_threshold = std::atof(argv[++i]);
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-rgb") == 0) {
            params.use_rgb = true;
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-u") == 0) {
            params.ultra_fast = true;
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-b") == 0) {
            params.enable_blur = true;
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            params.verbose = true;
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "-f") == 0) {
            params.file_size_check = true;
            img_arg_start = i + 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    const char* image1_path = argv[argc - 2];
    const char* image2_path = argv[argc - 1];
    
    if (params.verbose) {
        std::cout << "Motion Detector (libjpeg-turbo) starting..." << std::endl;
        std::cout << "Scale factor: " << params.scale_factor << " (decode-time scaling)" << std::endl;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Quick file size check if requested
    if (params.file_size_check) {
        float size_diff = compare_file_sizes(image1_path, image2_path, params);
        if (size_diff >= 0 && size_diff < params.file_size_threshold) {
            std::cout << "No motion detected (file size difference: " 
                      << std::fixed << std::setprecision(1) << size_diff << "% < " 
                      << params.file_size_threshold << "%)" << std::endl;
            return 1; // No motion
        }
        if (params.verbose && size_diff >= params.file_size_threshold) {
            std::cout << "File size difference detected (" << size_diff 
                      << "%), proceeding with image analysis..." << std::endl;
        }
    }
    
    // Load images with scaling
    int width1, height1, channels1;
    int width2, height2, channels2;
    
    auto load_start = std::chrono::high_resolution_clock::now();
    
    unsigned char* img1 = load_image_safe(image1_path, &width1, &height1, &channels1, 
                                          params.scale_factor, params.verbose, params.ultra_fast);
    if (!img1) {
        std::cerr << "Failed to load image: " << image1_path << std::endl;
        return 1;
    }
    
    unsigned char* img2 = load_image_safe(image2_path, &width2, &height2, &channels2, 
                                          params.scale_factor, params.verbose, params.ultra_fast);
    if (!img2) {
        std::cerr << "Failed to load image: " << image2_path << std::endl;
        free(img1);
        return 1;
    }
    
    auto load_end = std::chrono::high_resolution_clock::now();
    
    // Check dimensions match
    if (width1 != width2 || height1 != height2 || channels1 != channels2) {
        std::cerr << "Image dimensions don't match after scaling!" << std::endl;
        std::cerr << "Image 1: " << width1 << "x" << height1 << " (channels: " << channels1 << ")" << std::endl;
        std::cerr << "Image 2: " << width2 << "x" << height2 << " (channels: " << channels2 << ")" << std::endl;
        free(img1);
        free(img2);
        return 1;
    }
    
    // Calculate motion
    auto motion_start = std::chrono::high_resolution_clock::now();
    float motion_percentage = calculate_motion_scaled(img1, img2, width1, height1, channels1, params);
    auto motion_end = std::chrono::high_resolution_clock::now();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Output results
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Motion detected: " << motion_percentage << "%" << std::endl;
    
    if (motion_percentage >= params.motion_threshold) {
        std::cout << "MOTION DETECTED (threshold: " << params.motion_threshold << "%)" << std::endl;
    } else {
        std::cout << "No significant motion (threshold: " << params.motion_threshold << "%)" << std::endl;
    }
    
    if (params.verbose) {
        auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        auto load_duration = std::chrono::duration_cast<std::chrono::microseconds>(load_end - load_start);
        auto motion_duration = std::chrono::duration_cast<std::chrono::microseconds>(motion_end - motion_start);
        
        std::cout << "Timing breakdown:" << std::endl;
        std::cout << "  Image loading: " << (load_duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "  Motion calc:   " << (motion_duration.count() / 1000.0) << " ms" << std::endl;
        std::cout << "  Total time:    " << (total_duration.count() / 1000.0) << " ms" << std::endl;
    }
    
    if (params.verbose) {
        std::cout << "Final image size: " << width1 << "x" << height1 << " (" << channels1 << " channels)" << std::endl;
        std::cout << "Memory usage: " << ((width1 * height1 * channels1 * 2) / 1024) << " KB" << std::endl;
        std::cout << "Pixel threshold: " << params.pixel_threshold << std::endl;
        std::cout << "RGB mode: " << (params.use_rgb ? "enabled" : "disabled (grayscale)") << std::endl;
        std::cout << "Ultra-fast mode: " << (params.ultra_fast ? "enabled (fastest IDCT + upsampling)" : "disabled") << std::endl;
    }
    
    // Cleanup
    free(img1);
    free(img2);
    
    return motion_percentage >= params.motion_threshold ? 0 : 1;
} 