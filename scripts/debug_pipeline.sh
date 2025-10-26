#!/bin/bash

# RS300 Pipeline Debug Script
# Comprehensive diagnostics for media controller pipeline issues

echo "=== RS300 Pipeline Debug Diagnostics ==="
echo "Timestamp: $(date)"
echo "Kernel: $(uname -r)"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    local status=$1
    local message=$2
    case $status in
        "OK")
            echo -e "${GREEN}✓${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}⚠${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}✗${NC} $message"
            ;;
        "INFO")
            echo -e "${BLUE}ℹ${NC} $message"
            ;;
    esac
}

# Function to check if command exists
check_command() {
    if command -v $1 >/dev/null 2>&1; then
        print_status "OK" "$1 command available"
        return 0
    else
        print_status "ERROR" "$1 command not found"
        return 1
    fi
}

echo "=== 1. System Prerequisites ==="
check_command "media-ctl" || { echo "Install v4l-utils package"; exit 1; }
check_command "v4l2-ctl" || { echo "Install v4l-utils package"; exit 1; }
check_command "i2cdetect" || { echo "Install i2c-tools package"; exit 1; }

echo ""
echo "=== 2. Driver Status ==="

# Check if RS300 module is loaded
if lsmod | grep -q rs300; then
    print_status "OK" "RS300 driver loaded"
    RS300_VERSION=$(modinfo rs300 | grep ^version | cut -d: -f2 | xargs)
    print_status "INFO" "Driver version: $RS300_VERSION"
else
    print_status "ERROR" "RS300 driver not loaded"
    echo "Run: ./setup.sh && sudo reboot"
    exit 1
fi

# Check related modules
print_status "INFO" "Related modules loaded:"
lsmod | grep -E "(rp1_cfe|unicam|bcm2835|i2c)" | while read line; do
    echo "  $line"
done

echo ""
echo "=== 3. Device Tree Status ==="

# Check device tree overlay
if [ -f "/proc/device-tree/soc/i2c@107d50000/rs300@3c/compatible" ]; then
    COMPATIBLE=$(cat /proc/device-tree/soc/i2c@107d50000/rs300@3c/compatible 2>/dev/null)
    print_status "OK" "Device tree overlay active: $COMPATIBLE"
else
    print_status "WARN" "Device tree overlay not found in expected location"
    echo "  Try: sudo dtoverlay rs300"
fi

# Check I2C bus configuration
if [ -e "/dev/i2c-10" ]; then
    print_status "OK" "I2C bus 10 available"
else
    print_status "ERROR" "I2C bus 10 not available"
fi

echo ""
echo "=== 4. Hardware Detection ==="

# I2C device detection
print_status "INFO" "Scanning I2C bus 10 for RS300 (0x3c):"
if i2cdetect -y 10 2>/dev/null | grep -q "3c"; then
    print_status "OK" "RS300 detected on I2C bus 10 at address 0x3c"
else
    print_status "ERROR" "RS300 not detected on I2C bus 10"
    echo "Check hardware connections and power"
fi

echo ""
echo "=== 5. Media Controller Analysis ==="

# Find media controller devices
MEDIA_DEVICES=()
for i in {0..5}; do
    if [ -e "/dev/media$i" ]; then
        MEDIA_DEVICES+=("/dev/media$i")
    fi
done

