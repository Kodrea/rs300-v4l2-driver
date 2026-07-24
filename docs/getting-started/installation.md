# Installation Guide - Raspberry Pi 4 & 5

Complete installation guide for the RS300 thermal camera driver on Raspberry Pi 4 or 5.

---

## Quick Overview

**Time Required**: ~10 minutes (plus reboot)

**Supported Platforms**:
- ✅ **Raspberry Pi 5** (RP1-CFE media controller, 640×512@60fps primary)
- ✅ **Raspberry Pi 4** (Legacy Unicam, pre-build configuration required)

**Key Differences**:
| Aspect | Pi 4 | Pi 5 |
|--------|------|------|
| Interface | Unicam (direct V4L2) | RP1-CFE (media controller) |
| Config | Pre-build driver modification | Auto-detect at runtime |
| I2C Bus | `i2c-1` | `i2c-10` |
| Media Pipeline | Not needed | Required (run `./configure_media.sh`) |
| Subdevice | `/dev/v4l-subdev0` | `/dev/v4l-subdev2` |

---

## Prerequisites

### Hardware Required (Both Platforms)
- ✅ RS300 Mini2 thermal camera module
- ✅ Custom Raspberry Pi adapter board (from Purple River)
- ✅ Appropriate FPC ribbon cable (15-pin for Pi 4, custom for Pi 5)
- ✅ Power supply: 15W minimum (Pi 4), 27W recommended (Pi 5)
- ✅ microSD card with Raspberry Pi OS installed

### Software Required
- **OS**: Raspberry Pi OS Bookworm (recommended) or Bullseye
- **Kernel**: 6.6.y or later (Bookworm) or recent version (Bullseye)
- **Internet**: Required for installing dependencies

### Pre-Installation Check

```bash
# Check OS version
cat /etc/os-release | grep VERSION

# Check kernel version (should be 6.6.y or later for Pi 5)
uname -r

# Verify internet connection
ping -c 3 google.com
```

---

## Installation Steps

### Step 1: Install Dependencies (Both Platforms)

```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers dkms git v4l-utils
```

**What this installs**:
- `raspberrypi-kernel-headers`: Required for building kernel modules
- `dkms`: Dynamic Kernel Module Support (automatic rebuilds on kernel updates)
- `git`: For cloning the repository
- `v4l-utils`: Video4Linux utilities for testing

**Time**: ~2-3 minutes

### Step 2: Clone Repository (Both Platforms)

```bash
cd ~
git clone https://github.com/Kodrea/rs300-v4l2-driver.git
cd rs300-v4l2-driver
```

---

## Step 3: Configure Driver (Pi 4 ONLY)

**⚠️ Pi 5 users skip this step**

Pi 4 requires pre-build configuration. Edit the driver source to match your module:

```bash
nano rs300.c
```

Find these lines (around line 88-91):

```c
static int mode = 0;   // 0=640×512, 1=256×192, 2=384×288
static int fps = 60;   // 256: 25/50fps, 384/640: 30/60fps
static int type = 16;  // 8 or 16 bit
static int debug = 1;  // 0=off, 1=on
```

**Select your module configuration:**

| Module | Mode | Command |
|--------|------|---------|
| 640×512 @ 60fps | 0 | `mode = 0; fps = 60;` |
| 384×288 @ 60fps | 2 | `mode = 2; fps = 60;` |
| 256×192 @ 50fps | 1 | `mode = 1; fps = 50;` |

**Save and exit**: `Ctrl+X`, then `Y`, then `Enter`

---

## Step 4: Run Setup Script (Both Platforms)

```bash
chmod +x install.sh
sudo ./install.sh
```

**What install.sh does**:
- Builds the RS300 kernel module
- Installs via DKMS (enables automatic rebuild on kernel updates)
- Copies device tree overlay
- Runs post-installation configuration

**Expected Output**:
```
Building kernel module...
Installing via DKMS...
Copying device tree overlay...
Installation complete!
```

**Time**: ~2-3 minutes

---

## Step 5: Configure Device Tree (Both Platforms)

**For Pi 5 (Bookworm)**:
```bash
sudo nano /boot/firmware/config.txt
```

**For Pi 4 (Bookworm)**:
```bash
sudo nano /boot/firmware/config.txt
```

**For Pi 4 (Bullseye)**:
```bash
sudo nano /boot/config.txt
```

Add these lines at the end:
```
camera_auto_detect=0
dtoverlay=rs300
```

**Explanation**:
- `camera_auto_detect=0`: Disables automatic camera detection
- `dtoverlay=rs300`: Loads the RS300 device tree overlay

**Save and exit**: `Ctrl+X`, then `Y`, then `Enter`

---

## Step 6: Reboot (Both Platforms)

```bash
sudo reboot
```

**Time**: ~30-60 seconds

---

## Post-Installation Verification

After reboot, log back in and verify:

### Step 1: Verify Driver Loaded (Both Platforms)

```bash
# Check if driver is loaded
lsmod | grep rs300

# Check kernel messages
dmesg | grep rs300
```

**Expected**: Module `rs300` shown in lsmod, "probe successful" in dmesg

### Step 2: Verify I2C Communication

**Pi 5**:
```bash
i2cdetect -y 10
```

**Pi 4**:
```bash
i2cdetect -y 1
```

**Expected output** (both):
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --
```

Device should show at address `0x3c`.

### Step 3: Check Video Device (Both Platforms)

```bash
# List video devices
v4l2-ctl --list-devices

