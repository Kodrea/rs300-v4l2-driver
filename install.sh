#!/bin/bash
# RS300 Thermal Camera Driver - Automated Installation Script
# Usage: curl -sSL https://raw.githubusercontent.com/Kodrea/rs300-v4l2-driver/pi5-testing/install.sh | bash
# Or: ./install.sh

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Banner
echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════╗"
echo "║   RS300 Thermal Camera Driver - Automated Installer  ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    log_error "Please do not run this script as root (no sudo)"
    log_info "The script will prompt for sudo when needed"
    exit 1
fi

# Detect platform
log_info "Detecting platform..."

# Check if Raspberry Pi
if [ ! -f /proc/device-tree/model ]; then
    log_error "Not a Raspberry Pi - /proc/device-tree/model not found"
    exit 1
fi

MODEL=$(tr -d '\0' < /proc/device-tree/model)
log_info "Detected: $MODEL"

# Determine Pi version
if echo "$MODEL" | grep -q "Raspberry Pi 5"; then
    PLATFORM="pi5"
    log_success "Platform: Raspberry Pi 5"
elif echo "$MODEL" | grep -q "Raspberry Pi 4"; then
    PLATFORM="pi4"
    log_success "Platform: Raspberry Pi 4"
elif echo "$MODEL" | grep -q "Raspberry Pi Zero 2"; then
    log_error "Platform: Raspberry Pi Zero 2W - NOT SUPPORTED"
    log_warning "Power limitations prevent reliable operation"
    log_info "See: docs/hardware/compatibility.md for details"
    exit 1
else
    log_warning "Platform: Unknown Raspberry Pi variant"
    log_warning "This script is designed for Pi 5 or Pi 4B"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
    PLATFORM="unknown"
fi

# Check OS version
log_info "Checking OS version..."
if [ -f /etc/os-release ]; then
    . /etc/os-release
    log_info "OS: $PRETTY_NAME"
    if [[ "$VERSION_CODENAME" != "bookworm" ]]; then
        log_warning "This script is tested on Bookworm"
        log_warning "You are running: $VERSION_CODENAME"
        read -p "Continue anyway? (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
else
    log_warning "Cannot detect OS version"
fi

# Check kernel version
KERNEL=$(uname -r)
log_info "Kernel: $KERNEL"

# Check internet connectivity
log_info "Checking internet connection..."
if ! ping -c 1 google.com &> /dev/null; then
    log_error "No internet connection detected"
    log_info "Internet is required to install dependencies"
    exit 1
fi
log_success "Internet connection OK"

# Check if already in repository directory
if [ -f "./setup.sh" ] && [ -f "./rs300.c" ]; then
    log_info "Already in rs300-v4l2-driver directory"
    REPO_DIR="$(pwd)"
    IN_REPO=true
else
    log_info "Not in repository directory, will clone"
    IN_REPO=false
    REPO_DIR="$HOME/rs300-v4l2-driver"
fi

# Install dependencies
log_info "Installing dependencies..."
log_info "This may take a few minutes..."

sudo apt update || { log_error "apt update failed"; exit 1; }

PACKAGES="raspberrypi-kernel-headers dkms git v4l-utils"
log_info "Installing: $PACKAGES"

sudo apt install -y $PACKAGES || { log_error "Failed to install dependencies"; exit 1; }

log_success "Dependencies installed"

# Clone repository if needed
if [ "$IN_REPO" = false ]; then
    log_info "Cloning RS300 driver repository..."

    # Check if directory already exists
    if [ -d "$REPO_DIR" ]; then
        log_warning "Directory $REPO_DIR already exists"
        read -p "Remove and re-clone? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$REPO_DIR"
        else
            log_info "Using existing directory"
            cd "$REPO_DIR"
        fi
    fi

    if [ ! -d "$REPO_DIR" ]; then
        # Clone appropriate branch
        if [ "$PLATFORM" = "pi5" ]; then
            git clone -b pi5-testing https://github.com/Kodrea/rs300-v4l2-driver.git "$REPO_DIR" || \
                { log_error "Failed to clone repository"; exit 1; }
        else
            git clone https://github.com/Kodrea/rs300-v4l2-driver.git "$REPO_DIR" || \
                { log_error "Failed to clone repository"; exit 1; }
        fi
        cd "$REPO_DIR"
        log_success "Repository cloned to $REPO_DIR"
    fi
