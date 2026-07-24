# Setup & Troubleshooting Guide

---

## Quick Start

**After reboot, run:**
```bash
./configure_media.sh
```

Pi 5 media controller config doesn't persist. Links & formats lost at reboot.

**Boot state:**
- ✅ Driver loaded, I2C works (0x3c bus 10)
- ❌ Pipeline links DISABLED
- ❌ Formats NOT configured
- ❌ Streaming fails

---

## What configure_media.sh Does

1. **Enable link**: `media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"`
2. **Set formats** (MUST use 16-bit packed: `UYVY8_1X16` on Pi5):
```bash
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none ...]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none ...]"
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY
```
3. **Validate** configuration works

---

## Configuration Methods

| Method | Setup | Auto? | When to Use |
|--------|-------|-------|------------|
| **Manual** | Run `sudo rs300-configure` after boot | No | Development, testing |
| **Systemd service** | `sudo cp utilities/config/rs300-media-config.service /etc/systemd/system/ && sudo systemctl enable rs300-media-config` | Yes | Production, unattended |
| **Udev rule** | `sudo cp utilities/config/99-rs300.rules /etc/udev/rules.d/` | Yes | Hotplug, driver reload |

---

## Install Systemd Auto-Config

```bash
# The unit already points at /usr/local/bin/rs300-configure, where install.sh
# puts the helper, so there is no path to edit.

# Enable and start
sudo systemctl daemon-reload
sudo systemctl enable rs300-media-config
sudo systemctl start rs300-media-config

# Check logs
sudo journalctl -u rs300-media-config.service -f
```

---

## Install Udev Auto-Config

```bash
# The rule already points at /usr/local/bin/rs300-configure, so there is no
# path to edit.
sudo cp utilities/config/99-rs300.rules /etc/udev/rules.d/

# Reload udev
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## Quick Diagnostics

```bash
lsmod | grep rs300                     # Driver loaded?
i2cdetect -y 10                        # I2C device at 0x3c?
ls -l /dev/video0 /dev/v4l-subdev2    # Devices exist?
media-ctl -p | grep rs300              # Media entity present?
dmesg | grep -i "rs300.*error" | tail  # Recent errors?
```

---

## Decision Tree

```
Problem: No /dev/video0
├─ Driver not loaded (lsmod)? → Go to: Driver Loading
└─ Driver loaded? → Go to: Device Detection

Problem: Can't capture video
├─ Media pipeline not configured? → Run ./configure_media.sh
├─ Wrong format? → Go to: Format Compatibility
└─ I2C not working? → Go to: I2C Communication

Problem: Controls don't work
├─ Subdevice missing? → Go to: Device Detection
└─ Subdevice present? → Go to: Control Operations
```

---

## Driver Loading Issues

**Symptom:** `lsmod | grep rs300` empty

**Check:**
```bash
dkms status | grep rs300
sudo modprobe rs300
dmesg | tail -50
```

| Cause | Fix |
|-------|-----|
| Not installed | `sudo ./install.sh && sudo reboot` |
| Kernel mismatch | `uname -r` → match with kernel headers, then `sudo ./install.sh` |
| Missing overlay | Edit `/boot/firmware/config.txt`: add `dtoverlay=rs300` and `camera_auto_detect=0` |
| Load fails (bad dmesg) | Check error message below |

**Symptom:** Driver loads then unloads immediately

| Error | Cause | Fix |
|-------|-------|-----|
| `control initialization failed` | V4L2 controls issue | Rebuild: `sudo ./install.sh` |
| `Hardware configuration check failed` | Device tree mismatch | Check `dtoverlay=rs300` in config.txt |
| `failed to get regulators` | Power supply config issue | Device tree power pin misconfigured |

**Reload the driver:**

On Pi 5 the module cannot be unloaded. `rmmod rs300` triggers an rp1_cfe
teardown crash, so a reboot is the supported way to reload it.

```bash
sudo reboot
# after the reboot
sudo rs300-configure
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
```

---

## Device Detection Issues

**Symptom:** No `/dev/video0` or `/dev/v4l-subdev2`

**Check:**
```bash
lsmod | grep rs300                   # Loaded?
dmesg | grep "rs300_probe"           # Probe ran?
i2cdetect -y 10                      # Device at 0x3c (or UU)?
media-ctl -p | grep rs300            # Entity present?
```

| Cause | Check | Fix |
|-------|-------|-----|
| I2C device not responding | `i2cdetect -y 10` shows `--` at 0x3c | Hardware issue, check CSI-2 cable & power |
| Probe failed | `dmesg \| grep -i "probe.*fail"` | Check dmesg error message |
| Media entity not created | `media-ctl -p \| grep rs300` empty | Driver issue, check dmesg |

---

## Media Pipeline Configuration

**Symptom:** `/dev/video0` exists but can't capture

**Check:**
```bash
./configure_media.sh                    # Run this first
media-ctl -p | grep ENABLED
v4l2-ctl -d /dev/video0 --get-fmt-video
```

**If not configured:** Run `./configure_media.sh`

**If still broken:**
```bash
media-ctl -p | grep -A2 "rs300"        # Check link status
# Should show: 'csi2':4 -> 'rp1-cfe-csi2_ch0':0 [ENABLED]

