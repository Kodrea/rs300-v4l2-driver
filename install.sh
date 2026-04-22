#!/bin/bash
#
# RS300 Thermal Camera — Raspberry Pi 4 / CM4 / Zero 2 W Install Script
#
# Installs the RS300 driver via DKMS, compiles and installs the DT
# overlay, and wires /boot/firmware/config.txt so the overlay loads at
# boot. Boot-time overlay load is required on Pi CSI: a runtime-applied
# overlay (sudo dtoverlay ...) probes cleanly but fails to capture
# bytes because the firmware cannot replay its CSI clock/pinmux setup
# at runtime.
#
# Targets the legacy unicam path (bcm2835-unicam-legacy), shared by
# Pi 4B, CM4, Zero 2 W, and the Pi 3 family. Pi 5 is rejected: Pi 5
# uses RP1-CFE, not legacy unicam. For Pi 5, use the mono-repo at
# github.com/Kodrea/mini2-thermal-driver (platforms/raspberry-pi/rpi5/).
#
# To build with the legacy 6-item output_mode menu, set the flag
# before running:
#
#   sudo CONFIG_RS300_LEGACY_MENU=1 ./install.sh
#
# Default builds use the 2-item menu (YUV / Y16).
#
# Usage: sudo ./install.sh

set -e

# ── Config ───────────────────────────────────────────────────────────────────
DRV_NAME="rs300"
DRV_VERSION="0.0.1"
DKMS_SRC="/usr/src/${DRV_NAME}-dkms-${DRV_VERSION}"
DKMS_ID="${DRV_NAME}-dkms/${DRV_VERSION}"
OVERLAY_DEST="/boot/firmware/overlays/rs300.dtbo"
CONFIG_FILE="/boot/firmware/config.txt"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Helpers ──────────────────────────────────────────────────────────────────
info()  { echo -e "\033[1;36m▶\033[0m $1"; }
ok()    { echo -e "\033[0;32m✓\033[0m $1"; }
warn()  { echo -e "\033[1;33m!\033[0m $1"; }
fail()  { echo -e "\033[0;31m✗\033[0m $1"; exit 1; }

backup_once() {
    local f="$1"
    [ -f "$f" ] || return 0
    local bak="${f}.bak.$(date +%Y%m%d-%H%M%S)"
    cp -a "$f" "$bak"
    echo "  backup: $bak"
}

# ── Root check ───────────────────────────────────────────────────────────────
if [ "$(id -u)" -ne 0 ]; then
    fail "Must run as root: sudo $0"
fi

echo ""
echo "RS300 Thermal Camera — Raspberry Pi 4 / CM4 / Zero 2 W Installer"
echo "================================================================="
echo ""

# ── Platform check ───────────────────────────────────────────────────────────
info "Checking platform..."
if [ ! -f /proc/device-tree/compatible ]; then
    fail "Cannot read /proc/device-tree/compatible"
fi

COMPAT=$(tr '\0' '\n' < /proc/device-tree/compatible)
MODEL=$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || echo "unknown")

# Reject Pi 5 (BCM2712, RP1-CFE): this installer targets bcm2835-unicam-legacy.
if echo "$COMPAT" | grep -q "brcm,bcm2712"; then
    fail "Pi 5 (BCM2712) uses RP1-CFE, not bcm2835-unicam-legacy.
  Detected: $MODEL
  For Pi 5, use the mono-repo:
    github.com/Kodrea/mini2-thermal-driver
  (platforms/raspberry-pi/rpi5/)"
fi

ok "Platform: $MODEL"

# ── Dependencies ─────────────────────────────────────────────────────────────
info "Checking dependencies..."
REQUIRED_PKGS="dkms device-tree-compiler i2c-tools v4l-utils"
KHEADERS_PKG="linux-headers-$(uname -r)"

MISSING=""
for pkg in $REQUIRED_PKGS; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        MISSING="$MISSING $pkg"
    fi
done

if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
    MISSING="$MISSING $KHEADERS_PKG"
fi

if [ -n "$MISSING" ]; then
    info "Installing missing packages:$MISSING"
    apt-get update -qq
    apt-get install -y $MISSING
fi
ok "Dependencies satisfied"

# ── DKMS module ──────────────────────────────────────────────────────────────
info "Installing kernel module via DKMS..."

if /usr/sbin/dkms status 2>/dev/null | grep -q "${DRV_NAME}-dkms/${DRV_VERSION}"; then
    warn "Removing existing DKMS registration"
    /usr/sbin/dkms remove -m "${DRV_NAME}-dkms" -v "$DRV_VERSION" --all 2>/dev/null || true
fi