else
    log_info "Using repository at: $REPO_DIR"
fi

# Pi 4 specific: Warn about driver modification
if [ "$PLATFORM" = "pi4" ]; then
    log_warning "IMPORTANT: Pi 4 requires driver configuration before building"
    echo ""
    log_info "You need to edit rs300.c to match your module resolution:"
    echo "  - 640×512: mode = 0"
    echo "  - 256×192: mode = 1"
    echo "  - 384×288: mode = 2"
    echo ""
    log_info "See: docs/getting-started/installation.md for details"
    echo ""
    read -p "Have you configured rs300.c for your module? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_warning "Please edit rs300.c before continuing"
        log_info "Run: nano $REPO_DIR/rs300.c"
        log_info "Then run: $REPO_DIR/install.sh again"
        exit 0
    fi
fi

# Run setup.sh
log_info "Running setup script..."
chmod +x ./setup.sh || { log_error "Failed to make setup.sh executable"; exit 1; }

./setup.sh || { log_error "setup.sh failed"; exit 1; }

log_success "Driver built and installed via DKMS"

# Configure device tree overlay
log_info "Checking device tree configuration..."

# Determine config.txt location
if [ -f "/boot/firmware/config.txt" ]; then
    CONFIG_TXT="/boot/firmware/config.txt"
elif [ -f "/boot/config.txt" ]; then
    CONFIG_TXT="/boot/config.txt"
else
    log_error "Cannot find config.txt"
    exit 1
fi

log_info "Config file: $CONFIG_TXT"

# Check if already configured
if grep -q "dtoverlay=rs300" "$CONFIG_TXT"; then
    log_success "Device tree overlay already configured"
else
    log_warning "Device tree overlay not configured"
    log_info "Need to add to $CONFIG_TXT:"
    echo "  camera_auto_detect=0"
    echo "  dtoverlay=rs300"
    echo ""
    read -p "Add automatically? (Y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        # Backup config.txt
        sudo cp "$CONFIG_TXT" "${CONFIG_TXT}.backup.$(date +%Y%m%d_%H%M%S)"
        log_success "Backed up config.txt"

        # Add configuration
        echo "" | sudo tee -a "$CONFIG_TXT" > /dev/null
        echo "# RS300 Thermal Camera Driver" | sudo tee -a "$CONFIG_TXT" > /dev/null
        echo "camera_auto_detect=0" | sudo tee -a "$CONFIG_TXT" > /dev/null
        echo "dtoverlay=rs300" | sudo tee -a "$CONFIG_TXT" > /dev/null

        log_success "Configuration added to $CONFIG_TXT"
    else
        log_warning "You will need to manually add to $CONFIG_TXT:"
        echo "  camera_auto_detect=0"
        echo "  dtoverlay=rs300"
    fi
fi

# Installation complete
echo ""
echo -e "${GREEN}╔═══════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║          Installation Complete!                       ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════════════════════╝${NC}"
echo ""

log_info "Next steps:"
echo ""
echo "  1. Reboot your Raspberry Pi:"
echo "     ${BLUE}sudo reboot${NC}"
echo ""

if [ "$PLATFORM" = "pi5" ]; then
    echo "  2. After reboot, configure media pipeline:"
    echo "     ${BLUE}cd $REPO_DIR${NC}"
    echo "     ${BLUE}./configure_media.sh${NC}"
    echo ""
    echo "  3. Test streaming:"
    echo "     ${BLUE}ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0${NC}"
else  # Pi 4
    echo "  2. After reboot, set video format:"
    echo "     ${BLUE}v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV${NC}"
    echo "     (adjust resolution to match your module)"
    echo ""
    echo "  3. Test streaming:"
    echo "     ${BLUE}ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0${NC}"
fi

echo ""
log_info "Documentation:"
echo "  - Quick start:     $REPO_DIR/docs/getting-started/first-capture.md"
echo "  - Full docs:       $REPO_DIR/docs/README.md"
echo "  - Troubleshooting: $REPO_DIR/TROUBLESHOOTING.md"
echo "  - Video tutorials: https://linktr.ee/kodrea"
echo ""

# Prompt for reboot
read -p "Reboot now? (Y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Nn]$ ]]; then
    log_info "Rebooting..."
    sudo reboot
else
    log_info "Remember to reboot before using the camera!"
fi
