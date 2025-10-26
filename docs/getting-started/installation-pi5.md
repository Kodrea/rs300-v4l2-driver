# Installation Guide - Raspberry Pi 5

Complete installation guide for the RS300 thermal camera driver on Raspberry Pi 5 with RP1-CFE camera system.

## Overview

**Time Required**: ~10 minutes (plus reboot)

**What You'll Get**:
- ✅ RS300 kernel driver installed via DKMS
- ✅ Device tree overlay configured
- ✅ Media controller pipeline ready
- ✅ 60fps thermal streaming at 640×512

## Prerequisites

### Hardware Required
- ✅ Raspberry Pi 5 (any RAM variant)
- ✅ RS300 Mini2 thermal camera module
- ✅ Custom Raspberry Pi adapter board (from Purple River)
- ✅ 22-pin to 15-pin FPC adapter cable
- ✅ 15-pin FPC ribbon cable (usually included)
- ✅ Power supply: Official 27W USB-C (5.1V/5A) recommended
- ✅ microSD card with Raspberry Pi OS installed

### Software Required
- **OS**: Raspberry Pi OS Bookworm (current)
- **Kernel**: 6.6.y or later (included in Bookworm)
- **Internet**: Required for installing dependencies

### Pre-Installation Check

Verify your system:
```bash
# Check OS version (should be Bookworm)
cat /etc/os-release | grep VERSION

# Check kernel version (should be 6.6.y or later)
uname -r

# Verify internet connection
ping -c 3 google.com
```

## Installation Steps

### Step 1: Install Dependencies

```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers dkms git v4l-utils
```

**What this installs**:
- `raspberrypi-kernel-headers`: Required for building kernel modules
- `dkms`: Dynamic Kernel Module Support (automatic rebuilds on kernel updates)
- `git`: For cloning the repository
- `v4l-utils`: Video4Linux utilities for testing and configuration

**Time**: ~2-3 minutes

### Step 2: Clone Repository

```bash
cd ~
git clone -b pi5-testing https://github.com/Kodrea/rs300-v4l2-driver.git
cd rs300-v4l2-driver
```

**Important**: Use the `pi5-testing` branch for Pi 5 support.

### Step 3: Run Setup Script

```bash
chmod +x setup.sh
./setup.sh
```

**What setup.sh does**:
- Builds the RS300 kernel module
- Installs via DKMS (enables automatic rebuild on kernel updates)
- Copies device tree overlay to `/boot/firmware/overlays/`
- Runs post-installation configuration

**Expected Output**:
```
Building kernel module...
Installing via DKMS...
Copying device tree overlay...
Installation complete!
```

**Time**: ~2-3 minutes

### Step 4: Configure Device Tree

Edit the boot configuration:
```bash
sudo nano /boot/firmware/config.txt
```

Add these lines at the end:
```
camera_auto_detect=0
dtoverlay=rs300
```

**Explanation**:
- `camera_auto_detect=0`: Disables automatic camera detection (required for custom overlays)
- `dtoverlay=rs300`: Loads the RS300 device tree overlay

**Save and exit**: `Ctrl+X`, then `Y`, then `Enter`

### Step 5: Reboot

```bash
sudo reboot
```

**Time**: ~30-60 seconds

## Post-Installation Configuration

After reboot, log back in and continue:

### Step 6: Verify Driver Loaded

```bash
# Check if driver is loaded
lsmod | grep rs300

# Check kernel messages
dmesg | grep rs300

# Verify I2C communication
i2cdetect -y 10
```

**Expected Results**:
- `lsmod`: Should show `rs300` module loaded
- `dmesg`: Should show "RS300 probe successful" or similar
- `i2cdetect`: Should show device at address `0x3c` on bus 10

**Example output**:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --
```

### Step 7: Configure Media Pipeline

Run the automatic configuration script:
```bash
cd ~/rs300-v4l2-driver
./configure_media.sh
```

**What this does**:
- Auto-detects RS300 camera
- Lets you choose UYVY or YUYV format
- Configures media controller links
- Sets up the complete pipeline
- Tests streaming automatically

**Interactive prompts**:
1. Choose pixel format (UYVY or YUYV) - either works, UYVY recommended
2. Script will configure and test

**Time**: ~1 minute

### Step 8: Verification

Test basic streaming:
```bash
# Quick stream test (10 frames)
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10

