#!/bin/bash
# RS300 V4L2 Control Testing Script
# Tests all camera controls and verifies responses

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SUBDEV="/dev/v4l-subdev2"
VIDEO_DEV="/dev/video0"
LOG_FILE="control_test_$(date +%Y%m%d_%H%M%S).log"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Print functions
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"

    # Check if running as root for i2c access
    if [[ $EUID -ne 0 ]] && ! groups | grep -q i2c; then
        print_info "Note: Not running as root. Some I2C tests may be skipped."
    fi

    # Check if devices exist
    if [ ! -e "$SUBDEV" ]; then
        echo -e "${RED}Error: Subdevice $SUBDEV not found${NC}"
        echo "Please ensure driver is loaded and media pipeline is configured"
        exit 1
    fi

    if [ ! -e "$VIDEO_DEV" ]; then
        echo -e "${RED}Error: Video device $VIDEO_DEV not found${NC}"
        exit 1
    fi

    # Check for required tools
    for tool in v4l2-ctl media-ctl; do
        if ! command -v $tool &> /dev/null; then
            echo -e "${RED}Error: $tool not found. Please install v4l-utils${NC}"
            exit 1
        fi
    done

    print_pass "All prerequisites met"
    echo
}

# Test control setting and getting
test_control() {
    local control_name=$1
    local test_value=$2
    local description=$3

    ((TESTS_RUN++))
    print_test "Testing $control_name = $test_value ($description)"

    # Set the control
    if v4l2-ctl -d $SUBDEV -c ${control_name}=${test_value} 2>&1 | tee -a $LOG_FILE | grep -q "error\|failed"; then
        print_fail "Failed to set $control_name to $test_value"
        return 1
    fi

    # Small delay for camera processing
    sleep 0.2

    # For non-button controls, verify the value was set
    if [[ $control_name != *"trigger"* ]]; then
        local current_value=$(v4l2-ctl -d $SUBDEV -C ${control_name} 2>/dev/null | awk -F': ' '{print $2}')
        if [ "$current_value" == "$test_value" ]; then
            print_pass "$control_name set to $test_value successfully"
            return 0
        else
            print_fail "$control_name readback mismatch (expected $test_value, got $current_value)"
            return 1
        fi
    else
        print_pass "$control_name triggered successfully"
        return 0
    fi
}

# Test brightness control
test_brightness() {
    print_header "Testing Brightness Control (0-100)"

    for value in 0 25 50 75 100; do
        test_control "brightness" $value "$(($value))%"
        sleep 0.5
    done

    # Reset to default
    test_control "brightness" 50 "Reset to default"
    echo
}

# Test contrast control
test_contrast() {
    print_header "Testing Contrast Control (0-100)"

    for value in 0 50 100; do
        test_control "contrast" $value "$(($value))%"
        sleep 0.5
    done

    # Reset to default
    test_control "contrast" 50 "Reset to default"
    echo
}

# Test colormap control
test_colormap() {
    print_header "Testing Colormap Control (0-11)"

    local colormaps=(
        "0:White Hot"
        "2:Sepia"
        "3:Ironbow"
        "4:Rainbow"
        "5:Night"
        "6:Aurora"
        "7:Red Hot"
        "8:Jungle"
        "9:Medical"
        "10:Black Hot"
        "11:Golden Red Glory"
    )

    for entry in "${colormaps[@]}"; do
        IFS=':' read -r value name <<< "$entry"
        test_control "colormap" $value "$name"
        sleep 1  # Longer delay to see colormap change
    done

    # Reset to default (White Hot)
    test_control "colormap" 0 "Reset to White Hot"
    echo
}

# Test scene mode control
test_scene_mode() {
    print_header "Testing Scene Mode Control (0-9)"

    local modes=(
        "0:Low"
        "1:Linear Stretch"
        "2:Low Contrast"
        "3:General Mode"
        "4:High Contrast"
        "5:Highlight"
        "9:Outline Mode"
    )

    for entry in "${modes[@]}"; do
        IFS=':' read -r value name <<< "$entry"
        test_control "scene_mode" $value "$name"
        sleep 0.5
    done

    # Reset to default (General Mode)
    test_control "scene_mode" 3 "Reset to General Mode"
    echo
}

# Test zoom control
test_zoom() {
    print_header "Testing Zoom Control (1-8)"

    for value in 1 2 4 6 8; do
        test_control "zoom_absolute" $value "${value}x zoom"
        sleep 0.5
    done

    # Reset to default
    test_control "zoom_absolute" 1 "Reset to 1x"
    echo
}

