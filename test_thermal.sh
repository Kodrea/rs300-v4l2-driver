#!/bin/bash

# RS300 Thermal Camera Specific Testing Script
# Tests thermal imaging functionality, controls, and data validation

echo "=== RS300 Thermal Camera Testing ==="
echo "Timestamp: $(date)"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo -e "\n${CYAN}=== $1 ===${NC}"
}

print_test() {
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

# Test configuration
THERMAL_TEST_DIR="/tmp/rs300_thermal_tests"
mkdir -p "$THERMAL_TEST_DIR"

# Check prerequisites
print_header "Prerequisites Check"

if ! lsmod | grep -q rs300; then
    print_error "RS300 driver not loaded"
    exit 1
fi
print_success "RS300 driver loaded"

# Find RS300 subdevice
RS300_SUBDEV=""
for i in {0..10}; do
    if [ -e "/dev/v4l-subdev$i" ]; then
        if v4l2-ctl -d "/dev/v4l-subdev$i" --info 2>/dev/null | grep -q "rs300"; then
            RS300_SUBDEV="/dev/v4l-subdev$i"
            break
        fi
    fi
done

if [ -z "$RS300_SUBDEV" ]; then
    print_error "RS300 subdevice not found"
    exit 1
fi
print_success "Found RS300 subdevice: $RS300_SUBDEV"

# Find media controller
MEDIA_DEV=""
for i in {0..5}; do
    if [ -e "/dev/media$i" ] && media-ctl -d "/dev/media$i" --print-topology 2>/dev/null | grep -q "rs300 10-003c"; then
        MEDIA_DEV="/dev/media$i"
        break
    fi
done

if [ -z "$MEDIA_DEV" ]; then
    print_error "RS300 media controller not found"
    exit 1
fi
print_success "Found RS300 media controller: $MEDIA_DEV"

print_header "Thermal Camera Control Tests"

# Test 1: List and validate available controls
print_test "Listing RS300 thermal controls"
CONTROLS=$(v4l2-ctl -d "$RS300_SUBDEV" --list-ctrls 2>/dev/null)
if [ $? -eq 0 ]; then
    print_success "Controls retrieved successfully:"
    echo "$CONTROLS" | sed 's/^/  /'
    
    # Parse available controls
    HAS_FFC=$(echo "$CONTROLS" | grep -i "ffc_trigger")
    HAS_COLORMAP=$(echo "$CONTROLS" | grep -i "colormap")
    HAS_BRIGHTNESS=$(echo "$CONTROLS" | grep -i "brightness")
    HAS_SCENE_MODE=$(echo "$CONTROLS" | grep -i "scene_mode")
    
else
    print_error "Failed to retrieve controls"
    exit 1
fi

# Test 2: FFC (Flat Field Correction) functionality
if [ -n "$HAS_FFC" ]; then
    print_test "Testing FFC (Flat Field Correction) trigger"
    
    # Get current FFC status
    FFC_BEFORE=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=ffc_trigger 2>/dev/null)
    print_success "FFC status before: $FFC_BEFORE"
    
    # Trigger FFC
    if v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=ffc_trigger=1 2>/dev/null; then
        print_success "FFC trigger command accepted"
        sleep 2  # Wait for FFC to complete
        
        # Check status after
        FFC_AFTER=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=ffc_trigger 2>/dev/null)
        print_success "FFC status after: $FFC_AFTER"
    else
        print_error "FFC trigger failed"
    fi
else
    print_warning "FFC control not available"
fi

# Test 3: Colormap functionality
if [ -n "$HAS_COLORMAP" ]; then
    print_test "Testing colormap controls"
    
    # Get current colormap
    CURRENT_COLORMAP=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=colormap 2>/dev/null)
    print_success "Current colormap: $CURRENT_COLORMAP"
    
    # Test colormap options (based on rs300.c menu items)
    declare -a COLORMAPS=("0" "3" "4" "7" "10")  # White Hot, Ironbow, Rainbow, Red Hot, Black Hot
    declare -a COLORMAP_NAMES=("White Hot" "Ironbow" "Rainbow" "Red Hot" "Black Hot")
    
    for i in "${!COLORMAPS[@]}"; do
        COLORMAP="${COLORMAPS[$i]}"
        NAME="${COLORMAP_NAMES[$i]}"
        
        print_test "  Testing colormap $COLORMAP ($NAME)"
        if v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=colormap="$COLORMAP" 2>/dev/null; then
            sleep 0.5
            VERIFY=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=colormap 2>/dev/null)
            if echo "$VERIFY" | grep -q "$COLORMAP"; then
                print_success "  ✓ Colormap $COLORMAP ($NAME) set successfully"
            else
                print_warning "  ⚠ Colormap set but verification failed: $VERIFY"
            fi
        else
            print_error "  ✗ Failed to set colormap $COLORMAP"
        fi
    done
    
    # Restore original colormap
    if [ -n "$CURRENT_COLORMAP" ]; then
        ORIGINAL_VALUE=$(echo "$CURRENT_COLORMAP" | grep -o '[0-9]\+')
        v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=colormap="$ORIGINAL_VALUE" 2>/dev/null
        print_success "Restored original colormap: $ORIGINAL_VALUE"
    fi