if [ ${#MEDIA_DEVICES[@]} -eq 0 ]; then
    print_status "ERROR" "No media controller devices found"
    exit 1
fi

print_status "INFO" "Found ${#MEDIA_DEVICES[@]} media controller device(s):"
for dev in "${MEDIA_DEVICES[@]}"; do
    echo "  $dev"
done

# Find RS300 in media controller
RS300_MEDIA_DEV=""
for dev in "${MEDIA_DEVICES[@]}"; do
    if media-ctl -d $dev --print-topology 2>/dev/null | grep -q "rs300 10-003c"; then
        RS300_MEDIA_DEV="$dev"
        print_status "OK" "RS300 found in $dev"
        break
    fi
done

if [ -z "$RS300_MEDIA_DEV" ]; then
    print_status "ERROR" "RS300 not found in any media controller device"
    echo ""
    echo "Available entities in media controllers:"
    for dev in "${MEDIA_DEVICES[@]}"; do
        echo "  $dev:"
        media-ctl -d $dev --print-topology 2>/dev/null | grep -E "entity|device" | head -10 | sed 's/^/    /'
    done
    exit 1
fi

echo ""
echo "=== 6. Media Topology Analysis ==="

print_status "INFO" "Full media controller topology for $RS300_MEDIA_DEV:"
media-ctl -d $RS300_MEDIA_DEV --print-topology

echo ""
print_status "INFO" "RS300-specific pipeline components:"
media-ctl -d $RS300_MEDIA_DEV --print-topology | grep -A2 -B2 -E "(rs300|csi2|rp1-cfe)"

echo ""
echo "=== 7. Current Format Configuration ==="

# Get current formats
print_status "INFO" "Current format configuration:"

RS300_FMT=$(media-ctl -d $RS300_MEDIA_DEV --get-v4l2 "'rs300 10-003c':0" 2>/dev/null)
if [ $? -eq 0 ]; then
    print_status "OK" "RS300 format: $RS300_FMT"
else
    print_status "ERROR" "Failed to get RS300 format"
fi

CSI2_IN_FMT=$(media-ctl -d $RS300_MEDIA_DEV --get-v4l2 "'csi2':0" 2>/dev/null)
if [ $? -eq 0 ]; then
    print_status "OK" "CSI2 input format: $CSI2_IN_FMT"
else
    print_status "WARN" "Failed to get CSI2 input format"
fi

CSI2_OUT_FMT=$(media-ctl -d $RS300_MEDIA_DEV --get-v4l2 "'csi2':4" 2>/dev/null)
if [ $? -eq 0 ]; then
    print_status "OK" "CSI2 output format: $CSI2_OUT_FMT"
else
    print_status "WARN" "Failed to get CSI2 output format"
fi

if [ -e "/dev/video0" ]; then
    VIDEO_FMT=$(v4l2-ctl -d /dev/video0 --get-fmt-video 2>/dev/null)
    if [ $? -eq 0 ]; then
        print_status "OK" "Video device format: $VIDEO_FMT"
    else
        print_status "WARN" "Failed to get video device format"
    fi
else
    print_status "ERROR" "/dev/video0 not available"
fi

echo ""
echo "=== 8. Link Status Analysis ==="

print_status "INFO" "Media controller links:"
media-ctl -d $RS300_MEDIA_DEV --print-topology | grep -E "(->|ENABLED|DISABLED)" | while read line; do
    if echo "$line" | grep -q "ENABLED"; then
        print_status "OK" "$line"
    elif echo "$line" | grep -q "DISABLED"; then
        print_status "WARN" "$line"
    else
        echo "  $line"
    fi
done

echo ""
echo "=== 9. V4L2 Device Analysis ==="

# Check video devices
print_status "INFO" "Available video devices:"
for i in {0..5}; do
    if [ -e "/dev/video$i" ]; then
        DEVICE_INFO=$(v4l2-ctl -d /dev/video$i --info 2>/dev/null | grep -E "(Card type|Driver name)")
        if [ $? -eq 0 ]; then
            print_status "OK" "/dev/video$i: $DEVICE_INFO"
        fi
    fi
done

# Check subdevices
print_status "INFO" "Available V4L2 subdevices:"
for i in {0..10}; do
    if [ -e "/dev/v4l-subdev$i" ]; then
        SUBDEV_INFO=$(v4l2-ctl -d /dev/v4l-subdev$i --info 2>/dev/null | grep -E "(Card type|Driver name)")
        if [ $? -eq 0 ]; then
            print_status "OK" "/dev/v4l-subdev$i: $SUBDEV_INFO"
            
            # Check if this is the RS300 subdevice
            if echo "$SUBDEV_INFO" | grep -q "rs300"; then
                print_status "INFO" "RS300 controls available:"
                v4l2-ctl -d /dev/v4l-subdev$i --list-ctrls 2>/dev/null | sed 's/^/  /'
            fi
        fi
    fi
done

echo ""
echo "=== 10. Clock and Power Analysis ==="

# Check clock configuration
if [ -d "/sys/kernel/debug/clk" ]; then
    print_status "INFO" "Camera-related clocks:"
    grep -r "cam" /sys/kernel/debug/clk/*/clk_rate 2>/dev/null | head -5 | sed 's/^/  /'
else
    print_status "WARN" "Clock debug information not available"
fi

# Check power domains
if [ -d "/sys/kernel/debug/pm_genpd" ]; then
    print_status "INFO" "Power domain status:"
    cat /sys/kernel/debug/pm_genpd/pm_genpd_summary 2>/dev/null | grep -E "(device|rp1)" | head -5 | sed 's/^/  /'
else
    print_status "WARN" "Power domain debug information not available"
fi

echo ""
echo "=== 11. Recent Kernel Messages ==="

print_status "INFO" "Recent RS300-related kernel messages:"
dmesg | grep -E "(rs300|RS300)" | tail -10 | sed 's/^/  /'

print_status "INFO" "Recent media/camera-related errors:"
dmesg | grep -E "(rp1-cfe|csi2|media|v4l2)" | grep -iE "(error|fail|invalid)" | tail -5 | sed 's/^/  /'

echo ""
echo "=== 12. Configuration Recommendations ==="

# Analyze issues and provide recommendations
HAS_ISSUES=false

if [ -z "$RS300_FMT" ]; then
    print_status "ERROR" "RS300 format not configured"
    echo "  → Run: media-ctl -d $RS300_MEDIA_DEV -V \"'rs300 10-003c':0 [fmt:YUYV8_2X8/640x512]\""
    HAS_ISSUES=true
fi

if ! media-ctl -d $RS300_MEDIA_DEV --print-topology | grep -q "csi2.*4.*rp1-cfe.*ENABLED"; then
    print_status "ERROR" "Critical pipeline link not enabled"
    echo "  → Run: media-ctl -d $RS300_MEDIA_DEV -l \"'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]\""
    HAS_ISSUES=true
fi

if [ ! -e "/dev/video0" ]; then
    print_status "ERROR" "Video device not available"
    echo "  → Check rp1-cfe driver loading and media controller configuration"
    HAS_ISSUES=true
fi

if ! i2cdetect -y 10 2>/dev/null | grep -q "3c"; then
    print_status "ERROR" "I2C communication issue"
    echo "  → Check hardware connections, power supply, and device tree overlay"
    HAS_ISSUES=true
fi

echo ""
if [ "$HAS_ISSUES" = false ]; then
    print_status "OK" "No critical issues detected - pipeline should be functional"
    echo ""
    echo "Next steps:"
    echo "1. Run ./test_formats.sh to find working format combination"
    echo "2. Test streaming with working format"
    echo "3. Validate thermal camera functionality"
else
    print_status "WARN" "Issues detected - follow recommendations above"
    echo ""
    echo "Troubleshooting order:"
    echo "1. Fix hardware/I2C issues first"
    echo "2. Ensure device tree overlay is loaded"
    echo "3. Configure media controller pipeline"
    echo "4. Test format combinations"
fi

echo ""
echo "=== Debug Complete ==="
echo "For detailed format testing, run: ./test_formats.sh"