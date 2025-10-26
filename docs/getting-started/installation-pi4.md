# Installation Guide - Raspberry Pi 4

Complete installation guide for the RS300 thermal camera driver on Raspberry Pi 4 with legacy Unicam driver system.

## Overview

**Time Required**: ~10 minutes (plus reboot)

**What You'll Get**:
- ✅ RS300 kernel driver installed via DKMS
- ✅ Device tree overlay configured
- ✅ 60fps thermal streaming at 640×512 or 384×288
- ✅ Legacy Unicam integration

**Important Difference from Pi 5**:
- Pi 4 uses legacy **Unicam** driver (not RP1-CFE)
- Requires **manual driver configuration** before building
- No media controller pipeline required
- Video device appears directly as `/dev/video0`

## Prerequisites

### Hardware Required
- ✅ Raspberry Pi 4B (any RAM variant)
- ✅ RS300 Mini2 thermal camera module
- ✅ Custom Raspberry Pi adapter board (from Purple River)
- ✅ 15-pin FPC ribbon cable (usually included)
- ✅ Power supply: Official 15W USB-C (5.1V/3A) minimum
- ✅ microSD card with Raspberry Pi OS installed

### Software Required
- **OS**: Raspberry Pi OS Bookworm (recommended) or Bullseye (untested)
- **Kernel**: Recent kernel version (included in OS)
- **Internet**: Required for installing dependencies

### Pre-Installation Check

Verify your system:
```bash
# Check OS version
cat /etc/os-release | grep VERSION

# Check kernel version
uname -r

# Verify internet connection
ping -c 3 google.com
```

## Important: Pre-Build Configuration

**⚠️ CRITICAL STEP**: You must configure the driver for your specific module **before** building.

### Determine Your Module

| Module Resolution | Mode Value | FPS Options |
|-------------------|------------|-------------|
| 640×512 | `mode = 0` | 30 or 60 |
| 256×192 | `mode = 1` | 25 or 50 |
| 384×288 | `mode = 2` | 30 or 60 |

**Note**: This manual configuration requirement will be improved in future versions.

## Installation Steps

### Step 1: Install Dependencies

```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers dkms git
```

**What this installs**:
- `raspberrypi-kernel-headers`: Required for building kernel modules
- `dkms`: Dynamic Kernel Module Support (automatic rebuilds on kernel updates)
- `git`: For cloning the repository

**Time**: ~2-3 minutes

### Step 2: Clone Repository

```bash
cd ~
git clone https://github.com/Kodrea/rs300-v4l2-driver.git
cd rs300-v4l2-driver
```

### Step 3: Configure Driver (REQUIRED)

Edit the driver source to match your module:

```bash
nano rs300.c
```

Find these lines near the top of the file (around line 88-91):

```c
static int mode = 0;   // 0=640×512, 1=256×192, 2=384×288
static int fps = 60;   // 256: 25/50fps, 384/640: 30/60fps
static int type = 16;  // 8 or 16 bit
static int debug = 1;  // 0=off, 1=on
```

**Modify for your module**:

**Example 1: 640×512 module @ 60fps** (default):
```c
static int mode = 0;
static int fps = 60;
```

**Example 2: 384×288 module @ 60fps**:
```c
static int mode = 2;
static int fps = 60;
```

**Example 3: 256×192 module @ 50fps**:
```c
static int mode = 1;
static int fps = 50;
```

**Save and exit**: `Ctrl+X`, then `Y`, then `Enter`

### Step 4: Run Setup Script

```bash
chmod +x setup.sh
./setup.sh
```

**What setup.sh does**:
- Builds the RS300 kernel module with your configuration
- Installs via DKMS (enables automatic rebuild on kernel updates)
- Copies device tree overlay to `/boot/firmware/overlays/` (Bookworm) or `/boot/overlays/` (Bullseye)
- Runs post-installation configuration

**Expected Output**:
```
Building kernel module...
Installing via DKMS...
Copying device tree overlay...
Installation complete!
```

**Time**: ~2-3 minutes

### Step 5: Configure Device Tree

**For Bookworm** (current):
```bash
sudo nano /boot/firmware/config.txt
```

**For Bullseye** (older):
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

### Step 6: Reboot

```bash
sudo reboot
```

**Time**: ~30-60 seconds

## Post-Installation Verification

After reboot, log back in and verify installation:

### Step 1: Verify Driver Loaded

```bash
# Check if driver is loaded
lsmod | grep rs300

# Check kernel messages
dmesg | grep rs300

# Verify I2C communication
i2cdetect -y 1
```

**Expected Results**:
- `lsmod`: Should show `rs300` module loaded
- `dmesg`: Should show "RS300 probe successful" or similar
- `i2cdetect`: Should show device at address `0x3c` on bus 1 (note: bus 1, not 10)

