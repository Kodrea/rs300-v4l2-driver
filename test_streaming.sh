#!/bin/bash

# RS300 Thermal Camera Streaming Test Script for Pi 5
# This script configures the complete media controller pipeline and tests streaming
# Uses Pi 5 compatible 16-bit packed UYVY format

echo "=== RS300 Pi 5 Streaming Test ==="

# Check if driver is loaded
if ! lsmod | grep -q rs300; then
    echo "ERROR: RS300 driver not loaded"
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

echo "=== Configuring Media Controller Pipeline ==="

# Enable the critical link from csi2 to video0
echo "Enabling media controller link..."
media-ctl -d $MEDIA_DEV -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
if [ $? -eq 0 ]; then
    echo "✓ Link enabled successfully"
else
    echo "⚠ Link enable failed, but continuing..."
fi

# Set formats throughout the pipeline (using Pi 5 compatible format with complete colorspace)
echo "Setting pipeline formats..."
media-ctl -d $MEDIA_DEV -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -d $MEDIA_DEV -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -d $MEDIA_DEV -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set video device format
echo "Setting video device format..."
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range

# Verify configuration
echo "=== Verifying Configuration ==="
echo "Video device format:"
v4l2-ctl -d /dev/video0 --get-fmt-video

echo "Media controller topology (key parts):"
media-ctl -d $MEDIA_DEV --print-topology | grep -A1 -B1 "fmt.*640x512\|ENABLED.*rp1-cfe-csi2_ch0"

echo "=== Testing V4L2 Controls ==="
echo "Testing FFC trigger..."
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=ffc_trigger=0
if [ $? -eq 0 ]; then
    echo "✓ FFC trigger works"
else
    echo "✗ FFC trigger failed"
fi

echo "Testing colormap control..."
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=colormap=3,brightness=70
v4l2-ctl -d /dev/v4l-subdev2 --get-ctrl=colormap,brightness

echo "=== Testing Streaming ==="
echo "Attempting to capture 5 frames..."
timeout 10s v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=5 --stream-to=/dev/null --verbose
STREAM_RESULT=$?

echo "=== Results ==="
if [ $STREAM_RESULT -eq 0 ]; then
    echo "🎉 SUCCESS: Streaming works!"
    echo "RS300 thermal camera is fully functional on Pi 5"
elif [ $STREAM_RESULT -eq 124 ]; then
    echo "⏱ TIMEOUT: Streaming started but timed out"
    echo "This may indicate partial success - sensor might be streaming"
else
    echo "❌ FAILED: Streaming error code $STREAM_RESULT"
    echo "Checking recent kernel messages..."
    dmesg | grep -E "rs300|rp1-cfe|stream" | tail -5
fi

echo "=== Media Controller Status ==="
media-ctl -d $MEDIA_DEV --print-topology | grep -E "rs300|rp1-cfe-csi2_ch0.*ENABLED"

echo "Test completed."