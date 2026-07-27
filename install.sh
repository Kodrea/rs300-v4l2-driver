#!/bin/bash
#
# RS300 Thermal Camera Install Script
#
# Installs the RS300 driver via DKMS, compiles and installs the DT
# overlay, and wires /boot/firmware/config.txt so the overlay loads at
# boot. Boot-time overlay load is required on Pi CSI: a runtime-applied
# overlay (sudo dtoverlay ...) probes cleanly but fails to capture
# bytes because the firmware cannot replay its CSI clock/pinmux setup
# at runtime.
#
# Supports two CSI receiver families, selected at runtime from
# /proc/device-tree/compatible:
#
#   Pi 5 (BCM2712)   RP1-CFE      overlay: rs300-overlay.pi5.dts
#   Pi 4B, CM4,      legacy       overlay: rs300-overlay.dts
#   Zero 2 W, Pi 3   unicam
#
# The driver makes the matching choice for the bus format on its own
# side, so the two halves agree on the board without being told twice.
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
CONFIGURE_DEST="/usr/local/bin/rs300-configure"
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
echo "RS300 Thermal Camera Installer"
echo "================================================================="
echo ""

# ── Platform detection ───────────────────────────────────────────────────────
info "Checking platform..."
if [ ! -f /proc/device-tree/compatible ]; then
    fail "Cannot read /proc/device-tree/compatible"
fi

COMPAT=$(tr '\0' '\n' < /proc/device-tree/compatible)
MODEL=$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || echo "unknown")

# Pi 5 (BCM2712) drives RP1-CFE and needs its own overlay targeting csi0 and
# the 16-bit packed bus format. Everything else here is bcm2835-unicam-legacy.
if echo "$COMPAT" | grep -q "brcm,bcm2712"; then
    PLATFORM="pi5"
    OVERLAY_NAME="rs300-overlay.pi5.dts"
    RECEIVER="RP1-CFE"
    EXPECT_FMT="YUYV8_1X16"
else
    PLATFORM="legacy"
    OVERLAY_NAME="rs300-overlay.dts"
    RECEIVER="bcm2835-unicam-legacy"
    EXPECT_FMT="YUYV8_2X8"
fi

ok "Platform: $MODEL"
ok "CSI receiver: $RECEIVER (overlay: $OVERLAY_NAME)"

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

# Clear any previous rs300 DKMS registration, whatever name it was added under.
# A board that has had an earlier install can carry "rs300/<ver>" while this
# installer adds "rs300-dkms/<ver>". Both build the same rs300.ko into /updates,
# so leaving both registered makes two packages fight over one module file.
for entry in $(/usr/sbin/dkms status 2>/dev/null | sed 's/[,:].*//' | sort -u); do
    dk_mod="${entry%%/*}"
    dk_ver="${entry##*/}"
    case "$dk_mod" in
        "${DRV_NAME}"|"${DRV_NAME}-dkms")
            warn "Removing existing DKMS registration: ${dk_mod}/${dk_ver}"
            /usr/sbin/dkms remove -m "$dk_mod" -v "$dk_ver" --all 2>/dev/null || true
            rm -rf "/usr/src/${dk_mod}-${dk_ver}"
            ;;
    esac
done

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

OVERLAY_SRC="${SCRIPT_DIR}/${OVERLAY_NAME}"
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

# ── Media pipeline helper ────────────────────────────────────────────────────
# configure_media.sh sets the format on the CSI-2 pads and runs a stream test.
# Installing it under a stable name on PATH is what the removed setup.sh used
# to do, and the docs and the systemd unit both refer to it by that path.
info "Installing media pipeline helper to $CONFIGURE_DEST..."
CONFIGURE_SRC="${SCRIPT_DIR}/configure_media.sh"
[ -r "$CONFIGURE_SRC" ] || fail "Helper not readable: $CONFIGURE_SRC"
install -m 755 -o root -g root "$CONFIGURE_SRC" "$CONFIGURE_DEST"
ok "Installed $(basename "$CONFIGURE_DEST")"

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
echo "     done                               (expect fmt:${EXPECT_FMT}/384x288)"
echo ""
echo "Configure the media pipeline and run its built-in stream test:"
echo "  sudo rs300-configure"
echo ""
echo "First capture test:"
echo "  v4l2-ctl -d /dev/video0 --set-fmt-video=width=384,height=288,pixelformat=YUYV \\"
echo "    --stream-mmap --stream-count=300 --stream-to=/tmp/frames.yuv"
echo "  ls -l /tmp/frames.yuv                 (expect 66355200 bytes = 384*288*2*300)"
echo ""
echo "The default module mode is 384x288. For a 640x512 or 256x192 module set"
echo "mode in /etc/modprobe.d/rs300.conf (0=640x512, 1=256x192, 2=384x288) and"
echo "reboot."
echo ""
