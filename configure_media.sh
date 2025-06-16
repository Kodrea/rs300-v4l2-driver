#!/bin/bash

# RS300 Media Controller Configuration Script
# Step-by-step pipeline setup with validation and rollback

echo "=== RS300 Media Controller Configuration ==="

# Configuration options
DEFAULT_FORMAT="UYVY8_1X16"
DEFAULT_PIXELFORMAT="UYVY"
DEFAULT_WIDTH=640
DEFAULT_HEIGHT=512

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

# Function to validate media controller operation
validate_media_operation() {
    local operation="$1"
    local device="$2"
    local command="$3"
    
    print_step "Executing: $operation"
    
    if eval "$command" 2>/dev/null; then
        print_success "$operation completed"
        return 0
    else
        print_error "$operation failed"
        echo "  Command: $command"
        return 1
    fi
}

# Function to backup current configuration
backup_configuration() {
    local media_dev="$1"
    local backup_file="media_config_backup_$(date +%Y%m%d_%H%M%S).txt"
    
    print_step "Backing up current configuration"
    
    {
        echo "# Media Controller Backup - $(date)"
        echo "# Device: $media_dev"
        echo ""
        echo "=== Topology ==="
        media-ctl -d "$media_dev" --print-topology 2>/dev/null
        echo ""
        echo "=== Current Formats ==="
        media-ctl -d "$media_dev" --get-v4l2 "'rs300 10-003c':0" 2>/dev/null
        media-ctl -d "$media_dev" --get-v4l2 "'csi2':0" 2>/dev/null
        media-ctl -d "$media_dev" --get-v4l2 "'csi2':4" 2>/dev/null
        echo ""
        echo "=== Video Device Format ==="
        v4l2-ctl -d /dev/video0 --get-fmt-video 2>/dev/null
    } > "$backup_file"
    
    if [ -f "$backup_file" ]; then
        print_success "Configuration backed up to $backup_file"
        echo "$backup_file"
    else
        print_warning "Backup failed, continuing anyway"
        echo ""
    fi
}

# Function to restore configuration
restore_configuration() {
    local media_dev="$1"
    
    print_step "Resetting media controller links"
    media-ctl -d "$media_dev" -r >/dev/null 2>&1
    print_success "Media controller reset"
}

# Parse command line arguments
INTERACTIVE=true
VISUALIZE=false
FORMAT="$DEFAULT_FORMAT"
PIXELFORMAT="$DEFAULT_PIXELFORMAT"
WIDTH="$DEFAULT_WIDTH"
HEIGHT="$DEFAULT_HEIGHT"

while [[ $# -gt 0 ]]; do
    case $1 in
        --format)
            FORMAT="$2"
            shift 2
            ;;
        --pixelformat)
            PIXELFORMAT="$2"
            shift 2
            ;;
        --width)
            WIDTH="$2"
            shift 2
            ;;
        --height)
            HEIGHT="$2"
            shift 2
            ;;
        --non-interactive)
            INTERACTIVE=false
            shift
            ;;
        --visualize)
            VISUALIZE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --format FORMAT        Target format (default: $DEFAULT_FORMAT, auto-detected from RS300)"
            echo "  --pixelformat FORMAT   V4L2 pixel format (default: $DEFAULT_PIXELFORMAT)"
            echo "  --width WIDTH          Target width (default: $DEFAULT_WIDTH, auto-detected from RS300)"
            echo "  --height HEIGHT        Target height (default: $DEFAULT_HEIGHT, auto-detected from RS300)"
            echo "  --non-interactive      Run without user prompts"
            echo "  --visualize            Generate PNG visualization of pipeline"
            echo "  --help                 Show this help"
            echo ""
            echo "Note: RS300 camera format and resolution are read-only (set by driver module parameters)."
            echo "      The script will auto-detect and use the RS300's current format for pipeline configuration."
            echo ""
            echo "Pi 5 Compatible formats (16-bit packed only):"
            echo "  Mediabus: UYVY8_1X16 (default), YUYV8_1X16, YVYU8_1X16, VYUY8_1X16"
            echo "  Pixel:    UYVY (default), YUYV, YVYU, VYUY"
            echo ""
            echo "WARNING: 8-bit dual lane formats (*8_2X8) are NOT supported on Pi 5 and will cause format mismatch errors!"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "Configuration parameters:"
echo "  Format: $FORMAT"
echo "  Pixel Format: $PIXELFORMAT"
echo "  Resolution: ${WIDTH}x${HEIGHT}"
echo ""