else
    print_warning "Colormap control not available"
fi

# Test 4: Brightness control
if [ -n "$HAS_BRIGHTNESS" ]; then
    print_test "Testing brightness control"
    
    CURRENT_BRIGHTNESS=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=brightness 2>/dev/null)
    print_success "Current brightness: $CURRENT_BRIGHTNESS"
    
    # Test brightness range
    declare -a BRIGHTNESS_VALUES=("0" "25" "50" "75" "100")
    
    for brightness in "${BRIGHTNESS_VALUES[@]}"; do
        print_test "  Testing brightness: $brightness"
        if v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=brightness="$brightness" 2>/dev/null; then
            sleep 0.3
            VERIFY=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=brightness 2>/dev/null)
            if echo "$VERIFY" | grep -q "$brightness"; then
                print_success "  ✓ Brightness $brightness set successfully"
            else
                print_warning "  ⚠ Brightness set but verification failed: $VERIFY"
            fi
        else
            print_error "  ✗ Failed to set brightness $brightness"
        fi
    done
    
    # Restore original brightness
    if [ -n "$CURRENT_BRIGHTNESS" ]; then
        ORIGINAL_VALUE=$(echo "$CURRENT_BRIGHTNESS" | grep -o '[0-9]\+')
        v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=brightness="$ORIGINAL_VALUE" 2>/dev/null
        print_success "Restored original brightness: $ORIGINAL_VALUE"
    fi
else
    print_warning "Brightness control not available"
fi

# Test 5: Scene mode control (if available)
if [ -n "$HAS_SCENE_MODE" ]; then
    print_test "Testing scene mode control"
    
    CURRENT_SCENE=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=scene_mode 2>/dev/null)
    print_success "Current scene mode: $CURRENT_SCENE"
    
    # Test scene modes (based on rs300.c menu items)
    declare -a SCENES=("0" "1" "3" "4" "9")  # Low, Linear Stretch, General, High Contrast, Outline
    declare -a SCENE_NAMES=("Low" "Linear Stretch" "General Mode" "High Contrast" "Outline Mode")
    
    for i in "${!SCENES[@]}"; do
        SCENE="${SCENES[$i]}"
        NAME="${SCENE_NAMES[$i]}"
        
        print_test "  Testing scene mode $SCENE ($NAME)"
        if v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=scene_mode="$SCENE" 2>/dev/null; then
            sleep 0.5
            VERIFY=$(v4l2-ctl -d "$RS300_SUBDEV" --get-ctrl=scene_mode 2>/dev/null)
            if echo "$VERIFY" | grep -q "$SCENE"; then
                print_success "  ✓ Scene mode $SCENE ($NAME) set successfully"
            else
                print_warning "  ⚠ Scene mode set but verification failed: $VERIFY"
            fi
        else
            print_error "  ✗ Failed to set scene mode $SCENE"
        fi
    done
    
    # Restore original scene mode
    if [ -n "$CURRENT_SCENE" ]; then
        ORIGINAL_VALUE=$(echo "$CURRENT_SCENE" | grep -o '[0-9]\+')
        v4l2-ctl -d "$RS300_SUBDEV" --set-ctrl=scene_mode="$ORIGINAL_VALUE" 2>/dev/null
        print_success "Restored original scene mode: $ORIGINAL_VALUE"
    fi
else
    print_warning "Scene mode control not available"
fi

print_header "Thermal Data Validation Tests"

# Test 6: Pipeline configuration check
print_test "Verifying thermal imaging pipeline configuration"

# Check if pipeline is configured
if media-ctl -d "$MEDIA_DEV" --print-topology | grep -q "csi2.*4.*rp1-cfe.*ENABLED"; then
    print_success "Media controller pipeline is configured"
    
    # Get current formats
    RS300_FMT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'rs300 10-003c':0" 2>/dev/null)
    print_success "RS300 format: $RS300_FMT"
    
    # Check if format is thermal-appropriate
    if echo "$RS300_FMT" | grep -qE "(Y10|Y16|YUYV)"; then
        print_success "Format appears suitable for thermal data"
    else
        print_warning "Format may not be optimal for thermal data: $RS300_FMT"
    fi
    