**Example i2cdetect output**:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --
```

### Step 2: Check Video Device

```bash
# List video devices
v4l2-ctl --list-devices

# Check device details
v4l2-ctl --all -d /dev/video0
```

**Expected**: Camera linked to Unicam driver on `/dev/video0`

### Step 3: Check Subdevice Controls

```bash
# List available controls
v4l2-ctl -d /dev/v4l-subdev0 --list-ctrls
```

**Should show**: All 11 RS300 controls (brightness, colormap, FFC, etc.)

### Step 4: Set Video Format

**IMPORTANT**: Unicam defaults to 640×480. You must set the format to match your module.

**For 640×512 module**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

**For 384×288 module**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=384,height=288,pixelformat=YUYV
```

**For 256×192 module**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=256,height=192,pixelformat=YUYV
```

### Step 5: Test Streaming

**Quick test** (10 frames):
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
```

**Live viewing with GStreamer**:

**640×512**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```

**384×288**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=384,height=288,framerate=60/1 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```

**256×192**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=256,height=192,framerate=50/1 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```

**Success indicators**:
- ✅ No error messages
- ✅ Frame rate counter shows target FPS
- ✅ Thermal image displays

## Known Issues & Limitations

### Low Voltage Warnings (640×512 module)
- **Symptom**: Yellow lightning bolt icon, kernel messages about voltage
- **Cause**: High current draw on 3.3V CSI port
- **Impact**: Rarely causes operational issues
- **Solution**: Use official 15W power supply minimum, consider 25W supply

### Resolution Changes Require Driver Rebuild
- **Limitation**: Cannot switch resolutions at runtime
- **Workaround**: Edit `rs300.c`, rebuild with `./setup.sh`, reboot
- **Future**: Runtime switching planned for future release

### 256×192 MIPI Issues
- **Status**: MIPI video troubleshooting in progress
- **Symptoms**: I2C works, camera operates, but no MIPI video data
- **Workaround**: Use USB mode for reliable 50Hz streaming
- **Investigation**: Ongoing

## Troubleshooting

### Driver Not Loading

**Check installation**:
```bash
dkms status | grep rs300
```

**Should show**: `rs300/0.0.1, [kernel-version], aarch64: installed`

**If not installed**, rebuild:
```bash
cd ~/rs300-v4l2-driver
./setup.sh
```

### I2C Communication Failed

**Verify camera connected**:
```bash
i2cdetect -y 1
```

**Should see**: Device at `0x3c` on bus 1

**If not visible**:
1. Check FPC cable connections (both ends)
2. Verify camera has power (check for warmth)
3. Inspect FPC cable for damage
4. Check power supply is adequate

### Wrong Resolution

**Symptom**: Distorted or incorrect video

**Cause**: Format mismatch between driver configuration and set format

**Solution**:
1. Verify driver mode matches your module (edit `rs300.c`)
2. Rebuild: `./setup.sh`
3. Reboot: `sudo reboot`
4. Set correct format with `v4l2-ctl --set-fmt-video`

### postinst Error

**Symptom**: Message about postinst not running

**Solution**:
```bash
cd ~/rs300-v4l2-driver
./setup.sh
sudo sh /usr/src/rs300-0.0.1/dkms.postinst
```

**Note**: Only needed first time or if device tree overlay changes

## Advanced Usage

### Changing Configuration

To change resolution or frame rate:

1. **Edit driver**:
```bash
cd ~/rs300-v4l2-driver
nano rs300.c
# Modify mode and fps values
```

2. **Rebuild**:
```bash
./setup.sh
```

3. **Reboot**:
```bash
sudo reboot
```

4. **Set format**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=[WIDTH],height=[HEIGHT],pixelformat=YUYV
```

### Camera Controls

Camera controls are accessed via subdevice (note: `subdev0` on Pi 4, `subdev2` on Pi 5):

```bash
# List controls
v4l2-ctl -d /dev/v4l-subdev0 --list-ctrls

# Trigger FFC calibration
v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=ffc_trigger=0

# Change colormap (0-11)
v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=colormap=3

# Adjust brightness (0-100)
v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=brightness=75
```

**See**: [Camera Controls Guide](../guides/camera-controls.md) for complete reference

## Next Steps

**Installation complete!** Now you can:

1. **[First Thermal Capture →](first-capture.md)** - Quick start guide
2. **[Basic Usage →](../guides/basic-usage.md)** - Streaming commands
3. **[Camera Controls →](../guides/camera-controls.md)** - Adjust settings

## Getting Help

- 📖 [Full Documentation](../../)
- 🐛 [Report Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- ❓ [Troubleshooting Guide](../reference/TROUBLESHOOTING.md)
- 📺 [Video Tutorials](https://linktr.ee/kodrea)

---

**Congratulations!** Your RS300 thermal camera is now installed on Raspberry Pi 4.
