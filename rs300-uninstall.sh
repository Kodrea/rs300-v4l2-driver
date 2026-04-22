#!/bin/bash
#
# RS300 Thermal Camera Driver - Uninstall Script
# Version: 1.0.0
#
# Complete removal of RS300 driver, configuration, and related files
# Can be run interactively or with --auto flag for non-interactive mode
#

set -e

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

# Parse flags
AUTO_MODE=false
while [[ $# -gt 0 ]]; do
    case $1 in
        --auto)
            AUTO_MODE=true
            shift
            ;;
        --help)
            cat <<EOF
RS300 Driver Uninstall Script

Usage: $0 [OPTIONS]

Options:
    --auto      Non-interactive mode (skip all prompts)
    --help      Show this help

This script removes:
  - DKMS kernel module
  - Device tree overlay
  - Config.txt entries (optional)
  - Systemd service
  - Udev rules
  - CLI tools
  - User logs and configuration (optional)

Interactive mode will prompt before each removal.
Auto mode removes everything except logs (unless confirmed).
EOF
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Track what was removed
REMOVED_ITEMS=()
MANUAL_STEPS=()

# ============================================================================
# MAIN UNINSTALL PROCESS
# ============================================================================

print_header "RS300 Camera Driver Uninstall"
print_header "=============================="
echo ""

if [ "$AUTO_MODE" = false ]; then
    print_warning "This will remove the RS300 driver and related files"
    echo ""
    read -p "Continue with uninstall? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Uninstall cancelled"
        exit 0
    fi
    echo ""
fi

# ----------------------------------------------------------------------------
# 1. Remove DKMS module
# ----------------------------------------------------------------------------
print_status "Removing DKMS module..."

if dkms status | grep -q "rs300-dkms\|^rs300,"; then
    # Try the current package name first (installed by install.sh), then
    # fall back to the historical package name for pre-sync installations.
    if sudo dkms remove -m rs300-dkms -v 0.0.1 --all 2>/dev/null; then
        print_success "DKMS module removed (rs300-dkms)"
        REMOVED_ITEMS+=("DKMS module (rs300-dkms/0.0.1)")
    elif sudo dkms remove -m rs300 -v 0.0.1 --all 2>/dev/null; then
        print_success "DKMS module removed (rs300, pre-sync install)"
        REMOVED_ITEMS+=("DKMS module (rs300/0.0.1)")
    else
        print_warning "Failed to remove DKMS module (may not be installed)"
    fi
else
    print_status "DKMS module not found (already removed or not installed)"
fi

# Remove source directory (check both current and pre-sync paths)
for src_dir in /usr/src/rs300-dkms-0.0.1 /usr/src/rs300-0.0.1; do
    if [ -d "$src_dir" ]; then
        sudo rm -rf "$src_dir"
        print_success "DKMS source directory removed: $src_dir"
        REMOVED_ITEMS+=("DKMS source directory ($src_dir)")
    fi
done

# Unload module if loaded
if lsmod | grep -q rs300; then
    if sudo rmmod rs300 2>/dev/null; then
        print_success "Kernel module unloaded"
    else
        print_warning "Failed to unload kernel module (may be in use)"
        MANUAL_STEPS+=("Reboot system to fully unload kernel module")
    fi
fi

echo ""

# ----------------------------------------------------------------------------
# 2. Remove device tree overlay
# ----------------------------------------------------------------------------
print_status "Removing device tree overlay..."

if [ -f /boot/firmware/overlays/rs300.dtbo ]; then
    sudo rm -f /boot/firmware/overlays/rs300.dtbo
    print_success "Device tree overlay removed"
    REMOVED_ITEMS+=("Device tree overlay (/boot/firmware/overlays/rs300.dtbo)")
else
    print_status "Device tree overlay not found (already removed)"
fi

echo ""

# ----------------------------------------------------------------------------
# 3. Remove config.txt entries
# ----------------------------------------------------------------------------
print_status "Checking boot configuration..."

if grep -q "dtoverlay=rs300\|RS300 Thermal Camera" /boot/firmware/config.txt 2>/dev/null; then
    SHOULD_REMOVE_CONFIG=false

    if [ "$AUTO_MODE" = true ]; then
        SHOULD_REMOVE_CONFIG=true
    else
        echo ""
        echo "Found RS300 entries in /boot/firmware/config.txt"
        read -p "Remove RS300 from config.txt? (y/N): " -n 1 -r
        echo
        [[ $REPLY =~ ^[Yy]$ ]] && SHOULD_REMOVE_CONFIG=true
    fi

    if [ "$SHOULD_REMOVE_CONFIG" = true ]; then
        # Create backup
        BACKUP="/boot/firmware/config.txt.uninstall-backup-$(date +%Y%m%d-%H%M%S)"
        sudo cp /boot/firmware/config.txt "$BACKUP"
        print_success "Created backup: $BACKUP"

        # Remove RS300 entries
        sudo sed -i '/# RS300 Thermal Camera/d' /boot/firmware/config.txt
        sudo sed -i '/dtoverlay=rs300/d' /boot/firmware/config.txt
        sudo sed -i '/camera_auto_detect=0/d' /boot/firmware/config.txt

        print_success "Removed RS300 from config.txt"
        REMOVED_ITEMS+=("Config.txt entries (backup: $BACKUP)")
        MANUAL_STEPS+=("Reboot required for boot config changes to take effect")
    else
        print_status "Skipping config.txt removal (manual cleanup required)"
        MANUAL_STEPS+=("Manually remove RS300 entries from /boot/firmware/config.txt if desired")
    fi
