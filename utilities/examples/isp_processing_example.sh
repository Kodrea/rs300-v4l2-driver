#!/bin/bash
#
# RS300 Thermal Camera - PiSP Backend Processing Example
# FIXED VERSION - No hanging!
#

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "RS300 + PiSP Backend Processing Demo"
echo "========================================"
echo ""

# Check prerequisites
echo -e "${YELLOW}Checking prerequisites...${NC}"

# Check RS300 device
if [ ! -e /dev/video0 ]; then
    echo -e "${RED}Error: /dev/video0 not found. Is RS300 driver loaded?${NC}"
    exit 1
fi

# Check pispbe devices
if [ ! -e /dev/video20 ]; then
    echo -e "${RED}Error: /dev/video20 not found. Is pispbe driver loaded?${NC}"
    echo "Try: lsmod | grep pisp"
    exit 1
fi

# Check GStreamer
if ! command -v gst-launch-1.0 &> /dev/null; then
    echo -e "${RED}Error: GStreamer not found. Install with:${NC}"
    echo "  sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad"
    exit 1
fi

echo -e "${GREEN}✓ All prerequisites met${NC}"
echo ""

# SIMPLIFIED: Just always configure the pipeline
# This avoids the camera hardware error that causes deadlock
echo -e "${YELLOW}Configuring media pipeline...${NC}"

CONFIG_SCRIPT="../configure_media.sh"
[ ! -f "$CONFIG_SCRIPT" ] && CONFIG_SCRIPT="./configure_media.sh"

if [ ! -f "$CONFIG_SCRIPT" ]; then
    echo -e "${RED}Error: configure_media.sh not found${NC}"
    exit 1
fi

if yes y | "$CONFIG_SCRIPT" --non-interactive >/dev/null 2>&1; then
    echo -e "${GREEN}✓ Media pipeline configured${NC}"
else
    echo -e "${RED}✗ Configuration failed${NC}"
    echo "Try running manually: ./configure_media.sh"
    exit 1
fi

echo ""

# Check current RS300 configuration
echo -e "${YELLOW}Current RS300 configuration:${NC}"
v4l2-ctl -d /dev/video0 --get-fmt-video
echo ""

# Show PiSP Backend devices
echo -e "${YELLOW}PiSP Backend devices:${NC}"
v4l2-ctl --list-devices | grep -A 20 "pispbe"
echo ""

# Show pispbe supported formats
echo -e "${YELLOW}PiSP Backend supported output formats:${NC}"
v4l2-ctl -d /dev/video23 --list-formats | head -20
echo ""

# Menu
echo "========================================"
echo "Select Processing Mode:"
echo "========================================"
echo "1) Direct capture (no ISP, lowest latency)"
echo "2) GStreamer preview with scaling"
echo "3) Dual-stream output (full + thumbnail)"
echo "4) Format conversion (YUV to RGB)"
echo "5) Check ISP capabilities"
echo "6) Exit"
echo ""
read -p "Enter choice [1-6]: " choice

# Get current format dynamically
CURRENT_FMT=$(v4l2-ctl -d /dev/video0 --get-fmt-video | grep "Pixel Format" | awk '{print $4}' | tr -d "'")

case $choice in
    1)
        echo ""
        echo -e "${GREEN}Starting direct capture from RS300...${NC}"
        echo "Press Ctrl+C to stop"
        echo ""

        gst-launch-1.0 -v \
            v4l2src device=/dev/video0 ! \
            video/x-raw,format=${CURRENT_FMT},width=640,height=512,framerate=30/1 ! \
            videoconvert ! \
            autovideosink
        ;;

    2)
        echo ""
        echo -e "${GREEN}Starting GStreamer preview with scaling...${NC}"
        echo "Press Ctrl+C to stop"
        echo ""

        gst-launch-1.0 -v \
            v4l2src device=/dev/video0 ! \
            video/x-raw,format=${CURRENT_FMT},width=640,height=512,framerate=30/1 ! \
            videoscale ! \
            video/x-raw,width=1280,height=1024 ! \
            videoconvert ! \
            autovideosink
        ;;

    3)
        echo ""
        echo -e "${GREEN}Starting dual-stream output (full + thumbnail)...${NC}"
        echo "Two windows will appear"
        echo "Press Ctrl+C to stop"
        echo ""

        gst-launch-1.0 -v \
            v4l2src device=/dev/video0 ! \
            video/x-raw,format=${CURRENT_FMT},width=640,height=512,framerate=30/1 ! \
            tee name=t \
            t. ! queue ! \
                videoconvert ! \
                autovideosink name="Full Resolution (640x512)" \
            t. ! queue ! \
                videoscale ! \
                video/x-raw,width=320,height=256 ! \
                videoconvert ! \
                autovideosink name="Thumbnail (320x256)"
        ;;

    4)
        echo ""
        echo -e "${GREEN}Format conversion: ${CURRENT_FMT} → RGB...${NC}"
        echo "Press Ctrl+C to stop"
        echo ""

        gst-launch-1.0 -v \
            v4l2src device=/dev/video0 ! \
            video/x-raw,format=${CURRENT_FMT},width=640,height=512,framerate=30/1 ! \
            videoconvert ! \
            video/x-raw,format=RGB ! \
            autovideosink
        ;;

    5)
        echo ""
        echo -e "${YELLOW}=== RS300 Capabilities ===${NC}"
        v4l2-ctl -d /dev/video0 --all
        echo ""

        echo -e "${YELLOW}=== PiSP Backend Input (video20) ===${NC}"
        v4l2-ctl -d /dev/video20 --all 2>&1 | head -30
        echo ""

        echo -e "${YELLOW}=== PiSP Backend Output0 (video23) ===${NC}"
        v4l2-ctl -d /dev/video23 --all 2>&1 | head -30
        echo ""

        echo -e "${YELLOW}=== PiSP Backend Media Topology ===${NC}"
        media-ctl -d /dev/media2 -p
        ;;

    6)
        echo "Exiting..."
        exit 0
        ;;

    *)
        echo -e "${RED}Invalid choice${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Done!${NC}"