# Test FFC trigger
test_ffc() {
    print_header "Testing FFC (Flat Field Correction) Trigger"

    print_info "FFC calibration takes ~1-2 seconds. Please wait..."
    test_control "ffc_trigger" 1 "Trigger shutter calibration"

    # Wait for FFC to complete
    sleep 2
    print_info "FFC complete"
    echo
}

# Test DDE (Digital Detail Enhancement)
test_dde() {
    print_header "Testing Digital Detail Enhancement (0-100)"

    for value in 0 50 100; do
        test_control "digital_detail_enhancement" $value "DDE at $(($value))%"
        sleep 0.5
    done

    # Reset to default
    test_control "digital_detail_enhancement" 50 "Reset to default"
    echo
}

# Test spatial noise reduction
test_spatial_nr() {
    print_header "Testing Spatial Noise Reduction (0-100)"

    for value in 0 50 100; do
        test_control "spatial_noise_reduction" $value "Spatial NR at $(($value))%"
        sleep 0.5
    done

    # Reset to default
    test_control "spatial_noise_reduction" 50 "Reset to default"
    echo
}

# Test temporal noise reduction
test_temporal_nr() {
    print_header "Testing Temporal Noise Reduction (0-100)"

    for value in 0 50 100; do
        test_control "temporal_noise_reduction" $value "Temporal NR at $(($value))%"
        sleep 0.5
    done

    # Reset to default
    test_control "temporal_noise_reduction" 50 "Reset to default"
    echo
}

# Test read-only controls
test_readonly_controls() {
    print_header "Testing Read-Only Controls"

    ((TESTS_RUN++))
    print_test "Reading pixel_rate"
    local pixel_rate=$(v4l2-ctl -d $SUBDEV -C pixel_rate 2>/dev/null | awk -F': ' '{print $2}')
    if [ -n "$pixel_rate" ]; then
        print_pass "Pixel rate: $pixel_rate Hz"
    else
        print_fail "Could not read pixel_rate"
    fi

    ((TESTS_RUN++))
    print_test "Reading link_freq"
    local link_freq=$(v4l2-ctl -d $SUBDEV -C link_freq 2>/dev/null | awk -F': ' '{print $2}')
    if [ -n "$link_freq" ]; then
        print_pass "Link frequency: $link_freq Hz"
    else
        print_fail "Could not read link_freq"
    fi

    echo
}

# Test invalid values (should fail gracefully)
test_boundary_conditions() {
    print_header "Testing Boundary Conditions"

    ((TESTS_RUN++))
    print_test "Testing brightness out of range (150)"
    if v4l2-ctl -d $SUBDEV -c brightness=150 2>&1 | grep -q "error\|invalid"; then
        print_pass "Correctly rejected out-of-range value"
    else
        print_fail "Did not reject out-of-range value"
    fi

    ((TESTS_RUN++))
    print_test "Testing colormap out of range (20)"
    if v4l2-ctl -d $SUBDEV -c colormap=20 2>&1 | grep -q "error\|invalid"; then
        print_pass "Correctly rejected out-of-range value"
    else
        print_fail "Did not reject out-of-range value"
    fi

    echo
}

# Monitor kernel messages during tests
monitor_kernel_messages() {
    print_header "Recent Kernel Messages"

    dmesg | grep rs300 | tail -20
    echo
}