# Validate Pi 5 format compatibility
if [[ "$FORMAT" == *"2X8"* ]]; then
    print_error "Format '$FORMAT' is not supported on Raspberry Pi 5"
    print_error "Pi 5 RP1-CFE only supports 16-bit packed formats (*8_1X16)"
    echo ""
    echo "Supported formats:"
    echo "  UYVY8_1X16, YUYV8_1X16, YVYU8_1X16, VYUY8_1X16"
    echo ""
    echo "Use --format UYVY8_1X16 or similar 1X16 format"
    exit 1
fi

# Check prerequisites
print_step "Checking prerequisites"

if ! lsmod | grep -q rs300; then
    print_error "RS300 driver not loaded"
    echo "Run: ./setup.sh && sudo reboot"
    exit 1
fi
print_success "RS300 driver loaded"

if ! command -v media-ctl >/dev/null 2>&1; then
    print_error "media-ctl not found"
    echo "Install: sudo apt install v4l-utils"
    exit 1
fi
print_success "media-ctl available"

# Find RS300 media controller device
print_step "Locating RS300 media controller device"

print_step "Available media devices:"
for i in {0..5}; do
    if [ -e "/dev/media$i" ]; then
        DEVICE_INFO=$(media-ctl -d "/dev/media$i" --info 2>/dev/null | grep -E "(driver|model)" | head -2 | paste -sd ' ' | tr -s ' ')
        echo "  /dev/media$i: $DEVICE_INFO"
    fi
done

MEDIA_DEV=""
for i in {0..5}; do
    if [ -e "/dev/media$i" ] && media-ctl -d "/dev/media$i" --print-topology 2>/dev/null | grep -q "rs300 10-003c"; then
        MEDIA_DEV="/dev/media$i"
        print_success "Found RS300 on $MEDIA_DEV"
        
        # Verify the topology structure we expect
        if media-ctl -d "$MEDIA_DEV" --print-topology 2>/dev/null | grep -q "rp1-cfe-csi2_ch0"; then
            print_success "Pipeline structure validated: RS300 → CSI-2 → rp1-cfe-csi2_ch0 → /dev/video0"
        else
            print_warning "Expected pipeline structure not found in $MEDIA_DEV"
        fi
        break
    fi
done

if [ -z "$MEDIA_DEV" ]; then
    print_error "RS300 not found in any media controller device"
    echo ""
    echo "Available entities in media controllers:"
    for i in {0..5}; do
        if [ -e "/dev/media$i" ]; then
            echo "  /dev/media$i:"
            media-ctl -d "/dev/media$i" --print-topology 2>/dev/null | grep -E "entity.*:" | head -5 | sed 's/^/    /'
        fi
    done
    exit 1
fi

# Interactive confirmation
if [ "$INTERACTIVE" = true ]; then
    echo ""
    echo "About to configure media pipeline:"
    echo "  Device: $MEDIA_DEV"
    echo "  Format: $FORMAT -> $PIXELFORMAT"
    echo "  Resolution: ${WIDTH}x${HEIGHT}"
    echo ""
    read -p "Continue? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Configuration cancelled"
        exit 0
    fi
fi

# Backup current configuration
BACKUP_FILE=$(backup_configuration "$MEDIA_DEV")

# Configuration steps with rollback capability
ROLLBACK_NEEDED=false

trap 'if [ "$ROLLBACK_NEEDED" = true ]; then echo ""; print_warning "Error occurred, performing rollback"; restore_configuration "$MEDIA_DEV"; fi' EXIT

echo ""
print_step "=== Starting Media Pipeline Configuration ==="

# Step 1: Reset media controller
print_step "Step 1: Resetting media controller"
if media-ctl -d "$MEDIA_DEV" -r >/dev/null 2>&1; then
    print_success "Media controller reset"
else
    print_warning "Media controller reset failed, continuing"
fi

# Step 2: Enable critical link
print_step "Step 2: Enabling CSI2 to video link"
ROLLBACK_NEEDED=true
if validate_media_operation "Enable media link" "$MEDIA_DEV" "media-ctl -d '$MEDIA_DEV' -l \"'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]\""; then
    # Verify link is enabled - check for the specific enabled link
    if media-ctl -d "$MEDIA_DEV" --print-topology | grep -q "rp1-cfe-csi2_ch0.*ENABLED"; then
        print_success "Link verification passed"
    else
        print_error "Link enabled but verification failed"
        echo "Expected: rp1-cfe-csi2_ch0 link with ENABLED status"
        echo "Actual link status around pad4:"
        media-ctl -d "$MEDIA_DEV" --print-topology | grep -A3 -B1 "pad4:" || echo "  Pad4 not found"
        echo "All rp1-cfe-csi2_ch0 links:"
        media-ctl -d "$MEDIA_DEV" --print-topology | grep "rp1-cfe-csi2_ch0" || echo "  No rp1-cfe-csi2_ch0 links found"
        exit 1
    fi