# Live viewing
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0
```

**Success indicators**:
- ✅ No error messages
- ✅ Frame rate counter shows ~60fps
- ✅ Thermal image displays (if using ffplay)

## Configuration Persistence

**Important**: The media pipeline configuration does **not** persist across reboots.

### Manual Method (Default)
Run `./configure_media.sh` after every reboot.

### Automatic Methods

See [BOOT_CONFIGURATION.md](../reference/BOOT_CONFIGURATION.md) for three automation options:

1. **Systemd Service**: Runs at boot (recommended for single user)
2. **Udev Rule**: Runs when camera detected (recommended for dynamic use)
3. **Hybrid**: Both methods for maximum reliability

**Quick setup** (systemd):
```bash
./setup.sh --enable-boot-config
```

## Troubleshooting

### Driver Not Loading

**Check installation**:
```bash
dkms status | grep rs300
```

**Should show**: `rs300/0.0.1, 6.6.x, aarch64: installed`

**If not installed**:
```bash
cd ~/rs300-v4l2-driver
./setup.sh
```

### I2C Communication Failed

**Verify camera is connected**:
```bash
i2cdetect -y 10
```

**Should see**: Device at `0x3c`

**If not visible**:
1. Check FPC cable connections (both ends)
2. Verify 22-pin to 15-pin adapter seated properly
3. Check power supply is adequate (27W recommended)
4. Inspect FPC cable for damage

### Media Pipeline Errors

**Check pipeline status**:
```bash
media-ctl -p
```

**Look for**:
- ✅ `rs300 10-003c` entity present
- ✅ Links marked `[ENABLED]`
- ✅ Format: `UYVY8_1X16` or `YUYV8_1X16`

**If issues**:
```bash
# Re-run configuration
./configure_media.sh

# Check for format mismatches
media-ctl -p | grep fmt
```

### "Format mismatch!" Error

**Cause**: Pi 5 RP1-CFE only supports 16-bit packed formats.

**Solution**: Ensure using `UYVY8_1X16` or `YUYV8_1X16` (not `*8_2X8` formats)

```bash
# Correct configuration
./configure_media.sh
```

### Video Device Not Found

**Check video devices**:
```bash
ls -l /dev/video*
v4l2-ctl --list-devices
```

**Should see**: `/dev/video0` linked to rp1-cfe

**If missing**:
1. Check driver loaded: `lsmod | grep rs300`
2. Check dmesg: `dmesg | grep -i error`
3. Verify device tree overlay: `sudo dtoverlay -l`

### Low Frame Rate

**Check actual frame rate**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
```

**Expected**: ~60fps

**If low**:
1. Check power supply (27W recommended)
2. Verify thermal throttling: `vcgencmd get_throttled`
3. Check CPU usage: `top`
4. Ensure adequate cooling

## Advanced Configuration

### Module Parameters

The driver supports compile-time module parameters (set before building):

Edit `rs300.c` before running `setup.sh`:
```c
static int mode = 0;   // 0=640×512, 1=256×192, 2=384×288
static int fps = 30;   // 25, 30, 50, 60
static int type = 16;  // 8 or 16 bit
static int debug = 1;  // 0=off, 1=on
```

**Note**: Runtime mode switching not yet implemented - requires driver rebuild for changes.

### Custom Media Pipeline

For advanced users wanting manual control:

```bash
# Link CSI receiver to video node
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set formats (all pads must match)
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set video node format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

**See**: [RS300_Media_Pipeline_Guide.md](../reference/RS300_Media_Pipeline_Guide.md) for complete pipeline documentation

## Next Steps

**Installation complete!** Now you can:

1. **[First Thermal Capture →](first-capture.md)** - Quick start guide
2. **[Basic Usage →](../guides/basic-usage.md)** - Streaming commands
3. **[Camera Controls →](../guides/camera-controls.md)** - Adjust settings
4. **[Boot Automation →](../reference/BOOT_CONFIGURATION.md)** - Auto-configure on boot

## Getting Help

- 📖 [Full Documentation](../../)
- 🐛 [Report Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- ❓ [Troubleshooting Guide](../../TROUBLESHOOTING.md)
- 📺 [Video Tutorials](https://linktr.ee/kodrea)

---

**Congratulations!** Your RS300 thermal camera is now installed on Raspberry Pi 5.
