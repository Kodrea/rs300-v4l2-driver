#!/bin/bash

# RS300 Diagnostic Log Capture Script
# Captures comprehensive system state for documentation and debugging
#
# Usage:
#   ./capture_logs.sh                    # Interactive mode with prompts
#   ./capture_logs.sh --before-config    # Capture state before configuration
#   ./capture_logs.sh --after-config     # Capture state after configuration
#   ./capture_logs.sh --full             # Capture full diagnostic suite

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
LOG_FILE="${LOG_DIR}/rs300_diagnostics_${TIMESTAMP}.log"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}========================================${NC}"
}

print_section() {
    echo ""
    echo -e "${BLUE}▶▶▶ $1${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

# Create log directory if it doesn't exist
mkdir -p "$LOG_DIR"

# Parse command line arguments
MODE="interactive"
if [ "$1" == "--before-config" ]; then
    MODE="before"
elif [ "$1" == "--after-config" ]; then
    MODE="after"
elif [ "$1" == "--full" ]; then
    MODE="full"
fi

# Start logging
{
    print_header "RS300 Thermal Camera Diagnostic Log Capture"
    echo "Timestamp: $(date)"
    echo "Hostname: $(hostname)"
    echo "Kernel: $(uname -r)"
    echo "Mode: $MODE"
    echo ""

    # System Information
    print_section "1. System Information"
    echo "OS Release:"
    cat /etc/os-release 2>/dev/null || echo "  N/A"
    echo ""
    echo "Kernel Version:"
    uname -a
    echo ""
    echo "Architecture:"
    dpkg --print-architecture 2>/dev/null || echo "  N/A"
    echo ""
    echo "Raspberry Pi Model:"
    cat /proc/device-tree/model 2>/dev/null || echo "  N/A"
    echo ""

    # Driver Status
    print_section "2. Driver and Module Status"
    echo "Loaded RS300 Module:"
    lsmod | grep rs300 || echo "  RS300 module NOT loaded"
    echo ""
    echo "RS300 Module Info:"
    modinfo rs300 2>/dev/null || echo "  Module info not available"
    echo ""
    echo "Module Parameters:"
    if [ -d "/sys/module/rs300/parameters" ]; then
        for param in /sys/module/rs300/parameters/*; do
            echo "  $(basename $param) = $(cat $param 2>/dev/null)"
        done
    else
        echo "  Parameters not accessible"
    fi
    echo ""
    echo "Related Camera/Video Modules:"
    lsmod | grep -E "(rp1_cfe|unicam|bcm2835|i2c|v4l2)" | head -20
    echo ""

    # Device Tree Status
    print_section "3. Device Tree and Hardware Configuration"
    echo "Boot Config (camera-related lines):"
    grep -E "(camera|dtoverlay=rs300)" /boot/firmware/config.txt 2>/dev/null || echo "  N/A"
    echo ""
    echo "Device Tree RS300 Node:"
    if [ -d "/proc/device-tree/soc" ]; then
        find /proc/device-tree/soc -name "*rs300*" -o -name "*003c*" 2>/dev/null | while read node; do
            echo "  Found: $node"
            if [ -f "$node/compatible" ]; then
                echo "    Compatible: $(cat $node/compatible 2>/dev/null | tr '\0' ' ')"
            fi
        done
    else
        echo "  Device tree not accessible"
    fi
    echo ""
    echo "Loaded Device Tree Overlays:"
    dtoverlay -l 2>/dev/null || echo "  dtoverlay command not available"
    echo ""

    # I2C Hardware Detection
    print_section "4. I2C Hardware Detection"
    echo "Available I2C Buses:"
    ls -1 /dev/i2c-* 2>/dev/null || echo "  No I2C devices found"
    echo ""
    echo "I2C Bus 10 Scan (RS300 should be at 0x3c):"
    if [ -c "/dev/i2c-10" ]; then
        i2cdetect -y 10 2>/dev/null || echo "  i2cdetect failed"
    else
        echo "  /dev/i2c-10 not available"
    fi
    echo ""

    # Media Controller Devices
    print_section "5. Media Controller Devices"
    echo "Available Media Devices:"
    for i in {0..5}; do
        if [ -e "/dev/media$i" ]; then
            echo ""
            echo "  /dev/media$i:"
            media-ctl -d /dev/media$i --info 2>/dev/null | sed 's/^/    /'
        fi
    done
    echo ""

    # Media Topology
    print_section "6. Media Controller Topology"
    echo "Searching for RS300 in media controllers..."
    RS300_FOUND=false
    for i in {0..5}; do
        if [ -e "/dev/media$i" ]; then
            if media-ctl -d /dev/media$i --print-topology 2>/dev/null | grep -q "rs300 10-003c"; then
                RS300_FOUND=true
                echo ""
                echo "RS300 FOUND on /dev/media$i"
                echo ""
                echo "Full Topology:"
                media-ctl -d /dev/media$i --print-topology 2>/dev/null | sed 's/^/  /'
                echo ""
                RS300_MEDIA_DEV="/dev/media$i"
                break
            fi
        fi
    done

    if [ "$RS300_FOUND" = false ]; then
        echo "  RS300 NOT FOUND in any media controller"
        echo ""
        echo "  Available entities in all media controllers:"
        for i in {0..5}; do
            if [ -e "/dev/media$i" ]; then
                echo "    /dev/media$i:"
                media-ctl -d /dev/media$i --print-topology 2>/dev/null | grep "entity" | head -10 | sed 's/^/      /'
            fi
        done
    fi
    echo ""

    # Format Configuration
    print_section "7. Current Format Configuration"
    if [ -n "$RS300_MEDIA_DEV" ]; then
        echo "RS300 Sensor Pad 0 Format:"
        media-ctl -d "$RS300_MEDIA_DEV" --get-v4l2 "'rs300 10-003c':0" 2>/dev/null | sed 's/^/  /' || echo "  Failed to read format"
        echo ""
        echo "RS300 Sensor Pad 1 Format (metadata):"
        media-ctl -d "$RS300_MEDIA_DEV" --get-v4l2 "'rs300 10-003c':1" 2>/dev/null | sed 's/^/  /' || echo "  Failed to read format"
        echo ""
        echo "CSI-2 Input (Pad 0) Format:"
        media-ctl -d "$RS300_MEDIA_DEV" --get-v4l2 "'csi2':0" 2>/dev/null | sed 's/^/  /' || echo "  Failed to read format"
        echo ""
        echo "CSI-2 Output (Pad 4) Format:"
        media-ctl -d "$RS300_MEDIA_DEV" --get-v4l2 "'csi2':4" 2>/dev/null | sed 's/^/  /' || echo "  Failed to read format"
        echo ""
    fi

    # Video Devices
    print_section "8. Video Device Status"
    echo "Available Video Devices:"
    for i in {0..5}; do
        if [ -e "/dev/video$i" ]; then
            echo ""
            echo "  /dev/video$i:"
            v4l2-ctl -d /dev/video$i --info 2>/dev/null | sed 's/^/    /'
            echo ""
            echo "    Current Format:"
            v4l2-ctl -d /dev/video$i --get-fmt-video 2>/dev/null | sed 's/^/      /'
        fi
    done
    echo ""

    # V4L2 Subdevices
    print_section "9. V4L2 Subdevice Status"
    echo "Available V4L2 Subdevices:"
    for i in {0..10}; do
        if [ -e "/dev/v4l-subdev$i" ]; then
            echo ""
            echo "  /dev/v4l-subdev$i:"
            v4l2-ctl -d /dev/v4l-subdev$i --info 2>/dev/null | sed 's/^/    /'

            # If this is RS300, show controls
            SUBDEV_INFO=$(v4l2-ctl -d /dev/v4l-subdev$i --info 2>/dev/null)
            if echo "$SUBDEV_INFO" | grep -q "rs300"; then
                echo ""
                echo "    RS300 Camera Controls:"
                v4l2-ctl -d /dev/v4l-subdev$i --list-ctrls 2>/dev/null | sed 's/^/      /'
            fi
        fi
    done
    echo ""

    # Link Status
    if [ -n "$RS300_MEDIA_DEV" ]; then
        print_section "10. Media Pipeline Link Status"
        echo "All Links:"
        media-ctl -d "$RS300_MEDIA_DEV" --print-topology 2>/dev/null | grep -E "(pad|->)" | sed 's/^/  /'
        echo ""
        echo "Enabled Links:"
        media-ctl -d "$RS300_MEDIA_DEV" --print-topology 2>/dev/null | grep "ENABLED" | sed 's/^/  /'
        echo ""
    fi

    # Kernel Messages
    print_section "11. Kernel Messages (dmesg)"
    echo "RS300-specific messages:"
    dmesg | grep -i rs300 | tail -30 | sed 's/^/  /'
    echo ""
    echo "Recent media/camera errors:"
    dmesg | grep -E "(rp1-cfe|csi2|media|v4l2)" | grep -iE "(error|fail|warn)" | tail -20 | sed 's/^/  /'
    echo ""
    echo "I2C-related messages:"
    dmesg | grep -i i2c | grep -E "(10-003c|rs300)" | tail -15 | sed 's/^/  /'
    echo ""

    # Systemd Service Status (if installed)
    print_section "12. Auto-Configuration Status"
    echo "Systemd Service Status:"
    if systemctl list-unit-files | grep -q rs300-media-config.service; then
        systemctl status rs300-media-config.service --no-pager 2>/dev/null | sed 's/^/  /'
        echo ""
        echo "Service Logs (last 20 lines):"
        journalctl -u rs300-media-config.service --no-pager -n 20 2>/dev/null | sed 's/^/  /'
    else
        echo "  Service not installed"
    fi
    echo ""

    echo "Udev Rule Status:"
    if [ -f "/etc/udev/rules.d/99-rs300.rules" ]; then
        echo "  Udev rule INSTALLED"
        echo ""
        echo "  Rule content:"
        grep -v "^#" /etc/udev/rules.d/99-rs300.rules | grep -v "^$" | sed 's/^/    /'
        echo ""
        if [ -f "/var/log/rs300-udev-config.log" ]; then
            echo "  Recent udev config log:"
            tail -20 /var/log/rs300-udev-config.log 2>/dev/null | sed 's/^/    /'
        fi
    else
        echo "  Udev rule NOT installed"
    fi
    echo ""

    # Configuration Script Outputs
    if [ "$MODE" == "full" ] || [ "$MODE" == "after" ]; then
        print_section "13. Test Streaming Capability"
        echo "Attempting to capture 1 frame..."
        if timeout 5s v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=/dev/null 2>&1; then
            echo "  ✓ Streaming SUCCESSFUL"
        else
            echo "  ✗ Streaming FAILED"
        fi
        echo ""
    fi

    # Summary
    print_section "14. Diagnostic Summary"

    ISSUES_FOUND=false

    if ! lsmod | grep -q rs300; then
        echo "  ✗ RS300 driver NOT loaded"
        ISSUES_FOUND=true
    else
        echo "  ✓ RS300 driver loaded"
    fi

    if ! i2cdetect -y 10 2>/dev/null | grep -q "3c"; then
        echo "  ✗ RS300 NOT detected on I2C bus 10"
        ISSUES_FOUND=true
    else
        echo "  ✓ RS300 detected on I2C (0x3c)"
    fi

    if [ "$RS300_FOUND" = false ]; then
        echo "  ✗ RS300 NOT found in media controller"
        ISSUES_FOUND=true
    else
        echo "  ✓ RS300 found in media controller ($RS300_MEDIA_DEV)"
    fi

    if [ -n "$RS300_MEDIA_DEV" ]; then
        if media-ctl -d "$RS300_MEDIA_DEV" --print-topology 2>/dev/null | grep -q "rp1-cfe.*ENABLED"; then
            echo "  ✓ Pipeline link ENABLED"
        else
            echo "  ✗ Pipeline link NOT enabled"
            ISSUES_FOUND=true
        fi
    fi

    if [ ! -e "/dev/video0" ]; then
        echo "  ✗ /dev/video0 NOT available"
        ISSUES_FOUND=true
    else
        echo "  ✓ /dev/video0 available"
    fi

    echo ""
    if [ "$ISSUES_FOUND" = true ]; then
        echo "  ⚠ Issues detected - review log for details"
    else
        echo "  ✓ No critical issues detected"
    fi
    echo ""

    # Footer
    print_header "End of Diagnostic Log"
    echo "Log saved to: $LOG_FILE"
    echo ""

} 2>&1 | tee "$LOG_FILE"

# Print summary to user
echo ""
print_success "Diagnostic log captured successfully!"
echo ""
print_info "Log file: $LOG_FILE"
echo ""
echo "To share this log for troubleshooting:"
echo "  cat $LOG_FILE"
echo ""
echo "To compare before/after configuration:"
echo "  ./capture_logs.sh --before-config"
echo "  ./configure_media.sh"
echo "  ./capture_logs.sh --after-config"
echo "  diff logs/rs300_diagnostics_*before* logs/rs300_diagnostics_*after*"
echo ""
