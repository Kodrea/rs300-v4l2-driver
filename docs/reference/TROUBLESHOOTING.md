# RS300 Driver Troubleshooting Guide

**Comprehensive guide for diagnosing and fixing common issues**

---

## Table of Contents
1. [Quick Diagnostics](#quick-diagnostics)
2. [Driver Loading Issues](#driver-loading-issues)
3. [Device Detection Issues](#device-detection-issues)
4. [Video Capture Issues](#video-capture-issues)
5. [Control Operation Issues](#control-operation-issues)
6. [Media Pipeline Issues](#media-pipeline-issues)
7. [I2C Communication Issues](#i2c-communication-issues)
8. [Performance Issues](#performance-issues)
9. [Format Compatibility Issues](#format-compatibility-issues)
10. [Common Error Messages](#common-error-messages)

---

## Quick Diagnostics

### Fast Health Check

Run these commands in sequence to quickly identify the problem area:

```bash
# 1. Check driver loaded
lsmod | grep rs300
# Expected: rs300 should be listed

# 2. Check I2C device detected
i2cdetect -y 10
# Expected: Device at 0x3c

# 3. Check video device exists
ls -l /dev/video*
# Expected: /dev/video0 exists

# 4. Check subdevice exists
ls -l /dev/v4l-subdev*
# Expected: /dev/v4l-subdev2 exists

# 5. Check media topology
media-ctl -p | grep rs300
# Expected: rs300 10-003c entity shown

# 6. Check recent errors
dmesg | grep -i "rs300.*error" | tail -10
# Expected: No errors (or only old/resolved errors)
```

### Decision Tree

```
No /dev/video0?
├─ Driver loaded (lsmod)? → NO → See "Driver Loading Issues"
└─ Driver loaded? → YES → See "Device Detection Issues"

Can't capture video?
├─ Media pipeline configured? → NO → See "Media Pipeline Issues"
├─ Format compatible? → NO → See "Format Compatibility Issues"
└─ I2C working? → NO → See "I2C Communication Issues"

Controls not working?
├─ Subdevice exists? → NO → See "Device Detection Issues"
└─ Subdevice exists? → YES → See "Control Operation Issues"
```

---

## Driver Loading Issues

### Symptom: `lsmod | grep rs300` shows nothing

**Diagnosis Steps**:

```bash
# 1. Check if driver is installed
dkms status | grep rs300
# Expected: rs300 version X.X.X: installed

# 2. Try loading manually
sudo modprobe rs300

# 3. Check for load errors
dmesg | tail -50
```

**Common Causes & Solutions**:

#### Cause 1: Driver not installed via DKMS

```bash
# Solution: Run setup script
./setup.sh
sudo reboot
```

#### Cause 2: Kernel version mismatch

```bash
# Check kernel version
uname -r

# Rebuild for current kernel
sudo dkms remove rs300/0.1 --all
./setup.sh
sudo reboot
```

#### Cause 3: Missing dependencies

```bash
# Check dmesg for missing symbols
dmesg | grep -i "unknown symbol"

# Solution: Ensure kernel headers match running kernel
sudo apt update
sudo apt install raspberrypi-kernel-headers
./setup.sh
sudo reboot
```

#### Cause 4: Device tree overlay not enabled

**Check `/boot/firmware/config.txt`**:

```bash
grep -E "camera_auto_detect|dtoverlay=rs300" /boot/firmware/config.txt
```

**Expected**:
```
camera_auto_detect=0
dtoverlay=rs300
```

**If missing, add**:
```bash
sudo nano /boot/firmware/config.txt
# Add the lines above
sudo reboot
```

### Symptom: Driver loads but immediately unloads

**Diagnosis**:
```bash
sudo modprobe rs300
dmesg | tail -30
```

**Common Errors**:

- `failed to initialize controls`: Control initialization failed
- `Hardware configuration check failed`: Device tree mismatch
- `failed to get regulators`: Power supply configuration issue

**Solutions**: See error-specific sections below.

### How to Reload Driver Without Reboot (Development Workflow)

**Status**: ✅ Fixed (2025-10-26) - Regulators properly disabled in rs300_remove()

**Quick Reload Method**:

```bash
# Unload driver
sudo rmmod rs300

# Wait a moment for cleanup
sleep 1

# Reload driver
sudo modprobe rs300

# Reconfigure media pipeline
cd ~/rs300-v4l2-driver
./configure_media.sh

# Verify functionality
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
```

**What Gets Reset**:
- ✅ Hardware: Full power cycle (regulators disabled then re-enabled)
- ✅ Driver State: V4L2 subdevice, media entity, all controls
- ⚠️ NOT Reset: Device tree overlay, media pipeline configuration

**Verification After Reload**:

```bash
# 1. Driver loaded
lsmod | grep rs300
# Expected: rs300 module listed

# 2. I2C communication
i2cdetect -y 10
# Expected: UU at address 0x3c

# 3. Media controller
media-ctl -d /dev/media2 -p | grep rs300
# Expected: rs300 10-003c entity present

# 4. V4L2 controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
# Expected: 17 controls listed
```

**Development Benefit**: ~10-15x faster iteration (10-15 seconds vs 60+ seconds for reboot)

**Technical Note**: The fix added `regulator_bulk_disable()` to rs300_remove() to ensure clean power state. Without this, regulators remained enabled causing undefined hardware state on reload. See rs300.c:3003-3019.

---

## Device Detection Issues

### Symptom: No /dev/video0 or /dev/v4l-subdev2

**Diagnosis Steps**:

```bash
# 1. Check if driver is loaded
lsmod | grep rs300

# 2. Check probe succeeded
dmesg | grep "rs300_probe"

# 3. Check for probe errors
dmesg | grep -A20 "Starting rs300_probe"

# 4. Check I2C bus
i2cdetect -y 10
```

**Common Causes**:

#### Cause 1: I2C device not responding

**Check I2C detection**:
```bash
i2cdetect -y 10
```

**Expected**: Device at 0x3c shows as `3c`

**If shows `UU`**: Driver is loaded and claimed the device (this is OK)

**If shows `--`**: Hardware problem
- Check camera connection
- Check cable (needs 22-pin to 15-pin adapter for Pi 5)
- Check camera power
- Try different CSI port

#### Cause 2: Wrong I2C bus number

On Pi 5, camera I2C should be on bus 10.

**Verify in device tree**:
```bash
dtc -I fs /sys/firmware/devicetree/base | grep -A5 rs300
```

**Check `/boot/firmware/rs300-overlay.dtbo` is correct for Pi 5**

#### Cause 3: Probe failed during initialization

**Check probe log**:
```bash
dmesg | grep -A30 "Starting rs300_probe"
```

**Look for**:
- `Failed to get regulators`: Power supply issue
- `Hardware configuration check failed`: Wrong lane count or link frequency
- `failed to register sensor sub-device`: V4L2 registration failed

**Solutions**:

For regulator issues:
```bash
# Check device tree defines dummy regulators
# See rs300-overlay.dts for regulator definitions
```

For hardware config issues:
```bash
# Verify device tree settings match
# Must be: 2 lanes, 80MHz link frequency
dtc -I fs /sys/firmware/devicetree/base | grep -A10 rs300
```

### Symptom: Device exists but shows wrong properties

```bash
# Check device capabilities
v4l2-ctl -d /dev/video0 --all

# Check subdevice info
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
```

**If controls are missing**: Driver initialization partially failed
- Check dmesg for control registration errors
- Verify `Control handler initialized` appears in dmesg

---

## Video Capture Issues

### Symptom: Captured images are uniformly dark/constant data pattern

⚠️ **MOST COMMON ISSUE** - Camera warm-up timing (October 2025)

**Symptom**: Captured thermal images appear uniformly dark or show no thermal gradients. Raw data shows constant byte patterns like `36 80 36 80...` instead of varying thermal data.

**Root Cause**: Camera requires **~2 seconds warm-up** after stream start before outputting valid thermal data.

**Evidence**:
- Frame 1 (0.0s): 2 unique patterns ❌ (initialization)
- Frame 30 (1.0s): 2 unique patterns ❌ (warming up)
- Frame 60 (2.0s): 4,133+ unique patterns ✅ (VALID)

**Solution**:
```bash
# CORRECT METHOD - Capture with warm-up
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=90 \
  --stream-to=/tmp/thermal.yuyv

# Extract frame 60+ (after 2-second warm-up)
dd if=/tmp/thermal.yuyv of=/tmp/thermal_valid.yuyv \
  bs=655360 count=1 skip=59

# Convert to image
ffmpeg -y -f rawvideo -pix_fmt yuyv422 -s 640x512 \
  -i /tmp/thermal_valid.yuyv ~/thermal.png
```

**Verify data variation**:
```bash
# Check for data variation (should be >1000 for valid thermal data)
hexdump -C /tmp/thermal_valid.yuyv | awk '{print $2,$3,$4,$5}' | sort -u | wc -l
```

### Symptom: `v4l2-ctl --stream-mmap` fails or hangs

**Diagnosis**:

```bash
# Try simple capture with verbose output
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=test.yuv --verbose
```

**Common Errors**:

#### Error: "VIDIOC_STREAMON: Broken pipe"

**Cause**: Media pipeline not configured or stream start failed

**Solution**:
```bash
# 1. Configure media pipeline
./configure_media.sh

# 2. Verify pipeline is enabled
media-ctl -p | grep ENABLED

# 3. Check stream start in dmesg
dmesg | grep "RS300_SET_STREAM"
```

**Expected in dmesg**:
```
[  ] === RS300_SET_STREAM CALLED: enable=1 ===
[  ] Stream started successfully
```

#### Error: "VIDIOC_DQBUF: Input/output error"

**Cause**: Camera streaming failed to initialize or stopped unexpectedly

**Diagnosis**:
```bash
# Check for stream errors
dmesg | grep -E "rs300.*(stream|error)" | tail -20

# Check I2C status
sudo i2cget -y 10 0x3c 0x02 b 0x00
# 0x00 = idle, 0x01 = busy, 0x02+ = error
```

**Solutions**:

If status shows error (0x02+):
```bash
# Reset camera
sudo rmmod rs300
sudo modprobe rs300
./configure_media.sh
```

If I2C times out:
```bash
# Check I2C bus health
i2cdetect -y 10

# If bus is frozen, reboot
sudo reboot
```

#### Error: "Format mismatch"

See [Format Compatibility Issues](#format-compatibility-issues)

### Symptom: Capture succeeds but file is all zeros

**Cause**: Camera not actually streaming data (command sent but not started)

**Diagnosis**:
```bash
# Check if data is flowing from CSI-2
cat /sys/kernel/debug/csi2/csi2_stats 2>/dev/null || echo "Debug FS not available"

# Check frame count in driver
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10 2>&1 | grep fps
```

**Solutions**:

1. Verify FPS is set correctly:
```bash
# Check module parameter
cat /sys/module/rs300/parameters/fps

# FPS should match mode (60 for 640x512)
```

2. Check MIPI CSI-2 is receiving data:
```bash
# Enable debug in device tree if needed
# Check for CSI-2 errors
dmesg | grep csi2
```

3. Power cycle camera:
```bash
sudo rmmod rs300
sleep 2
sudo modprobe rs300
./configure_media.sh
```

---

## Control Operation Issues

### Symptom: `v4l2-ctl -c brightness=50` fails

**Diagnosis**:

```bash
# 1. Check control exists
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls | grep brightness

# 2. Try setting with error output
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=50 2>&1

# 3. Check dmesg for command errors
dmesg | grep "Setting control\|brightness"
```

**Common Errors**:

#### Error: "Control not found" or "VIDIOC_S_EXT_CTRLS: Invalid argument"

**Cause**: Wrong device or control name

**Solution**:
```bash
# Use subdevice, not video device
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=50  # Correct
v4l2-ctl -d /dev/video0 -c brightness=50      # Wrong

# List available controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
```

#### Error: Control sets but dmesg shows timeout

**Diagnosis**:
```bash
dmesg | grep -E "brightness.*timeout|Command timed out"
```

**Cause**: I2C communication issue or camera not responding

**Solution**:
```bash
# Check I2C bus
i2cdetect -y 10

# Check status register
sudo i2cget -y 10 0x3c 0x02 b 0x00

# If stuck busy (0x01), reset camera
sudo rmmod rs300
sudo modprobe rs300
```

### Symptom: FFC trigger (`ffc_trigger=1`) hangs or fails

**Expected behavior**: FFC takes 1-2 seconds

**Diagnosis**:
```bash
# Set with verbose output
time v4l2-ctl -d /dev/v4l-subdev2 -c ffc_trigger=1

# Check dmesg
dmesg | grep "FFC\|shutter"
```

**If timeout**:
- FFC may need longer than configured timeout
- Check camera is warmed up (wait 30+ seconds after power-on)
- Verify lens cover is removed

**If command returns immediately**:
- Check for errors in dmesg
- Verify camera supports FFC (not all thermal cameras do)

---

## Media Pipeline Issues

### Symptom: Media pipeline not configured correctly

**Diagnosis**:

```bash
# Check media topology
media-ctl -p

# Verify link is enabled
media-ctl -p | grep "\[ENABLED\]"

# Check format on all pads
media-ctl -p | grep "fmt:"
```

**Expected Output**:

```
- entity 5: rs300 10-003c (1 pad, 1 link)
            type V4L2 subdev subtype Sensor
        pad0: Source
                [fmt:UYVY8_1X16/640x512 ...]
                -> "csi2":0 [ENABLED]

- entity 9: csi2 (5 pads, 5 links)
        pad0: Sink
                [fmt:UYVY8_1X16/640x512 ...]
                <- "rs300 10-003c":0 [ENABLED]
        pad4: Source
                [fmt:UYVY8_1X16/640x512 ...]
                -> "rp1-cfe-csi2_ch0":0 [ENABLED]
```

**Common Issues**:

#### Issue: Link not enabled

**Solution**:
```bash
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
```

#### Issue: Format mismatch between pads

**Symptom**: Different formats on connected pads

**Solution**: Set format on all pads
```bash
./configure_media.sh
# Or manually:
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 ...]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 ...]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 ...]"
```

#### Issue: Pipeline not persistent across reboots

**Cause**: Media pipeline configuration is not saved

**Solution**: Set up auto-configuration
```bash
# See BOOT_CONFIGURATION.md for options:
# - systemd service
# - udev rule
# - rc.local
```

---

## I2C Communication Issues

### Symptom: "i2c read/write error" in dmesg

**Diagnosis**:

```bash
# Check I2C bus status
i2cdetect -y 10

# Try direct I2C read
sudo i2cget -y 10 0x3c 0x02 b 0x00

# Check for I2C errors in kernel
dmesg | grep -i "i2c.*error"
```

**Common Causes**:

#### Cause 1: I2C bus contention or lockup

**Solution**:
```bash
# Reset I2C bus (requires reboot)
sudo reboot

# Or try unloading/reloading driver
sudo rmmod rs300
sudo modprobe i2c_bcm2835  # Pi 5 I2C driver
sudo modprobe rs300
```

#### Cause 2: Wrong I2C bus or address

**Verify** in dmesg:
```bash
dmesg | grep "rs300.*i2c"
```

**Should show**: i2c-10, address 0x3c

#### Cause 3: Electrical issues (bad cable, loose connection)

**Symptoms**:
- Intermittent "i2c transfer failed"
- Device detection works, but communication fails
- `i2cdetect` shows device sometimes but not always

**Solution**:
- Reseat camera cable
- Check for damaged pins
- Try different cable
- Verify 22-pin to 15-pin adapter (for Pi 5)

### Symptom: CRC errors in camera commands

**Diagnosis**:
```bash
dmesg | grep -i crc
```

**Expected**: Should see CRC calculations, not CRC errors

**If CRC errors occur**:
- Possible I2C transmission corruption
- Check cable/connection quality
- Verify CRC calculation in driver matches camera expectation

---

## Performance Issues

### Symptom: Low frame rate or dropped frames

**Diagnosis**:

```bash
# Measure actual frame rate
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 2>&1 | grep fps

# Expected for 640x512: ~60 fps
```

**Common Causes**:

#### Cause 1: FPS not set correctly

**Check**:
```bash
cat /sys/module/rs300/parameters/fps
```

**Solution**:
```bash
# Reload with correct FPS
sudo rmmod rs300
sudo modprobe rs300 fps=60
./configure_media.sh
```

#### Cause 2: CPU/system overload

**Check**:
```bash
# Monitor CPU during capture
top -d 1

# Check for thermal throttling
vcgencmd measure_clock arm
vcgencmd measure_temp
```

**Solution**: Reduce system load, improve cooling

#### Cause 3: Pixel rate mismatch

**Check current pixel rate**:
```bash
v4l2-ctl -d /dev/v4l-subdev2 -C pixel_rate
```

**Expected**:
- 8-bit formats: 200 MHz
- 16-bit formats: 400 MHz

**If incorrect**: Driver bug, report issue

### Symptom: High CPU usage during capture

**Normal**: Some CPU usage expected for format conversion

**Abnormal**: >50% CPU for simple capture

**Diagnosis**:
```bash
# Profile during capture
perf record -g v4l2-ctl --stream-mmap --stream-count=100
perf report
```

**Possible causes**:
- Inefficient buffer handling
- Memory copy overhead
- System configuration issue

---

## Format Compatibility Issues

### Symptom: "Format not supported" or "Format mismatch"

**Diagnosis**:

```bash
# Check supported formats
v4l2-ctl -d /dev/video0 --list-formats-ext

# Check current format
v4l2-ctl -d /dev/video0 --get-fmt-video
```

**Pi 5 RP1-CFE Compatibility**:

**IMPORTANT**: Pi 5 RP1-CFE **only supports 16-bit packed formats**:
- ✅ `UYVY8_1X16` (0x200e)
- ✅ `YUYV8_1X16` (0x200f)
- ❌ `UYVY8_2X8` (0x2006) - NOT supported on Pi 5
- ❌ `YUYV8_2X8` (0x2007) - NOT supported on Pi 5

**Solution**: Always use 16-bit packed formats on Pi 5

```bash
# Set correct format
v4l2-ctl -d /dev/video0 --set-fmt-video=pixelformat=UYVY
# Or
v4l2-ctl -d /dev/video0 --set-fmt-video=pixelformat=YUYV

# Verify
v4l2-ctl -d /dev/video0 --get-fmt-video | grep "Pixel Format"
```

### Symptom: Wrong resolution

**Check available resolutions**:
```bash
v4l2-ctl -d /dev/video0 --list-formats-ext
```

**Available modes**:
- 640×512 @ 60fps (mode=0, default)
- 256×192 @ 25fps (mode=1)
- 384×288 @ 30fps (mode=2)

**To change mode**: Requires driver reload
```bash
sudo rmmod rs300
sudo modprobe rs300 mode=0  # 0, 1, or 2
./configure_media.sh
```

---

## Common Error Messages

### Error: "probe of 10-003c failed with error -22"

**Meaning**: Invalid argument (-EINVAL) during probe

**Common causes**:
- Device tree configuration mismatch
- Wrong lane count (must be 2)
- Wrong link frequency (must be 80MHz)

**Solution**: Check device tree overlay

### Error: "timeout waiting for device"

**Meaning**: Camera not responding to I2C commands

**Solutions**:
1. Check camera power
2. Verify I2C connection
3. Reset camera
4. Check cable connection

### Error: "failed to register sensor sub-device"

**Meaning**: V4L2 subsystem rejected driver registration

**Causes**:
- Another driver claiming same device
- Invalid device configuration
- V4L2 framework issue

**Solution**:
```bash
# Check for conflicting drivers
lsmod | grep -E "rs300|camera"

# Check dmesg for details
dmesg | grep -A10 "failed to register"
```

### Error: "Format mismatch!"

**Meaning**: RP1-CFE received unsupported format

**Solution**: Use 16-bit packed formats (see Format Compatibility section)

### Error: "CSI-2 receiver error"

**Meaning**: MIPI CSI-2 physical layer or protocol error

**Solutions**:
1. Check cable quality
2. Verify link frequency matches (80MHz)
3. Check for EMI/signal integrity issues
4. Try different CSI port (if available)

---

## Advanced Debugging

### Enable Driver Debug Messages

```bash
# Reload with debug enabled
sudo rmmod rs300
sudo modprobe rs300 debug=1

# Or set debug level dynamically
echo 1 | sudo tee /sys/module/rs300/parameters/debug
```

### Kernel Debug Features

```bash
# Enable V4L2 core debugging (if compiled with debug)
echo 0xff | sudo tee /sys/module/videodev/parameters/debug

# Enable media controller debug
echo 0xff | sudo tee /sys/module/media/parameters/debug
```

### Capture Full System State

```bash
#!/bin/bash
# debug_snapshot.sh - Capture complete system state for bug reports

OUTPUT="rs300_debug_$(date +%Y%m%d_%H%M%S).txt"

{
    echo "=== System Info ==="
    uname -a
    cat /etc/os-release

    echo -e "\n=== Kernel Modules ==="
    lsmod | grep -E "rs300|video|media|i2c"

    echo -e "\n=== I2C Devices ==="
    i2cdetect -y 10

    echo -e "\n=== V4L2 Devices ==="
    ls -l /dev/video* /dev/v4l-subdev* 2>/dev/null

    echo -e "\n=== Media Topology ==="
    media-ctl -p

    echo -e "\n=== Device Formats ==="
    v4l2-ctl -d /dev/video0 --all
    v4l2-ctl -d /dev/v4l-subdev2 --all

    echo -e "\n=== Module Parameters ==="
    cat /sys/module/rs300/parameters/*

    echo -e "\n=== Recent Kernel Messages ==="
    dmesg | grep -E "rs300|csi2|rp1-cfe" | tail -100

    echo -e "\n=== Device Tree ==="
    dtc -I fs /sys/firmware/devicetree/base | grep -A20 rs300

} > "$OUTPUT"

echo "Debug info saved to: $OUTPUT"
```

---

## Getting Help

If issues persist after trying these solutions:

1. **Capture debug info** using the script above
2. **Check GitHub issues**: https://github.com/[your-repo]/rs300-v4l2-driver/issues
3. **Provide details**:
   - Full dmesg output
   - Media controller topology
   - Steps to reproduce
   - System configuration

4. **Useful logs to include**:
   ```bash
   # Full kernel log
   dmesg > dmesg.txt

   # Media pipeline state
   media-ctl -p > media_topology.txt

   # I2C status
   i2cdetect -y 10 > i2c_scan.txt

   # Control test results
   ./test_controls.sh > control_test.log 2>&1
   ```

---

**Document Version**: 1.0
**Last Updated**: 2025-10-21
**Covers**: RS300 driver v0.01.01 on Raspberry Pi 5
