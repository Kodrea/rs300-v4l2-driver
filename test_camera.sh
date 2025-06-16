#!/bin/bash

# RS300 Camera Testing Script
# Interactive test script for different module types

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

echo "=== RS300 Camera Testing ===" 
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
echo "1) 256x192 module"
echo "2) 384x288 module" 
echo "3) 640x512 module (default)"
echo ""
read -p "Select module type (1-3, default: 3): " -n 1 -r
echo

case $REPLY in
    1)
        WIDTH=256
        HEIGHT=192
        print_success "Selected 256x192 module"
        ;;
    2)
        WIDTH=384
        HEIGHT=288
        print_success "Selected 384x288 module"
        ;;
    3|"")
        WIDTH=640
        HEIGHT=512
        print_success "Selected 640x512 module (default)"
        ;;
    *)
        print_warning "Invalid selection, using 640x512 (default)"
        WIDTH=640
        HEIGHT=512
        ;;
esac

echo ""
echo "=== Stream Test Configuration ==="
echo "Resolution: ${WIDTH}x${HEIGHT}"
echo "Device: /dev/video0"
echo ""

# Test different streaming options
echo "Choose streaming test:"
echo "1) Quick test (5 frames)"
echo "2) Standard test (30 frames)" 
echo "3) Long test (100 frames)"
echo "4) Continuous streaming (Ctrl+C to stop)"
echo ""
read -p "Select test type (1-4, default: 2): " -n 1 -r
echo

case $REPLY in
    1)
        FRAME_COUNT=5
        TEST_TYPE="Quick test"
        ;;
    2|"")
        FRAME_COUNT=30
        TEST_TYPE="Standard test"
        ;;
    3)
        FRAME_COUNT=100
        TEST_TYPE="Long test"
        ;;
    4)
        FRAME_COUNT=0
        TEST_TYPE="Continuous streaming"
        ;;
    *)
        print_warning "Invalid selection, using standard test (30 frames)"
        FRAME_COUNT=30
        TEST_TYPE="Standard test"
        ;;
esac

echo ""
print_step "Starting $TEST_TYPE"
echo "Resolution: ${WIDTH}x${HEIGHT}"
echo "Format: YUYV"

if [ $FRAME_COUNT -eq 0 ]; then
    echo "Press Ctrl+C to stop streaming..."
    echo ""
    print_step "Starting continuous stream"
    v4l2-ctl -d /dev/video0 --stream-mmap --stream-to=/dev/null
else
    echo "Capturing $FRAME_COUNT frames..."
    echo ""
    print_step "Starting stream test"
    
    # Run the stream test with timing
    START_TIME=$(date +%s.%N)
    if v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=$FRAME_COUNT --stream-to=/dev/null; then
        END_TIME=$(date +%s.%N)
        DURATION=$(echo "$END_TIME - $START_TIME" | bc 2>/dev/null || echo "unknown")
        
        print_success "Stream test completed successfully!"
        echo "Frames captured: $FRAME_COUNT"
        if [ "$DURATION" != "unknown" ]; then
            FPS=$(echo "scale=2; $FRAME_COUNT / $DURATION" | bc 2>/dev/null || echo "unknown")
            echo "Duration: ${DURATION}s"
            echo "Average FPS: $FPS"
        fi
    else
        print_error "Stream test failed"
        echo ""
        echo "Troubleshooting suggestions:"
        echo "1. Run ./configure_media.sh to set up the pipeline"
        echo "2. Check dmesg for error messages: dmesg | grep rs300"
        echo "3. Verify driver is loaded: lsmod | grep rs300"
        exit 1
    fi
fi

echo ""
print_success "Camera test completed!"