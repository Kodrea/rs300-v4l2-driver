#!/bin/bash

# RS300 Format Testing Script
# Systematically tests different mediabus formats to resolve pipeline issues

echo "=== RS300 Format Testing ==="

# Check if driver is loaded
if ! lsmod | grep -q rs300; then
    echo "ERROR: RS300 driver not loaded. Run ./setup.sh first"
    exit 1
fi

# Find the correct media controller device
MEDIA_DEV=""
for i in {0..3}; do
    if media-ctl -d /dev/media$i --print-topology 2>/dev/null | grep -q "rs300 10-003c"; then
        MEDIA_DEV="/dev/media$i"
        echo "Found RS300 on $MEDIA_DEV"
        break
    fi
done

if [ -z "$MEDIA_DEV" ]; then
    echo "ERROR: Could not find RS300 media controller device"
    exit 1
fi

# Format combinations to test (Pi 5 compatible 16-bit packed formats only)
declare -a FORMATS=(
    "UYVY8_1X16"    # Primary format (recommended)
    "YUYV8_1X16"    # Alternative YUYV format
    "YVYU8_1X16"    # Alternative YUV order
    "VYUY8_1X16"    # Alternative YUV order
    "Y10_1X10"      # 10-bit grayscale (thermal)
    "Y16_1X16"      # 16-bit grayscale (thermal)
    "Y12_1X12"      # 12-bit grayscale
)

# Corresponding V4L2 pixelformats for video device
declare -a PIXELFORMATS=(
    "UYVY"          # YUV 4:2:2 (recommended)
    "YUYV"          # YUV 4:2:2
    "YVYU"          # YUV 4:2:2 alternative
    "VYUY"          # YUV 4:2:2 alternative
    "Y10 "          # 10-bit Y
    "Y16 "          # 16-bit Y
    "Y12 "          # 12-bit Y
)

