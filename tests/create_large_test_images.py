#!/usr/bin/env python3

import os
import sys

try:
    from PIL import Image, ImageDraw
    print("Using PIL to create large test images...")
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False
    print("PIL not available, trying alternative methods...")

def create_large_images_pil():
    """Create large images using PIL"""
    width, height = 1920, 1080
    
    # Create first image - solid color with some shapes
    img1 = Image.new('RGB', (width, height), color='black')
    draw1 = ImageDraw.Draw(img1)
    # Add some rectangles for motion detection
    draw1.rectangle([100, 100, 300, 300], fill='white')
    draw1.rectangle([500, 400, 700, 600], fill='gray')
    img1.save('images/real_large_1.jpg', 'JPEG', quality=85)
    
    # Create second image - similar but slightly different
    img2 = Image.new('RGB', (width, height), color='black')
    draw2 = ImageDraw.Draw(img2)
    # Slightly different rectangles to create motion
    draw2.rectangle([150, 150, 350, 350], fill='white')
    draw2.rectangle([550, 450, 750, 650], fill='gray')
    img2.save('images/real_large_2.jpg', 'JPEG', quality=85)
    
    print(f"✓ Created real large images: {width}x{height}")
    return True

def create_large_images_manual():
    """Create large images by resizing existing ones if PIL not available"""
    import subprocess
    
    # Try using sips (macOS built-in image tool)
    try:
        # Check if we have existing test images
        test_img = 'images/test_motion_1.jpg'
        if os.path.exists(test_img):
            # Use sips to resize
            subprocess.run(['sips', '-z', '1080', '1920', test_img, '--out', 'images/real_large_1.jpg'], 
                          check=True, capture_output=True)
            subprocess.run(['sips', '-z', '1080', '1920', 'images/test_motion_2.jpg', '--out', 'images/real_large_2.jpg'], 
                          check=True, capture_output=True)
            print("✓ Created large images using sips")
            return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    
    print("❌ Could not create large images")
    return False

def main():
    # Ensure output directory exists
    os.makedirs('images', exist_ok=True)
    
    print("Creating large test images (1920x1080) for segfault testing...")
    
    if PIL_AVAILABLE:
        success = create_large_images_pil()
    else:
        success = create_large_images_manual()
    
    if success:
        # Check file sizes
        for img in ['images/real_large_1.jpg', 'images/real_large_2.jpg']:
            if os.path.exists(img):
                size = os.path.getsize(img)
                print(f"  {img}: {size} bytes ({size/1024/1024:.2f} MB)")
    else:
        print("Failed to create large test images")
        sys.exit(1)

if __name__ == "__main__":
    main() 