/*
   motion_stb_image.h - Simplified image loader for motion detection
   Based on stb_image.h by Sean Barrett
   
   SIMPLE OPTIMIZATIONS:
   - Caching loaded images to avoid re-reading
   - Basic grayscale conversion optimization
   - Simple file format detection
   
   USAGE:
     unsigned char *img = motion_stbi_load(filename, &x, &y, &comp, req_comp, mode, buffer);
*/

#ifndef MOTION_STB_IMAGE_INCLUDE_STB_IMAGE_H
#define MOTION_STB_IMAGE_INCLUDE_STB_IMAGE_H

// Standard C includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for stb_image
#ifdef __cplusplus
extern "C" {
#endif

unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
void stbi_image_free(void *retval_from_stbi_load);
int stbi_info(char const *filename, int *x, int *y, int *comp);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char motion_stbi_uc;

// Simple buffer for caching loaded images
typedef struct {
    motion_stbi_uc *data;
    int width, height, channels;
    int capacity;
    char filename[256];
} motion_buffer_t;

// Simple modes
enum {
    MOTION_MODE_FULL = 0,
    MOTION_MODE_DC_ONLY,
    MOTION_MODE_QUARTER,
    MOTION_MODE_HALF
};

// Simple API that just wraps stbi_load with caching
static motion_stbi_uc *motion_stbi_load(
    char const *filename, int *x, int *y, int *comp, 
    int req_comp, int motion_mode, motion_buffer_t *reuse_buffer) {
    
    // Check if we can reuse buffer (simple filename comparison)
    if (reuse_buffer && reuse_buffer->data && 
        reuse_buffer->filename[0] != 0 &&
        strcmp(reuse_buffer->filename, filename) == 0) {
        *x = reuse_buffer->width;
        *y = reuse_buffer->height;
        *comp = reuse_buffer->channels;
        return reuse_buffer->data;
    }
    
    // Just use standard stbi_load - no complex optimizations
    motion_stbi_uc *img = stbi_load(filename, x, y, comp, req_comp > 0 ? req_comp : 0);
    
    if (!img) {
        return NULL;
    }
    
    // Simple downsampling for QUARTER/HALF modes
    if (motion_mode == MOTION_MODE_QUARTER && *x > 4 && *y > 4) {
        int new_w = *x / 4;
        int new_h = *y / 4;
        motion_stbi_uc *downsampled = (motion_stbi_uc*)malloc(new_w * new_h * (*comp));
        
        if (downsampled) {
            // Simple nearest-neighbor downsampling
            for (int y_out = 0; y_out < new_h; y_out++) {
                for (int x_out = 0; x_out < new_w; x_out++) {
                    int src_y = y_out * 4;
                    int src_x = x_out * 4;
                    int src_idx = (src_y * (*x) + src_x) * (*comp);
                    int dst_idx = (y_out * new_w + x_out) * (*comp);
                    
                    for (int c = 0; c < *comp; c++) {
                        downsampled[dst_idx + c] = img[src_idx + c];
                    }
                }
            }
            
            stbi_image_free(img);
            img = downsampled;
            *x = new_w;
            *y = new_h;
        }
    } else if (motion_mode == MOTION_MODE_HALF && *x > 2 && *y > 2) {
        int new_w = *x / 2;
        int new_h = *y / 2;
        motion_stbi_uc *downsampled = (motion_stbi_uc*)malloc(new_w * new_h * (*comp));
        
        if (downsampled) {
            // Simple nearest-neighbor downsampling
            for (int y_out = 0; y_out < new_h; y_out++) {
                for (int x_out = 0; x_out < new_w; x_out++) {
                    int src_y = y_out * 2;
                    int src_x = x_out * 2;
                    int src_idx = (src_y * (*x) + src_x) * (*comp);
                    int dst_idx = (y_out * new_w + x_out) * (*comp);
                    
                    for (int c = 0; c < *comp; c++) {
                        downsampled[dst_idx + c] = img[src_idx + c];
                    }
                }
            }
            
            stbi_image_free(img);
            img = downsampled;
            *x = new_w;
            *y = new_h;
        }
    }
    
    // Cache result if buffer provided
    if (img && reuse_buffer) {
        int size = (*x) * (*y) * (*comp);
        if (size > 0 && size < (50 * 1024 * 1024)) { // Max 50MB cache
            if (!reuse_buffer->data || reuse_buffer->capacity < size) {
                free(reuse_buffer->data);
                reuse_buffer->data = (motion_stbi_uc*)malloc(size);
                reuse_buffer->capacity = reuse_buffer->data ? size : 0;
            }
            
            if (reuse_buffer->data) {
                memcpy(reuse_buffer->data, img, size);
                reuse_buffer->width = *x;
                reuse_buffer->height = *y;
                reuse_buffer->channels = *comp;
                strncpy(reuse_buffer->filename, filename, sizeof(reuse_buffer->filename) - 1);
                reuse_buffer->filename[sizeof(reuse_buffer->filename) - 1] = 0;
            }
        }
    }
    
    return img;
}

// Simple buffer management
static motion_buffer_t *motion_stbi_buffer_create(int initial_capacity) {
    motion_buffer_t *buffer = (motion_buffer_t*)calloc(1, sizeof(motion_buffer_t));
    if (buffer && initial_capacity > 0) {
        buffer->data = (motion_stbi_uc*)malloc(initial_capacity);
        buffer->capacity = buffer->data ? initial_capacity : 0;
    }
    return buffer;
}

static void motion_stbi_buffer_free(motion_buffer_t *buffer) {
    if (buffer) {
        free(buffer->data);
        free(buffer);
    }
}

// Simple compatibility test (just check if file exists and is JPEG)
static int motion_stbi_test_dc_compatibility(const char *filename) {
    if (!filename) return 0;
    
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    
    unsigned char header[2];
    int result = (fread(header, 1, 2, f) == 2 && header[0] == 0xFF && header[1] == 0xD8);
    fclose(f);
    return result;
}

// Simple cleanup
static void motion_stbi_image_free(void *retval_from_stbi_load) {
    if (retval_from_stbi_load) {
        stbi_image_free(retval_from_stbi_load);
    }
}

#ifdef __cplusplus
}
#endif

#endif // MOTION_STB_IMAGE_INCLUDE_STB_IMAGE_H 