SUCCESS_COUNT=0
TOTAL_TESTS=${#FORMATS[@]}

echo "Testing $TOTAL_TESTS Pi 5 compatible format combinations..."
echo "NOTE: Only testing 16-bit packed formats (*8_1X16) - 8-bit dual lane formats (*8_2X8) are not supported on Pi 5"
echo "==============================================="

for i in "${!FORMATS[@]}"; do
    FORMAT="${FORMATS[$i]}"
    PIXELFORMAT="${PIXELFORMATS[$i]}"
    
    echo ""
    echo "Test $((i+1))/$TOTAL_TESTS: Testing format $FORMAT -> $PIXELFORMAT"
    echo "-------------------------------------------"
    
    # Reset any previous configuration
    echo "Resetting media links..."
    media-ctl -d $MEDIA_DEV -r >/dev/null 2>&1
    
    # Enable the critical link
    echo "Enabling media link..."
    if media-ctl -d $MEDIA_DEV -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]" 2>/dev/null; then
        echo "✓ Link enabled"
    else
        echo "✗ Link enable failed"
        continue
    fi
    
    # Set formats throughout pipeline
    echo "Setting pipeline format to $FORMAT..."
    
    # Set RS300 output format
    if media-ctl -d $MEDIA_DEV -V "'rs300 10-003c':0 [fmt:$FORMAT/640x512]" 2>/dev/null; then
        echo "  ✓ RS300 format set"
    else
        echo "  ✗ RS300 format failed"
        continue
    fi
    
    # Set CSI2 input format
    if media-ctl -d $MEDIA_DEV -V "'csi2':0 [fmt:$FORMAT/640x512]" 2>/dev/null; then
        echo "  ✓ CSI2 input format set"
    else
        echo "  ✗ CSI2 input format failed"
        continue
    fi
    
    # Set CSI2 output format  
    if media-ctl -d $MEDIA_DEV -V "'csi2':4 [fmt:$FORMAT/640x512]" 2>/dev/null; then
        echo "  ✓ CSI2 output format set"
    else
        echo "  ✗ CSI2 output format failed"
        continue
    fi
    
    # Set video device format
    echo "Setting video device format to $PIXELFORMAT..."
    if v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=$PIXELFORMAT 2>/dev/null; then
        echo "  ✓ Video device format set"
    else
        echo "  ✗ Video device format failed"
        continue
    fi
    
    # Verify format negotiation
    echo "Verifying format negotiation..."
    RS300_FMT=$(media-ctl -d $MEDIA_DEV --get-v4l2 "'rs300 10-003c':0" 2>/dev/null)
    CSI2_IN_FMT=$(media-ctl -d $MEDIA_DEV --get-v4l2 "'csi2':0" 2>/dev/null)
    CSI2_OUT_FMT=$(media-ctl -d $MEDIA_DEV --get-v4l2 "'csi2':4" 2>/dev/null)
    VIDEO_FMT=$(v4l2-ctl -d /dev/video0 --get-fmt-video 2>/dev/null)
    
    echo "  RS300:     $RS300_FMT"
    echo "  CSI2 IN:   $CSI2_IN_FMT"
    echo "  CSI2 OUT:  $CSI2_OUT_FMT"
    echo "  VIDEO:     $(echo $VIDEO_FMT | grep -o 'Width/Height.*')"
    
    # Test streaming capability
    echo "Testing streaming capability..."
    
    # Start kernel message monitoring in background
    dmesg -C  # Clear previous messages
    
    # Attempt streaming
    timeout 5s v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=/dev/null --verbose >/dev/null 2>&1
    STREAM_RESULT=$?
    
    # Check for kernel errors
    KERNEL_ERRORS=$(dmesg | grep -E "(error|Error|ERROR|failed|Failed|FAILED|invalid|Invalid|INVALID)" | wc -l)
    
    if [ $STREAM_RESULT -eq 0 ]; then
        echo "  🎉 SUCCESS: Streaming works with $FORMAT!"
        echo "  ✓ No kernel errors detected"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        
        # Save working configuration
        echo "# Working configuration found:" > "working_format_$FORMAT.conf"
        echo "MEDIABUS_FORMAT=$FORMAT" >> "working_format_$FORMAT.conf"
        echo "PIXEL_FORMAT=$PIXELFORMAT" >> "working_format_$FORMAT.conf"
        echo "DATE=$(date)" >> "working_format_$FORMAT.conf"
        echo "Configuration saved to working_format_$FORMAT.conf"
        
    elif [ $STREAM_RESULT -eq 124 ]; then
        echo "  ⏱ TIMEOUT: Streaming may be working but slow"
        if [ $KERNEL_ERRORS -eq 0 ]; then
            echo "  ✓ No kernel errors - format may be compatible"
        else
            echo "  ⚠ $KERNEL_ERRORS kernel errors detected"
        fi
    else
        echo "  ❌ FAILED: Streaming error (code $STREAM_RESULT)"
        if [ $KERNEL_ERRORS -gt 0 ]; then
            echo "  ⚠ $KERNEL_ERRORS kernel errors detected:"
            dmesg | grep -E "(error|Error|ERROR|failed|Failed|FAILED|invalid|Invalid|INVALID)" | tail -3 | sed 's/^/    /'
        fi
    fi
    
    sleep 1  # Brief pause between tests
done

echo ""
echo "==============================================="
echo "Format Testing Complete"
echo "==============================================="
echo "Total tests: $TOTAL_TESTS"
echo "Successful: $SUCCESS_COUNT"
echo "Failed: $((TOTAL_TESTS - SUCCESS_COUNT))"

if [ $SUCCESS_COUNT -gt 0 ]; then
    echo ""
    echo "🎉 Found $SUCCESS_COUNT working format(s)!"
    echo "Check working_format_*.conf files for details"
    echo ""
    echo "Recommended next steps:"
    echo "1. Use the working format in your driver"
    echo "2. Test thermal camera functionality with working format"
    echo "3. Optimize performance parameters"
else
    echo ""
    echo "❌ No working formats found"
    echo ""
    echo "Debugging suggestions:"
    echo "1. Check 'dmesg | grep rs300' for driver errors"
    echo "2. Verify I2C communication: i2cdetect -y 10"
    echo "3. Check media controller topology: media-ctl -d $MEDIA_DEV -p"
    echo "4. Review driver format support in rs300.c"
    echo "5. Ensure RS300 driver outputs Pi 5 compatible 16-bit packed format"
fi

echo ""
echo "For detailed analysis, check kernel messages:"
echo "dmesg | grep -E '(rs300|rp1-cfe|format)'"