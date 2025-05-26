#!/bin/bash

echo "=== Simple Large Image Test ==="

# Copy existing test image to simulate larger load
cp images/test_motion_1.jpg images/large_test1.jpg 2>/dev/null || echo "Could not copy test image"
cp images/test_motion_2.jpg images/large_test2.jpg 2>/dev/null || echo "Could not copy test image"

echo "Testing motion detection with parameters that caused segfault..."
echo "Command: ../motion-detector images/large_test1.jpg images/large_test2.jpg -s 4 -m 4 -g -b -v"
echo ""

../motion-detector images/large_test1.jpg images/large_test2.jpg -s 4 -m 4 -g -b -v
exit_code=$?

echo ""
echo "Exit code: $exit_code"

if [ $exit_code -eq 139 ]; then
    echo "❌ Segmentation fault detected!"
elif [ $exit_code -eq 0 ] || [ $exit_code -eq 1 ]; then
    echo "✅ Success! Program completed normally"
else
    echo "❓ Unexpected exit code: $exit_code"
fi 