media-ctl -p | grep "fmt:UYVY"         # Check format
# Should show formats like UYVY8_1X16
```

**Service not starting:**
```bash
sudo systemctl status rs300-media-config.service
sudo journalctl -u rs300-media-config.service
```

**Manual test:**
```bash
./configure_media.sh
# If works manually but not auto-config, check timing/paths in service file
```

---

## I2C Communication Issues

**Symptom:** Device detected but I2C commands fail

**Check:**
```bash
i2cdetect -y 10                        # Shows UU at 0x3c?
dmesg | grep -i "i2c.*error"           # I2C errors?
```

| Issue | Check | Fix |
|-------|-------|-----|
| I2C write fails | `dmesg \| grep "i2c_transfer fail"` | Check CSI-2 cable, power, reset line |
| Timeout on commands | `dmesg \| grep "timeout"` | Camera busy/hung, restart driver |
| CRC errors | `dmesg \| grep "CRC"` | Data corruption, check bus noise |

**Manual I2C test:**
```bash
i2cget -y 10 0x3c 0x02 b              # Read status (0x00=idle, 0x01=busy, 0x02=fail)
```

---

## Video Capture Issues

**Symptom:** Capture command hangs or fails

**Check:**
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=test.yuv
dmesg | tail -20                       # Check for errors
```

| Issue | Cause | Fix |
|-------|-------|-----|
| Hangs indefinitely | Camera not streaming or I2C stuck | Reboot, then `sudo rs300-configure` |
| `VIDIOC_STREAMON fails` | Pipeline not configured | Run `./configure_media.sh` |
| `Operation not permitted` | Permission issue | Need `sudo` or add user to video group |
| Frame rate too low | Warm-up period | Camera needs 2-3s before valid data, skip first 60 frames |

---

## Control Operation Issues

**Symptom:** Control commands fail

**Check:**
```bash
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls  # Controls visible?
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=50  # Try setting brightness
dmesg | tail -20                             # Check errors
```

| Issue | Fix |
|-------|-----|
| Control not found (list empty) | Driver not loaded or subdevice missing, check device detection |
| Set fails silently | Timing issue, wait 1-2s before next command |
| Set fails with error in dmesg | I2C communication error, check I2C issues section |
| Value doesn't change | Camera not responding, check I2C |

---

## Format Compatibility

**Critical:** Pi 5 requires **16-bit packed formats** like `UYVY8_1X16`, NOT 8-bit dual-lane like `*_2X8`

**Wrong formats cause:** `Format mismatch!` error in dmesg

**Check current format:**
```bash
v4l2-ctl -d /dev/video0 --get-fmt-video
# Good: width=640, height=512, 'UYVY' (packed)
# Bad: Shows error or 8-bit format

media-ctl -p | grep fmt:
# Good: fmt:UYVY8_1X16 or YUYV8_1X16
# Bad: fmt:*_2X8 formats
```

**Fix format:**
```bash
./configure_media.sh                   # Reconfigure pipeline with correct formats
```

---

## Performance Issues

**Symptom:** Low frame rate, jerky video

```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 2>&1 | grep fps
# Check actual fps vs expected

dmesg | grep -i "drop"                 # Dropped frames?
```

| Issue | Expected FPS | Typical Cause | Fix |
|-------|--------------|---------------|-----|
| 640x512 mode | ~60 fps | Warm-up period | Skip first 60 frames (2-3s) |
| 384x288 mode | ~30 fps | CSI-2 bandwidth | Verify 2-lane 80MHz config |
| Consistent drops | Match resolution fps | I2C busy, CPU load | Reduce background tasks |

---

## Kernel Log Errors - Reference

| Error | Location | Meaning | Action |
|-------|----------|---------|--------|
| `Format mismatch!` | dmesg | Wrong format type for Pi5 | Use 16-bit packed, run configure_media.sh |
| `Camera not ready` | rs300.c | Hardware init failed | Check power, I2C, device tree |
| `Timeout waiting for` | rs300.c | I2C command timeout | Reload driver |
| `CRC mismatch` | rs300.c | Data corruption | Check CSI-2 cable |
| `I2C write failed` | rs300.c | Bus error | Check device, power, cable |

---

## Why Manual Config Needed

Media controller is runtime-only (lost at reboot). Not a bug—V4L2 media controller design.

Persistence comparison:

| Type | Persists? | Lost at Reboot |
|------|-----------|----------------|
| Device tree | ✅ | Links, formats |
| Module params | ✅ | Links, formats |
| **Media links** | ❌ | Reset to disabled |
| **Media formats** | ❌ | Reset to default |

---

## References

- **DRIVER_ANALYSIS.md** - Technical deep-dive
- **DEV_QUICK_REFERENCE.md** - Command cheat sheet
- **dmesg** - Always check this first!