else
    print_error "Critical link configuration failed"
    exit 1
fi

# Step 3: Read RS300 current format (RS300 format is read-only, set by driver)
print_step "Step 3: Reading RS300 current format"
RS300_ACTUAL_FORMAT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'rs300 10-003c':0" 2>/dev/null)
if [ $? -eq 0 ] && [ -n "$RS300_ACTUAL_FORMAT" ]; then
    print_success "RS300 current format: $RS300_ACTUAL_FORMAT"
    
    # Extract the actual format code from RS300 for downstream configuration
    RS300_FORMAT_CODE=$(echo "$RS300_ACTUAL_FORMAT" | grep -o 'fmt:[^/\s]*' | cut -d: -f2 | tr -d '\n\r\t ')
    RS300_RESOLUTION=$(echo "$RS300_ACTUAL_FORMAT" | grep -oE '[0-9]+x[0-9]+' | head -1 | tr -d '\n\r\t ')
    
    if [ -n "$RS300_FORMAT_CODE" ] && [ -n "$RS300_RESOLUTION" ]; then
        print_success "Detected RS300 format: $RS300_FORMAT_CODE at $RS300_RESOLUTION"
        # Use RS300's actual format for downstream configuration instead of user-specified format
        FORMAT="$RS300_FORMAT_CODE"
        if [ "$RS300_RESOLUTION" != "${WIDTH}x${HEIGHT}" ]; then
            print_warning "RS300 resolution ($RS300_RESOLUTION) differs from requested (${WIDTH}x${HEIGHT})"
            print_warning "Using RS300's native resolution: $RS300_RESOLUTION"
            WIDTH=$(echo "$RS300_RESOLUTION" | cut -dx -f1 | tr -d '\n\r\t ')
            HEIGHT=$(echo "$RS300_RESOLUTION" | cut -dx -f2 | tr -d '\n\r\t ')
        fi
    else
        print_warning "Could not parse RS300 format details, using requested format"
    fi
else
    print_error "Failed to read RS300 format"
    exit 1
fi

# Step 4: Configure CSI2 input format (match RS300 output)
print_step "Step 4: Setting CSI2 input format to match RS300"
if validate_media_operation "Set CSI2 input format" "$MEDIA_DEV" "media-ctl -d '$MEDIA_DEV' -V \"'csi2':0 [fmt:$FORMAT/${WIDTH}x${HEIGHT} field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]\""; then
    ACTUAL_FORMAT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'csi2':0" 2>/dev/null)
    print_success "CSI2 input format: $ACTUAL_FORMAT"
else
    print_error "CSI2 input format configuration failed"
    exit 1
fi

# Step 5: Configure CSI2 output format
print_step "Step 5: Setting CSI2 output format"
if validate_media_operation "Set CSI2 output format" "$MEDIA_DEV" "media-ctl -d '$MEDIA_DEV' -V \"'csi2':4 [fmt:$FORMAT/${WIDTH}x${HEIGHT} field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]\""; then
    ACTUAL_FORMAT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'csi2':4" 2>/dev/null)
    print_success "CSI2 output format: $ACTUAL_FORMAT"
else
    print_error "CSI2 output format configuration failed"
    exit 1
fi

# Step 5.5: Configure CSI2 metadata format (pad 1)
print_step "Step 5.5: Setting CSI2 metadata format"
if validate_media_operation "Set CSI2 metadata format" "$MEDIA_DEV" "media-ctl -d '$MEDIA_DEV' -V \"'csi2':1 [fmt:$FORMAT/16384x1 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]\""; then
    METADATA_FORMAT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'csi2':1" 2>/dev/null)
    print_success "CSI2 metadata format: $METADATA_FORMAT"
else
    print_warning "CSI2 metadata format configuration failed, continuing anyway"
fi

