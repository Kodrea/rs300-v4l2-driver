#!/bin/bash
#
# RS300 Thermal Camera Driver - Installation Script
# Version: 1.0.0
#
# Follows Linux package conventions:
# - Installation phase ONLY (no activation)
# - No automatic config.txt editing
# - No automatic service enablement
# - Clear instructions for manual activation
#
# Usage: ./setup.sh [--auto] [--help]
#

set -e

VERSION="1.0.0"
DRV_VERSION="0.0.1"
DRV_NAME="rs300"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

# Print functions
print_status() {
    echo -e "${BLUE}▶${NC} $1"
}

print_success() {
    echo -e "${GREEN}✅${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠️${NC} $1"
}

print_error() {
    echo -e "${RED}❌${NC} $1"
}

print_header() {
    echo -e "${BOLD}$1${NC}"
}

# Parse command line arguments
AUTO_MODE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --auto)
            AUTO_MODE=true
            shift
            ;;
        --help)
            cat <<EOF
RS300 Driver Installation Script

Usage: $0 [OPTIONS]

Options:
    --auto      Non-interactive mode (auto-install dependencies)
    --help      Show this help

This script installs:
  - RS300 kernel module via DKMS
  - Device tree overlay
  - rs300 CLI tool
  - Helper scripts (rs300-configure)
  - Systemd service (not enabled)
  - Udev rule (not enabled)

After installation:
  1. Enable camera: rs300 enable
  2. Reboot: sudo reboot
  3. Configure: rs300 configure
  4. Test: rs300 test

This follows Linux conventions - installation does NOT activate the camera.
Activation requires explicit user action via 'rs300 enable'.
EOF
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# ============================================================================
# PRE-FLIGHT CHECKS
# ============================================================================

print_header "RS300 Driver Installation v${VERSION}"
print_header "====================================="
echo ""

check_prerequisites() {
    print_status "Checking prerequisites..."
    echo ""

    local errors=0

    # Check if running on Raspberry Pi
    if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null; then
        print_warning "Not running on Raspberry Pi"
        print_warning "This driver is designed for Raspberry Pi 5"
        echo ""
    else
        local pi_model=$(grep "Model" /proc/cpuinfo | cut -d':' -f2 | xargs)
        echo "  Pi Model: $pi_model"

        if ! echo "$pi_model" | grep -q "Raspberry Pi 5"; then
            print_warning "Detected: $pi_model"
            print_warning "This driver is optimized for Raspberry Pi 5"
            print_warning "Pi 4 support may vary"
            echo ""
        fi
    fi

    # Check kernel version
    local kernel_version=$(uname -r)
    local kernel_major=$(echo "$kernel_version" | cut -d'.' -f1)
    local kernel_minor=$(echo "$kernel_version" | cut -d'.' -f2)

    echo "  Kernel: $kernel_version"

    if [ "$kernel_major" -lt 6 ] || ([ "$kernel_major" -eq 6 ] && [ "$kernel_minor" -lt 6 ]); then
        print_error "Kernel 6.6+ required, found $kernel_version"
        ((errors++))
    fi

    # Check architecture
    local arch=$(uname -m)
    echo "  Architecture: $arch"

    if [ "$arch" != "aarch64" ]; then
        print_warning "Expected aarch64, found $arch"
    fi

    # Check OS
    if [ -f /etc/os-release ]; then
        local os_name=$(grep PRETTY_NAME /etc/os-release | cut -d'"' -f2)
        echo "  OS: $os_name"
    fi

    echo ""

    if [ $errors -gt 0 ]; then
        print_error "Prerequisites check failed"
        exit 1
    fi

    print_success "Prerequisites check passed"
    echo ""
}

# ============================================================================
# DEPENDENCY INSTALLATION
# ============================================================================