# Test frame capture with different settings
test_capture() {
    print_header "Testing Video Capture"

    # IMPORTANT: Camera requires 2-second warm-up before valid thermal data
    # Capture 90 frames (3 seconds at 30fps) and extract frame 60+ after warm-up
    local full_capture="test_full_$(date +%Y%m%d_%H%M%S).yuv"
    local test_frame="test_frame_$(date +%Y%m%d_%H%M%S).yuv"

    ((TESTS_RUN++))
    print_test "Capturing 90 frames with warm-up (extracting frame 60)"
    print_info "Camera needs 2-second warm-up for valid thermal data"

    # Capture 90 frames (includes warm-up period)
    if v4l2-ctl -d $VIDEO_DEV --stream-mmap --stream-count=90 --stream-to=$full_capture 2>&1 | tee -a $LOG_FILE; then
        if [ -f "$full_capture" ] && [ -s "$full_capture" ]; then
            # Extract frame 60 (after 2-second warm-up) using dd
            # Frame size: 640x512x2 = 655360 bytes
            # Skip 59 frames (0-indexed), extract 1 frame
            dd if=$full_capture of=$test_frame bs=655360 count=1 skip=59 2>/dev/null

            if [ -f "$test_frame" ] && [ -s "$test_frame" ]; then
                local filesize=$(stat -c%s "$test_frame")
                print_pass "Extracted frame 60 successfully (${filesize} bytes)"

                # Calculate expected size for 640x512 UYVY (2 bytes per pixel)
                local expected_size=$((640 * 512 * 2))
                if [ $filesize -eq $expected_size ]; then
                    print_pass "Frame size matches expected 640x512 UYVY"
                else
                    print_info "Frame size: $filesize bytes (expected: $expected_size bytes)"
                fi
            else
                print_fail "Failed to extract frame 60"
            fi

            # Clean up temporary files
            rm -f $full_capture $test_frame
        else
            print_fail "Capture file is empty or missing"
        fi
    else
        print_fail "Failed to capture frames"
    fi

    echo
}

# List all controls
list_all_controls() {
    print_header "All Available Controls"

    v4l2-ctl -d $SUBDEV --list-ctrls-menus | tee -a $LOG_FILE
    echo
}

# Check I2C communication
test_i2c_communication() {
    print_header "Testing I2C Communication"

    if command -v i2cget &> /dev/null; then
        ((TESTS_RUN++))
        print_test "Reading status register (0x0200)"

        if status=$(sudo i2cget -y 10 0x3c 0x02 b 0x00 2>/dev/null); then
            print_pass "I2C communication successful (status: $status)"

            # Decode status byte
            local busy=$((status & 0x01))
            local failed=$(((status & 0x02) >> 1))
            local error_code=$(((status & 0xFC) >> 2))

            print_info "  Busy: $busy, Failed: $failed, Error Code: $error_code"
        else
            print_info "I2C direct access not available (may need root)"
        fi
    else
        print_info "i2c-tools not installed, skipping I2C tests"
    fi

    echo
}

# Generate summary report
print_summary() {
    print_header "Test Summary"

    echo "Tests Run:    $TESTS_RUN"
    echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
    else
        echo -e "${YELLOW}Some tests failed. Check $LOG_FILE for details${NC}"
    fi

    echo
    echo "Detailed log saved to: $LOG_FILE"
}

# Main test execution
main() {
    print_header "RS300 V4L2 Control Test Suite"
    echo "Started: $(date)"
    echo "Log file: $LOG_FILE"
    echo

    # Initialize log
    echo "RS300 Control Test Log - $(date)" > $LOG_FILE
    echo "===========================================" >> $LOG_FILE
    echo >> $LOG_FILE

    # Run all tests
    check_prerequisites
    list_all_controls
    test_readonly_controls
    test_brightness
    test_contrast
    test_colormap
    test_scene_mode
    test_zoom
    test_dde
    test_spatial_nr
    test_temporal_nr
    test_ffc  # Last because it takes longest
    test_boundary_conditions
    test_i2c_communication
    test_capture
    monitor_kernel_messages

    # Print summary
    print_summary
}

# Parse command line arguments
if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "RS300 V4L2 Control Test Suite"
    echo
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  --help, -h          Show this help message"
    echo "  --quick             Run quick test (subset of controls)"
    echo "  --control <name>    Test specific control only"
    echo
    echo "Examples:"
    echo "  $0                  # Run all tests"
    echo "  $0 --quick          # Quick sanity check"
    echo "  $0 --control brightness  # Test brightness only"
    echo
    exit 0
elif [ "$1" == "--quick" ]; then
    print_header "RS300 Quick Control Test"
    check_prerequisites
    test_brightness
    test_colormap
    test_zoom
    test_capture
    print_summary
elif [ "$1" == "--control" ] && [ -n "$2" ]; then
    print_header "Testing Control: $2"
    check_prerequisites

    case $2 in
        brightness) test_brightness ;;
        contrast) test_contrast ;;
        colormap) test_colormap ;;
        scene_mode) test_scene_mode ;;
        zoom) test_zoom ;;
        ffc) test_ffc ;;
        dde) test_dde ;;
        spatial_nr) test_spatial_nr ;;
        temporal_nr) test_temporal_nr ;;
        *) echo "Unknown control: $2"; exit 1 ;;
    esac

    print_summary
else
    main
fi