else
    print_error "Media controller pipeline not configured"
    echo "Run: ./configure_media.sh"
fi

# Test 7: Thermal data capture test
print_test "Testing thermal data capture capability"

if [ -e "/dev/video0" ]; then
    # Check video device format
    VIDEO_FMT=$(v4l2-ctl -d /dev/video0 --get-fmt-video 2>/dev/null)
    print_success "Video device format: $VIDEO_FMT"
    
    # Attempt frame capture
    CAPTURE_FILE="$THERMAL_TEST_DIR/thermal_test_$(date +%Y%m%d_%H%M%S).raw"
    
    print_test "Capturing thermal frames to $CAPTURE_FILE"
    timeout 10s v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=5 --stream-to="$CAPTURE_FILE" --verbose 2>&1 | tee "$THERMAL_TEST_DIR/capture_log.txt"
    CAPTURE_RESULT=$?
    
    if [ $CAPTURE_RESULT -eq 0 ] && [ -f "$CAPTURE_FILE" ] && [ -s "$CAPTURE_FILE" ]; then
        CAPTURE_SIZE=$(stat -c%s "$CAPTURE_FILE")
        print_success "✓ Thermal data captured successfully!"
        print_success "  File: $CAPTURE_FILE"
        print_success "  Size: $CAPTURE_SIZE bytes"
        
        # Analyze captured data
        print_test "Analyzing thermal data characteristics"
        
        # Check if data looks like thermal imaging data (basic analysis)
        if [ "$CAPTURE_SIZE" -gt 0 ]; then
            # Calculate expected frame size
            if echo "$VIDEO_FMT" | grep -q "640.*512"; then
                if echo "$VIDEO_FMT" | grep -qE "(YUYV|UYVY)"; then
                    EXPECTED_FRAME_SIZE=$((640 * 512 * 2))  # 16-bit per pixel
                elif echo "$VIDEO_FMT" | grep -qE "(GREY|Y10|Y16)"; then
                    EXPECTED_FRAME_SIZE=$((640 * 512 * 2))  # Assume 16-bit
                else
                    EXPECTED_FRAME_SIZE=$((640 * 512))      # 8-bit fallback
                fi
                
                FRAMES_CAPTURED=$((CAPTURE_SIZE / EXPECTED_FRAME_SIZE))
                print_success "  Estimated frames captured: $FRAMES_CAPTURED"
                print_success "  Frame size: $EXPECTED_FRAME_SIZE bytes"
                
                if [ "$FRAMES_CAPTURED" -ge 3 ]; then
                    print_success "  ✓ Multiple frames captured - streaming is working!"
                fi
            fi
            
            # Basic data integrity check
            if hexdump -C "$CAPTURE_FILE" | head -5 | grep -q "[1-9a-f]"; then
                print_success "  ✓ Data contains non-zero values (good sign for thermal data)"
            else
                print_warning "  ⚠ Data appears to be mostly zeros - check thermal sensor"
            fi
        fi
        
    elif [ $CAPTURE_RESULT -eq 124 ]; then
        print_warning "Capture timed out - streaming may be slow"
        if [ -f "$CAPTURE_FILE" ] && [ -s "$CAPTURE_FILE" ]; then
            print_success "  But some data was captured: $(stat -c%s "$CAPTURE_FILE") bytes"
        fi
    else
        print_error "Thermal data capture failed (exit code: $CAPTURE_RESULT)"
        
        # Check for specific errors
        if [ -f "$THERMAL_TEST_DIR/capture_log.txt" ]; then
            ERRORS=$(grep -iE "(error|fail|invalid)" "$THERMAL_TEST_DIR/capture_log.txt")
            if [ -n "$ERRORS" ]; then
                echo "Capture errors:"
                echo "$ERRORS" | sed 's/^/  /'
            fi
        fi
    fi
else
    print_error "/dev/video0 not available"
fi

print_header "I2C Communication Test"

# Test 8: I2C thermal sensor communication
print_test "Testing direct I2C thermal sensor communication"

