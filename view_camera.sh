#!/bin/bash

# RS300 Camera Live Viewer Script
# Interactive live video viewer for different module types

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_step() {
    echo -e "${BLUE}▶${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

echo "=== RS300 Camera Live Viewer ===" 
echo ""

# Check prerequisites
print_step "Checking prerequisites"

if ! lsmod | grep -q rs300; then
    print_error "RS300 driver not loaded"
    echo "Run: ./setup.sh && sudo reboot"
    exit 1
fi
print_success "RS300 driver loaded"

if ! command -v v4l2-ctl >/dev/null 2>&1; then
    print_error "v4l2-ctl not found"
    echo "Install: sudo apt install v4l-utils"
    exit 1
fi
print_success "v4l2-ctl available"

# Check for GStreamer
if ! command -v gst-launch-1.0 >/dev/null 2>&1; then
    print_error "GStreamer not found"
    echo "Install: sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good"
    exit 1
fi
print_success "GStreamer available for live viewing"

# Check if video device exists
if [ ! -e "/dev/video0" ]; then
    print_error "/dev/video0 not available"
    echo "Run ./configure_media.sh first to set up the pipeline"
    exit 1
fi
print_success "/dev/video0 available"

echo ""
echo "=== Module Type Selection ==="
echo "Which RS300 module are you using?"
echo "0) 640x512 module (mode 0)"
echo "1) 256x192 module (mode 1)"
echo "2) 384x288 module (mode 2)"
echo ""
read -p "Select module type (0-2): " -n 1 -r
echo

case $REPLY in
    0)
        WIDTH=640
        HEIGHT=512
        print_success "Selected 640x512 module (mode 0)"
        ;;
    1)
        WIDTH=256
        HEIGHT=192
        print_success "Selected 256x192 module (mode 1)"
        ;;
    2)
        WIDTH=384
        HEIGHT=288
        print_success "Selected 384x288 module (mode 2)"
        ;;
    *)
        print_error "Invalid selection"
        exit 1
        ;;
esac

echo ""
echo "=== Live Viewer Configuration ==="
echo "Resolution: ${WIDTH}x${HEIGHT}"
echo "Device: /dev/video0"
echo "Viewer: GStreamer"
echo ""

# Choose display options
echo "Display options:"
echo "1) Normal window"
echo "2) Fullscreen"
echo "3) Scaled 2x window"
echo ""
read -p "Select display mode (1-3, default: 1): " -n 1 -r
echo

case $REPLY in
    1|"")
        DISPLAY_MODE="Normal window"
        SCALE_FACTOR="1"
        FULLSCREEN="false"
        ;;
    2)
        DISPLAY_MODE="Fullscreen"
        SCALE_FACTOR="1"
        FULLSCREEN="true"
        ;;
    3)
        DISPLAY_MODE="Scaled 2x window"
        SCALE_FACTOR="2"
        FULLSCREEN="false"
        ;;
    *)
        print_warning "Invalid selection, using normal window"
        DISPLAY_MODE="Normal window"
        SCALE_FACTOR="1"
        FULLSCREEN="false"
        ;;
esac

echo ""
print_step "Starting live viewer"
echo "Display mode: $DISPLAY_MODE"
echo "Format: YUYV"
echo "Press 'q' or close window to exit"
echo ""

# Build GStreamer pipeline
print_step "Launching GStreamer live viewer"

# Calculate scaled dimensions
SCALED_WIDTH=$((WIDTH * SCALE_FACTOR))
SCALED_HEIGHT=$((HEIGHT * SCALE_FACTOR))

# Build GStreamer pipeline
GST_PIPELINE="v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=${WIDTH},height=${HEIGHT} ! videoconvert"

# Add scaling if needed
if [ "$SCALE_FACTOR" != "1" ]; then
    GST_PIPELINE="$GST_PIPELINE ! videoscale ! video/x-raw,width=${SCALED_WIDTH},height=${SCALED_HEIGHT}"
fi

# Add display sink
if [ "$FULLSCREEN" = "true" ]; then
    GST_PIPELINE="$GST_PIPELINE ! autovideosink fullscreen=true"
else
    GST_PIPELINE="$GST_PIPELINE ! autovideosink"
fi

echo "GStreamer pipeline: $GST_PIPELINE"
echo ""

# Launch GStreamer
gst-launch-1.0 $GST_PIPELINE

VIEWER_RESULT=$?

echo ""
if [ $VIEWER_RESULT -eq 0 ]; then
    print_success "GStreamer live viewer session completed successfully!"
else
    print_error "GStreamer live viewer failed (exit code: $VIEWER_RESULT)"
    echo ""
    echo "Troubleshooting suggestions:"
    echo "1. Install GStreamer: sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good"
    echo "2. Run ./configure_media.sh to set up the pipeline"
    echo "3. Test basic streaming first: ./test_camera.sh"
    echo "4. Check dmesg for error messages: dmesg | grep rs300"
    echo "5. Verify driver is loaded: lsmod | grep rs300"
    echo "6. Check video device format: v4l2-ctl -d /dev/video0 --get-fmt-video"
    exit 1
fi

echo ""
print_success "Camera live viewer completed!"