# Step 6: Configure video device format
print_step "Step 6: Setting video device format"
if [ -e "/dev/video0" ]; then
    if validate_media_operation "Set video device format" "/dev/video0" "v4l2-ctl -d /dev/video0 --set-fmt-video=width=$WIDTH,height=$HEIGHT,pixelformat=$PIXELFORMAT,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range"; then
        VIDEO_FORMAT=$(v4l2-ctl -d /dev/video0 --get-fmt-video 2>/dev/null)
        print_success "Video device format: $VIDEO_FORMAT"
    else
        print_error "Video device format configuration failed"
        exit 1
    fi
else
    print_error "/dev/video0 not available"
    exit 1
fi

# Step 7: Final validation
print_step "Step 7: Final pipeline validation"

echo ""
print_step "=== Pipeline Status Summary ==="

# Check all formats match
RS300_FMT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'rs300 10-003c':0" 2>/dev/null)
CSI2_IN_FMT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'csi2':0" 2>/dev/null)
CSI2_OUT_FMT=$(media-ctl -d "$MEDIA_DEV" --get-v4l2 "'csi2':4" 2>/dev/null)
VIDEO_FMT=$(v4l2-ctl -d /dev/video0 --get-fmt-video 2>/dev/null)

echo "Pipeline formats:"
echo "  RS300:     $RS300_FMT"
echo "  CSI2 IN:   $CSI2_IN_FMT"
echo "  CSI2 OUT:  $CSI2_OUT_FMT"
echo "  VIDEO:     $(echo $VIDEO_FMT | grep -o 'Width/Height.*Pixel Format.*')"

# Check link status
if media-ctl -d "$MEDIA_DEV" --print-topology | grep -q "rp1-cfe-csi2_ch0.*ENABLED"; then
    print_success "Critical pipeline link is ENABLED"
else
    print_error "Critical pipeline link is NOT enabled"
    exit 1
fi

# Test basic streaming capability
print_step "Testing basic streaming capability"
dmesg -C  # Clear previous messages

timeout 3s v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=/dev/null >/dev/null 2>&1
STREAM_RESULT=$?

if [ $STREAM_RESULT -eq 0 ]; then
    print_success "🎉 STREAMING WORKS! Pipeline successfully configured"
    ROLLBACK_NEEDED=false
elif [ $STREAM_RESULT -eq 124 ]; then
    print_warning "Streaming timeout (may still work with longer timeout)"
    ROLLBACK_NEEDED=false
else
    print_error "Streaming failed (exit code: $STREAM_RESULT)"
    # Check for specific error messages
    ERRORS=$(dmesg | grep -E "(error|Error|failed|invalid)" | tail -3)
    if [ -n "$ERRORS" ]; then
        echo "Recent errors:"
        echo "$ERRORS" | sed 's/^/  /'
    fi
fi

echo ""
print_step "=== Configuration Complete ==="

if [ "$ROLLBACK_NEEDED" = false ]; then
    print_success "Media pipeline successfully configured!"
    echo ""
    echo "Configuration saved. To reproduce this setup:"
    echo "  $0 --format $FORMAT --pixelformat $PIXELFORMAT --width $WIDTH --height $HEIGHT --non-interactive"
    echo ""
    echo "Next steps:"
    echo "1. Test streaming: v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10"
    echo "2. Test controls: ./test_thermal.sh"
    echo "3. Save working config for future use"
    
    # Save working configuration
    WORKING_CONFIG="working_config_$(date +%Y%m%d_%H%M%S).sh"
    cat > "$WORKING_CONFIG" << EOF
#!/bin/bash
# Working RS300 configuration - Generated $(date)
# This script reproduces a working media pipeline configuration

$0 --format $FORMAT --pixelformat $PIXELFORMAT --width $WIDTH --height $HEIGHT --non-interactive
EOF
    chmod +x "$WORKING_CONFIG"
    print_success "Working configuration saved to $WORKING_CONFIG"
    
    # Generate visualization if requested
    if [ "$VISUALIZE" = true ]; then
        print_step "Generating pipeline visualization"
        
        if [ -f "./media-topology-visualizer.py" ]; then
            VISUAL_OUTPUT="pipeline_configured_$(date +%Y%m%d_%H%M%S).png"
            if python3 ./media-topology-visualizer.py --device "$MEDIA_DEV" --output "$VISUAL_OUTPUT" --check-formats; then
                print_success "Pipeline visualization saved to $VISUAL_OUTPUT"
            else
                print_warning "Visualization generation failed"
            fi
        else
            print_warning "Visualizer script not found - skipping visualization"
        fi
    fi
    
else
    print_error "Configuration failed - check error messages above"
fi

# Disable rollback trap
trap - EXIT