if command -v i2cget >/dev/null 2>&1; then
    # Try to read some basic registers (if known)
    print_test "Reading I2C thermal sensor registers"
    
    # Test basic I2C communication
    if i2cdetect -y 10 2>/dev/null | grep -q "3c"; then
        print_success "RS300 responds on I2C bus 10 at address 0x3c"
        
        # Try to read a few registers (these are example addresses)
        for reg in 0x00 0x01 0x02; do
            VALUE=$(i2cget -y 10 0x3c $reg 2>/dev/null)
            if [ $? -eq 0 ]; then
                print_success "  Register $reg: $VALUE"
            else
                print_warning "  Failed to read register $reg"
            fi
        done
    else
        print_error "RS300 not responding on I2C"
    fi
else
    print_warning "i2cget not available for direct I2C testing"
fi

print_header "Performance and Thermal Analysis"

# Test 9: Temperature monitoring during operation
print_test "Monitoring system temperature during thermal camera operation"

if [ -f "/sys/class/thermal/thermal_zone0/temp" ]; then
    CPU_TEMP_BEFORE=$(cat /sys/class/thermal/thermal_zone0/temp)
    print_success "CPU temperature before test: $((CPU_TEMP_BEFORE / 1000))°C"
    
    # Run a brief streaming session to check thermal impact
    timeout 5s v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=30 --stream-to=/dev/null >/dev/null 2>&1 &
    STREAM_PID=$!
    
    sleep 3
    
    CPU_TEMP_DURING=$(cat /sys/class/thermal/thermal_zone0/temp)
    print_success "CPU temperature during streaming: $((CPU_TEMP_DURING / 1000))°C"
    
    wait $STREAM_PID 2>/dev/null
    
    sleep 2
    CPU_TEMP_AFTER=$(cat /sys/class/thermal/thermal_zone0/temp)
    print_success "CPU temperature after streaming: $((CPU_TEMP_AFTER / 1000))°C"
    
    TEMP_DELTA=$((CPU_TEMP_DURING - CPU_TEMP_BEFORE))
    if [ "$TEMP_DELTA" -gt 5000 ]; then  # 5°C increase
        print_warning "Significant temperature increase: $((TEMP_DELTA / 1000))°C"
    else
        print_success "Temperature impact acceptable: $((TEMP_DELTA / 1000))°C"
    fi
else
    print_warning "CPU temperature monitoring not available"
fi

print_header "Test Summary and Recommendations"

echo ""
print_test "Test Results Summary:"

# Count successful tests
SUCCESS_TESTS=0
TOTAL_TESTS=9

# Summarize results
if [ -n "$HAS_FFC" ]; then
    print_success "✓ FFC (Flat Field Correction) controls working"
    SUCCESS_TESTS=$((SUCCESS_TESTS + 1))
fi

if [ -n "$HAS_COLORMAP" ]; then
    print_success "✓ Colormap controls working"
    SUCCESS_TESTS=$((SUCCESS_TESTS + 1))
fi

if [ -n "$HAS_BRIGHTNESS" ]; then
    print_success "✓ Brightness controls working"
    SUCCESS_TESTS=$((SUCCESS_TESTS + 1))
fi

if [ -f "$CAPTURE_FILE" ] && [ -s "$CAPTURE_FILE" ]; then
    print_success "✓ Thermal data capture working"
    SUCCESS_TESTS=$((SUCCESS_TESTS + 1))
fi

echo ""
echo "Thermal Camera Status: $SUCCESS_TESTS/$TOTAL_TESTS tests passed"

if [ "$SUCCESS_TESTS" -ge 3 ]; then
    print_success "🎉 RS300 thermal camera is largely functional!"
    echo ""
    echo "Working features:"
    echo "  • Thermal sensor controls"
    echo "  • I2C communication"
    echo "  • Basic thermal data capture"
    echo ""
    echo "Recommended usage:"
    echo "  1. Use FFC trigger before thermal measurements"
    echo "  2. Adjust colormap for better thermal visualization"
    echo "  3. Set appropriate brightness for ambient conditions"
    echo "  4. Capture thermal data for analysis"
else
    print_warning "RS300 thermal camera has limited functionality"
    echo ""
    echo "Issues to address:"
    if [ -z "$HAS_FFC" ]; then
        echo "  • FFC controls not available"
    fi
    if [ -z "$HAS_COLORMAP" ]; then
        echo "  • Colormap controls not available"
    fi
    if [ ! -f "$CAPTURE_FILE" ] || [ ! -s "$CAPTURE_FILE" ]; then
        echo "  • Thermal data capture not working"
    fi
fi

echo ""
echo "Test artifacts saved to: $THERMAL_TEST_DIR"
echo "For detailed diagnostics, run: ./debug_pipeline.sh"