else
    print_status "No RS300 entries found in config.txt"
fi

echo ""

# ----------------------------------------------------------------------------
# 4. Remove systemd service
# ----------------------------------------------------------------------------
print_status "Removing systemd service..."

if systemctl is-enabled rs300-media-config.service >/dev/null 2>&1; then
    sudo systemctl disable rs300-media-config.service 2>/dev/null || true
    print_success "Systemd service disabled"
fi

if systemctl is-active rs300-media-config.service >/dev/null 2>&1; then
    sudo systemctl stop rs300-media-config.service 2>/dev/null || true
    print_success "Systemd service stopped"
fi

if [ -f /etc/systemd/system/rs300-media-config.service ]; then
    sudo rm -f /etc/systemd/system/rs300-media-config.service
    sudo systemctl daemon-reload
    print_success "Systemd service removed"
    REMOVED_ITEMS+=("Systemd service (rs300-media-config.service)")
else
    print_status "Systemd service not found (already removed)"
fi

echo ""

# ----------------------------------------------------------------------------
# 5. Remove udev rule
# ----------------------------------------------------------------------------
print_status "Removing udev rule..."

if [ -f /etc/udev/rules.d/99-rs300.rules ]; then
    sudo rm -f /etc/udev/rules.d/99-rs300.rules
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    print_success "Udev rule removed"
    REMOVED_ITEMS+=("Udev rule (/etc/udev/rules.d/99-rs300.rules)")
else
    print_status "Udev rule not found (already removed)"
fi

echo ""

# ----------------------------------------------------------------------------
# 6. Remove CLI tools
# ----------------------------------------------------------------------------
print_status "Removing CLI tools..."

if [ -f /usr/local/bin/rs300 ]; then
    sudo rm -f /usr/local/bin/rs300
    print_success "rs300 CLI tool removed"
    REMOVED_ITEMS+=("rs300 CLI tool (/usr/local/bin/rs300)")
else
    print_status "rs300 CLI tool not found (already removed)"
fi

if [ -f /usr/local/bin/rs300-configure ]; then
    sudo rm -f /usr/local/bin/rs300-configure
    print_success "rs300-configure script removed"
    REMOVED_ITEMS+=("rs300-configure script (/usr/local/bin/rs300-configure)")
else
    print_status "rs300-configure script not found (already removed)"
fi

echo ""

# ----------------------------------------------------------------------------
# 7. Remove user logs and configuration
# ----------------------------------------------------------------------------
print_status "Checking user logs and configuration..."

RS300_DIR="${HOME}/.rs300"

if [ -d "$RS300_DIR" ]; then
    # Show what's in there
    LOCAL_SIZE=$(du -sh "$RS300_DIR" 2>/dev/null | cut -f1)
    LOG_COUNT=$(find "$RS300_DIR/logs" -type f 2>/dev/null | wc -l)

    echo ""
    echo "Found RS300 user directory: $RS300_DIR"
    echo "  Size: $LOCAL_SIZE"
    echo "  Logs: $LOG_COUNT files"
    echo ""

    SHOULD_REMOVE_LOGS=false

    if [ "$AUTO_MODE" = true ]; then
        # In auto mode, preserve logs by default
        print_status "Auto mode: Preserving user logs"
    else
        read -p "Remove logs and configuration? (y/N): " -n 1 -r
        echo
        [[ $REPLY =~ ^[Yy]$ ]] && SHOULD_REMOVE_LOGS=true
    fi

    if [ "$SHOULD_REMOVE_LOGS" = true ]; then
        rm -rf "$RS300_DIR"
        print_success "User logs and configuration removed"
        REMOVED_ITEMS+=("User directory ($RS300_DIR)")
    else
        print_status "Preserving user logs: $RS300_DIR"
        echo "  To remove manually: rm -rf $RS300_DIR"
    fi
else
    print_status "No user directory found"
fi

echo ""

# ============================================================================
# SUMMARY
# ============================================================================

print_header "========================================"
print_header "Uninstall Summary"
print_header "========================================"
echo ""

if [ ${#REMOVED_ITEMS[@]} -gt 0 ]; then
    print_success "Removed items:"
    for item in "${REMOVED_ITEMS[@]}"; do
        echo "  ✓ $item"
    done
    echo ""
fi

if [ ${#MANUAL_STEPS[@]} -gt 0 ]; then
    print_warning "Manual steps required:"
    for step in "${MANUAL_STEPS[@]}"; do
        echo "  • $step"
    done
    echo ""
fi

# Final status
if [ ${#REMOVED_ITEMS[@]} -eq 0 ]; then
    print_warning "No items were removed (already uninstalled or not found)"
else
    print_success "Uninstall complete!"

    if grep -q "Reboot" <<< "${MANUAL_STEPS[*]}"; then
        echo ""
        print_warning "⚠️  Reboot recommended: sudo reboot"
    fi
fi

echo ""
echo "To reinstall: sudo ./install.sh"
echo ""