mkdir -p "$DKMS_SRC"
cp "${SCRIPT_DIR}/dkms.conf" "$DKMS_SRC/"
cp "${SCRIPT_DIR}/Makefile"  "$DKMS_SRC/"
cp "${SCRIPT_DIR}/rs300.c"   "$DKMS_SRC/"

# Honor CONFIG_RS300_LEGACY_MENU env var during DKMS build by patching
# the MAKE line in the staged dkms.conf so the flag reaches the Makefile.
if [ "${CONFIG_RS300_LEGACY_MENU:-}" = "1" ]; then
    info "Building with legacy 6-item output_mode menu"
    sed -i 's|^MAKE="make |MAKE="make CONFIG_RS300_LEGACY_MENU=1 |' "$DKMS_SRC/dkms.conf"
fi

/usr/sbin/dkms add     -m "${DRV_NAME}-dkms" -v "$DRV_VERSION"
/usr/sbin/dkms build   -m "${DRV_NAME}-dkms" -v "$DRV_VERSION"
/usr/sbin/dkms install -m "${DRV_NAME}-dkms" -v "$DRV_VERSION"

ok "DKMS module installed: $(/usr/sbin/dkms status | grep "${DRV_NAME}-dkms")"

# ── Device tree overlay ─────────────────────────────────────────────────────
info "Compiling device tree overlay..."

OVERLAY_SRC="${SCRIPT_DIR}/rs300-overlay.dts"
OVERLAY_BUILD="${SCRIPT_DIR}/rs300.dtbo"

[ -r "$OVERLAY_SRC" ] || fail "Overlay source not readable: $OVERLAY_SRC"

dtc -q -@ -I dts -O dtb -o "$OVERLAY_BUILD" "$OVERLAY_SRC"
DTBO_SIZE=$(stat -c%s "$OVERLAY_BUILD")
ok "Overlay compiled ($DTBO_SIZE bytes)"

info "Installing overlay to $OVERLAY_DEST..."
cp "$OVERLAY_BUILD" "$OVERLAY_DEST"
chown root:root "$OVERLAY_DEST"
chmod 644 "$OVERLAY_DEST"
ok "Overlay installed"

# ── config.txt: disable camera_auto_detect ──────────────────────────────────
info "Configuring $CONFIG_FILE..."
[ -w "$CONFIG_FILE" ] || fail "$CONFIG_FILE not writable"

backup_once "$CONFIG_FILE"

if grep -qE "^camera_auto_detect=1" "$CONFIG_FILE"; then
    sed -i "s/^camera_auto_detect=1/camera_auto_detect=0/" "$CONFIG_FILE"
    ok "camera_auto_detect set to 0"
elif grep -qE "^camera_auto_detect=0" "$CONFIG_FILE"; then
    ok "camera_auto_detect already 0"
else
    warn "camera_auto_detect line not found, adding"
    echo "camera_auto_detect=0" >> "$CONFIG_FILE"
fi

# ── config.txt: add dtoverlay=rs300 under [all] ─────────────────────────────
if grep -qE "^dtoverlay=rs300$" "$CONFIG_FILE"; then
    ok "dtoverlay=rs300 already present in config.txt"
else
    # Append under [all] stanza. If no [all] stanza exists, add at EOF —
    # on RPi firmware that is treated as [all] by default.
    if grep -qE "^\[all\]$" "$CONFIG_FILE"; then
        # Append at end; the last [all] block is still the effective one.
        echo "dtoverlay=rs300" >> "$CONFIG_FILE"
    else
        printf "\n[all]\ndtoverlay=rs300\n" >> "$CONFIG_FILE"
    fi
    ok "dtoverlay=rs300 added to config.txt"
fi

# ── Done ─────────────────────────────────────────────────────────────────────
echo ""
echo "================================================================="
ok "Installation complete"
echo "================================================================="
echo ""
echo "Reboot required for the overlay to load. After reboot verify:"
echo ""
echo "  1. sudo reboot"
echo "  2. lsmod | grep rs300                 (expect rs300 loaded)"
echo "  3. sudo dmesg | grep -i rs300         (expect 'Starting rs300_probe')"
echo "  4. ls /dev/video0                     (expect device present)"
echo "  5. for m in /dev/media*; do"
echo "       media-ctl -d \$m -p 2>/dev/null | grep -q 'rs300 10-003c' && \\"
echo "         media-ctl -d \$m -p | grep -A2 'rs300 10-003c' && break"
echo "     done                               (expect fmt:YUYV8_2X8/384x288)"
echo ""
echo "First capture test:"
echo "  v4l2-ctl -d /dev/video0 --set-fmt-video=width=384,height=288,pixelformat=YUYV \\"
echo "    --stream-mmap --stream-count=300 --stream-to=/tmp/frames.yuv"
echo "  ls -l /tmp/frames.yuv                 (expect 66355200 bytes = 384*288*2*300)"
echo ""