install_dependencies() {
    print_status "Checking required packages..."
    echo ""

    local missing_pkgs=()

    # Check each required package
    for pkg in raspberrypi-kernel-headers dkms v4l-utils i2c-tools; do
        if ! dpkg -l 2>/dev/null | grep -q "^ii  $pkg "; then
            missing_pkgs+=($pkg)
        fi
    done

    if [ ${#missing_pkgs[@]} -eq 0 ]; then
        print_success "All required packages are installed"
        echo ""
        return 0
    fi

    echo "Missing packages:"
    for pkg in "${missing_pkgs[@]}"; do
        echo "  - $pkg"
    done
    echo ""

    local should_install=false

    if [ "$AUTO_MODE" = true ]; then
        should_install=true
        print_status "Auto mode: Installing packages..."
    else
        read -p "Install missing packages now? (Y/n): " -r
        echo
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            should_install=true
        fi
    fi

    if [ "$should_install" = true ]; then
        print_status "Updating package list..."
        sudo apt update

        print_status "Installing packages..."
        sudo apt install -y "${missing_pkgs[@]}"

        print_success "Dependencies installed"
        echo ""
    else
        print_error "Installation cannot continue without required packages"
        echo ""
        echo "Install manually:"
        echo "  sudo apt update"
        echo "  sudo apt install ${missing_pkgs[*]}"
        echo ""
        exit 1
    fi
}

# ============================================================================
# KERNEL MODULE INSTALLATION
# ============================================================================

install_kernel_module() {
    print_status "Installing kernel module via DKMS..."
    echo ""

    # Remove old version if exists
    if dkms status | grep -q "${DRV_NAME}"; then
        print_status "Removing previous installation..."
        sudo dkms remove -m ${DRV_NAME} -v ${DRV_VERSION} --all 2>/dev/null || true
    fi

    # Create source directory
    sudo mkdir -p /usr/src/${DRV_NAME}-${DRV_VERSION}

    # Copy source files
    print_status "Copying source files..."
    sudo cp dkms.conf Makefile rs300.c /usr/src/${DRV_NAME}-${DRV_VERSION}/

    # Add to DKMS
    print_status "Adding to DKMS..."
    sudo dkms add -m ${DRV_NAME} -v ${DRV_VERSION}

    # Build module
    print_status "Building kernel module..."
    sudo dkms build -m ${DRV_NAME} -v ${DRV_VERSION}

    # Install module
    print_status "Installing kernel module..."
    sudo dkms install -m ${DRV_NAME} -v ${DRV_VERSION}

    print_success "Kernel module installed via DKMS"
    echo ""
}

# ============================================================================
# DEVICE TREE OVERLAY INSTALLATION
# ============================================================================

install_device_tree() {
    print_status "Installing device tree overlay..."
    echo ""

    if [ ! -f rs300-overlay.dtbo ]; then
        print_error "Device tree overlay file not found: rs300-overlay.dtbo"
        exit 1
    fi

    sudo cp rs300-overlay.dtbo /boot/firmware/overlays/rs300.dtbo

    print_success "Device tree overlay installed"
    echo "  Location: /boot/firmware/overlays/rs300.dtbo"
    echo ""
}

# ============================================================================
# HELPER SCRIPTS INSTALLATION
# ============================================================================

install_helpers() {
    print_status "Installing helper scripts..."
    echo ""

    # Install rs300 CLI tool
    if [ ! -f rs300 ]; then
        print_error "rs300 CLI tool not found"
        exit 1
    fi

    sudo cp rs300 /usr/local/bin/rs300
    sudo chmod +x /usr/local/bin/rs300
    print_success "rs300 CLI tool installed"
    echo "  Location: /usr/local/bin/rs300"

    # Install rs300-configure script (renamed from configure_media.sh)
    if [ -f configure_media.sh ]; then
        sudo cp configure_media.sh /usr/local/bin/rs300-configure
        sudo chmod +x /usr/local/bin/rs300-configure
        print_success "rs300-configure script installed"
        echo "  Location: /usr/local/bin/rs300-configure"
    else
        print_warning "configure_media.sh not found (skipping)"
    fi

    echo ""
}

# ============================================================================
# SERVICE/UDEV INSTALLATION (not enabled)
# ============================================================================

install_services() {
    print_status "Installing systemd service and udev rule..."
    echo ""

    # Install systemd service
    if [ -f rs300-media-config.service ]; then
        sudo cp rs300-media-config.service /etc/systemd/system/

        # Update ExecStart path to use rs300 CLI
        sudo sed -i 's|ExecStart=.*|ExecStart=/usr/local/bin/rs300 configure --non-interactive|' \
            /etc/systemd/system/rs300-media-config.service

        # Update documentation URL
        sudo sed -i 's|Documentation=.*|Documentation=https://github.com/Kodrea/rs300-v4l2-driver|' \
            /etc/systemd/system/rs300-media-config.service

        sudo systemctl daemon-reload

        print_success "Systemd service installed (not enabled)"
        echo "  Service: rs300-media-config.service"
        echo "  Enable: Automatically via 'rs300 enable' command"
    else
        print_warning "Service file not found (skipping)"
    fi

    # Install udev rule
    if [ -f 99-rs300.rules ]; then
        sudo cp 99-rs300.rules /etc/udev/rules.d/

        # Update script paths to use /usr/local/bin
        sudo sed -i 's|/home/[^/]*/rs300-v4l2-driver/configure_media.sh|/usr/local/bin/rs300-configure|g' \
            /etc/udev/rules.d/99-rs300.rules

        sudo udevadm control --reload-rules

        print_success "Udev rule installed"
        echo "  Rule: /etc/udev/rules.d/99-rs300.rules"
    else
        print_warning "Udev rule file not found (skipping)"
    fi

    echo ""
}

# ============================================================================
# POST-INSTALLATION
# ============================================================================

show_next_steps() {
    echo ""
    print_header "════════════════════════════════════════════════════"
    print_success "  Installation Complete!"
    print_header "════════════════════════════════════════════════════"
    echo ""
    echo "The RS300 driver has been installed but NOT activated."
    echo "This follows Linux conventions - activation requires explicit user action."
    echo ""
    print_header "Next Steps to Activate Camera:"
    echo ""
    echo "1. Enable the camera:"
    echo "   ${BLUE}rs300 enable${NC}"
    echo ""
    echo "2. Reboot your Raspberry Pi:"
    echo "   ${BLUE}sudo reboot${NC}"
    echo ""
    echo "3. After reboot, configure media pipeline:"
    echo "   ${BLUE}rs300 configure${NC}"
    echo ""
    echo "4. Test the camera:"
    echo "   ${BLUE}rs300 test${NC}"
    echo ""
    print_header "Helpful Commands:"
    echo ""
    echo "  ${BLUE}rs300 status${NC}      # Check camera status anytime"
    echo "  ${BLUE}rs300 doctor${NC}      # Run diagnostics if issues occur"
    echo "  ${BLUE}rs300 help${NC}        # Show all available commands"
    echo ""
    echo "For issues: ${BLUE}rs300 logs${NC} (creates diagnostic archive)"
    echo "Report bugs: https://github.com/Kodrea/rs300-v4l2-driver/issues"
    echo ""
}

# ============================================================================
# MAIN INSTALLATION FLOW
# ============================================================================

main() {
    check_prerequisites
    install_dependencies
    install_kernel_module
    install_device_tree
    install_helpers
    install_services
    show_next_steps
}

# Run installation
main