# Check device details
v4l2-ctl -d /dev/video0 --all
```

**Expected**: Camera linked to driver on `/dev/video0`

### Step 4: Set Video Format

**Pi 4 ONLY** — Unicam defaults to 640×480, must set format to match module:

```bash
# For 640×512 module
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV

# For 384×288 module
v4l2-ctl -d /dev/video0 --set-fmt-video=width=384,height=288,pixelformat=YUYV

# For 256×192 module
v4l2-ctl -d /dev/video0 --set-fmt-video=width=256,height=192,pixelformat=YUYV
```

**Pi 5**: Format set automatically by `./configure_media.sh` (see next step)

### Step 5: Configure Media Pipeline

**Pi 5 ONLY**:

Run the automatic configuration script:

```bash
cd ~/rs300-v4l2-driver
./configure_media.sh
```

**What this does**:
- Auto-detects RS300 camera
- Lets you choose UYVY or YUYV format
- Configures media controller links
- Tests streaming automatically

**Time**: ~1 minute

**Pi 4**: Skip this step (no media controller needed)

### Step 6: Test Streaming (Both Platforms)

```bash
# Quick stream test (10 frames)
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
```

**Success indicators**:
- ✅ No error messages
- ✅ Frame rate counter shows ~60fps (Pi 5) or configured fps (Pi 4)

---

## Configuration Persistence (Pi 5 Only)

**Important**: Pi 5 media pipeline configuration does **not** persist across reboots.

### Manual Method (Default)
Run `./configure_media.sh` after every reboot.

### Automatic Methods (Optional)

See [SETUP_AND_TROUBLESHOOTING.md](../reference/SETUP_AND_TROUBLESHOOTING.md) for three automation options:

1. **Systemd Service**: Runs at boot
2. **Udev Rule**: Runs when camera detected
3. **Hybrid**: Both methods for maximum reliability

---

## Troubleshooting

### Driver Not Loading (Both Platforms)

**Check installation**:
```bash
dkms status | grep rs300
```

**Should show**: `rs300/0.0.1, [kernel-version], aarch64: installed`

**If not installed**, rebuild:
```bash
cd ~/rs300-v4l2-driver
sudo ./install.sh
```

---

### I2C Communication Failed (Both Platforms)

**Verify camera connected**:

**Pi 5**:
```bash
i2cdetect -y 10
```

**Pi 4**:
```bash
i2cdetect -y 1
```

**Should see**: Device at `0x3c`

**If not visible**:
1. Check FPC cable connections (both ends)
2. Verify camera has power
3. Check power supply is adequate
4. Inspect FPC cable for damage

---

### Video Device Not Found (Both Platforms)

**Check video devices**:
```bash
ls -l /dev/video*
v4l2-ctl --list-devices
```

**Should see**: `/dev/video0` linked to driver

**If missing**:
1. Check driver loaded: `lsmod | grep rs300`
2. Check dmesg: `dmesg | grep -i error`
3. Verify device tree overlay: `sudo dtoverlay -l`

---

### Format Mismatch (Pi 5 Only)

**Symptom**: `Format mismatch!` error in dmesg

**Cause**: Pi 5 RP1-CFE only supports 16-bit packed formats.

**Solution**: Ensure using `UYVY8_1X16` or `YUYV8_1X16` (not `*8_2X8` formats)

```bash
./configure_media.sh
```

---

### Wrong Resolution (Pi 4 Only)

**Symptom**: Distorted or incorrect video

**Cause**: Format mismatch between driver configuration and set format

**Solution**:
1. Verify driver mode matches your module (edit `rs300.c`)
2. Rebuild: `sudo ./install.sh`
3. Reboot: `sudo reboot`
4. Set correct format with `v4l2-ctl --set-fmt-video`

---

### Low Frame Rate (Both Platforms)

**Check actual frame rate**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
```

**If low**:
1. Check power supply (27W Pi 5, 15W Pi 4 minimum)
2. Verify thermal throttling: `vcgencmd get_throttled`
3. Check CPU usage: `top`

---

### Low Voltage Warnings (Pi 4 Only)

**Symptom**: Yellow lightning bolt icon, kernel messages about voltage

**Cause**: High current draw on 3.3V CSI port

**Impact**: Rarely causes operational issues

**Solution**: Use official 15W power supply minimum, consider 25W supply

---

## Advanced Configuration

### Module Parameters (Both Platforms)

The driver supports compile-time module parameters. Edit `rs300.c` before running `install.sh`:

```c
static int mode = 0;   // 0=640×512, 1=256×192, 2=384×288
static int fps = 30;   // 25, 30, 50, 60
static int type = 16;  // 8 or 16 bit
static int debug = 1;  // 0=off, 1=on
```

**Note**: Changes require driver rebuild (`sudo ./install.sh`) and reboot.

### Custom Media Pipeline (Pi 5 Only)

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

See [RS300_Media_Pipeline_Guide.md](../reference/RS300_Media_Pipeline_Guide.md) for complete pipeline documentation.

---

## Next Steps

**Installation complete!** Now you can:

1. **[First Thermal Capture →](first-capture.md)** - Quick start guide
2. **[Setup & Troubleshooting →](../reference/SETUP_AND_TROUBLESHOOTING.md)** - Boot automation, detailed diagnostics
3. **[Quick Reference →](../reference/DEV_QUICK_REFERENCE.md)** - Command cheat sheet

---

## Getting Help

- 📖 [Full Documentation](../../)
- 🐛 [Report Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- ❓ [Troubleshooting Guide](../reference/SETUP_AND_TROUBLESHOOTING.md)

---

**Congratulations!** Your RS300 thermal camera is now installed on Raspberry Pi.
