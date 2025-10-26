# RS300 Boot-Time Configuration Guide

## Table of Contents
1. [Understanding the Problem](#understanding-the-problem)
2. [Why Manual Configuration is Needed](#why-manual-configuration-is-needed)
3. [Configuration Options](#configuration-options)
4. [Setup Methods](#setup-methods)
5. [Troubleshooting](#troubleshooting)

---

## Understanding the Problem

### The Media Controller Pipeline Challenge

On Raspberry Pi 5, the RS300 thermal camera requires **runtime media controller configuration** that doesn't persist across reboots. This is different from Pi 4's simpler Unicam driver architecture.

**What Happens at Boot:**
1. ✅ Driver loads successfully (`rs300.ko`)
2. ✅ Device tree overlay activates hardware
3. ✅ I2C communication works (camera responds on bus 10)
4. ✅ Media controller devices appear (`/dev/media0`, `/dev/video0`)
5. ❌ **Pipeline links are DISABLED by default**
6. ❌ **Format configuration is NOT applied**
7. ❌ **Streaming fails until manual configuration**

### What is the Media Controller Pipeline?

The V4L2 media controller is a framework for complex camera systems with multiple processing stages:

```
┌─────────────┐      ┌──────────┐      ┌─────────────┐      ┌──────────────┐
│  RS300      │──────▶│  CSI-2   │──────▶│  RP1-CFE    │──────▶│  /dev/video0 │
│  Thermal    │      │ Receiver │      │  CSI2 Ch0   │      │   (capture)  │
│  Camera     │      │          │      │             │      │              │
└─────────────┘      └──────────┘      └─────────────┘      └──────────────┘
   (rs300 10-003c)      (csi2)         (rp1-cfe-csi2_ch0)

   pad0: Source      pad0: Sink        pad0: Sink
                     pad4: Source
```

**These connections and formats must be configured via `media-ctl` commands.**

---

## Why Manual Configuration is Needed

### Media Controller State is Runtime-Only

Unlike device tree overlays or driver parameters, media controller configuration exists **only in kernel memory**:

| Configuration Type | Persistence | Example |
|-------------------|-------------|---------|
| Device Tree Overlay | ✅ Survives reboot | Camera I2C address, GPIO pins |
| Driver Module Parameters | ✅ Survives reboot (if in modprobe.d) | `mode=0`, `fps=60` |
| **Media Controller Links** | ❌ Lost at reboot | `'csi2':4 → 'rp1-cfe-csi2_ch0':0` |
| **Media Controller Formats** | ❌ Lost at reboot | `YUYV8_1X16/640x512` pipeline |
| V4L2 Controls | ❌ Lost at reboot | Brightness, colormap settings |

### What configure_media.sh Actually Does

The script performs these critical runtime operations:

**1. Enable Pipeline Link:**
```bash
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
```
- Without this, video data can't flow from CSI-2 to capture device
- Link state defaults to DISABLED after boot

**2. Configure Format Cascade:**
```bash
# CSI-2 input (from RS300)
media-ctl -V "'csi2':0 [fmt:YUYV8_1X16/640x512 field:none colorspace:smpte170m ...]"

# CSI-2 output (to RP1-CFE)
media-ctl -V "'csi2':4 [fmt:YUYV8_1X16/640x512 field:none colorspace:smpte170m ...]"

# Video device
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV,...
```
- Ensures format consistency throughout pipeline
- Pi 5 RP1-CFE requires 16-bit packed formats (`*_1X16`)
- Colorspace parameters must match for ISP compatibility

**3. Validate Configuration:**
- Tests streaming capability
- Checks link status
- Verifies format propagation

---

## Configuration Options

You have three options for managing media controller configuration:

### Option 1: Manual Configuration (Default)

**How it works:**
- Run `./configure_media.sh` after every reboot
- Simple, explicit control
- Good for development and testing

**When to use:**
- You're actively developing/debugging
- You want explicit control over configuration
- You don't reboot often

**Advantages:**
- ✅ No system modifications
- ✅ Easy to troubleshoot
- ✅ Explicit control

**Disadvantages:**
- ❌ Must run manually after each boot
- ❌ Camera not ready until you run script

---

### Option 2: Systemd Service (Recommended for Production)

**How it works:**
- systemd service runs `configure_media.sh --non-interactive` at boot
- Starts after media devices are available
- Runs once per boot automatically

**When to use:**
- Production deployments
- Embedded systems
- Unattended operation (boots into ready state)

**Advantages:**
- ✅ Fully automatic
- ✅ Camera ready after boot
- ✅ Easy to enable/disable (`systemctl enable/disable`)
- ✅ Logs visible in `journalctl`

**Disadvantages:**
- ❌ Fixed timing (may race with device initialization)
- ❌ Requires system-level installation

---

### Option 3: Udev Rule (Event-Driven Alternative)

**How it works:**
- udev monitors kernel device events
- When `/dev/media0` appears with RS300, triggers configuration
- More dynamic than systemd timing

**When to use:**
- Want event-driven configuration
- Need to handle driver reload scenarios
- Prefer hardware-triggered setup

**Advantages:**
- ✅ Triggered when hardware is actually ready
- ✅ Works for driver reload (not just boot)
- ✅ More robust timing

**Disadvantages:**
- ❌ More complex debugging
- ❌ Requires careful udev rule syntax

---

### Option 4: Hybrid Approach (Most Robust)

Combine systemd service **and** udev rule:
- Systemd handles normal boot
- Udev handles driver reload/hotplug
- Script is idempotent (safe to run multiple times)

---

## Setup Methods

### Method 1: Manual Configuration (No Installation)

Simply run the configuration script whenever needed:

```bash
# After every reboot
./configure_media.sh

# Or non-interactive for scripting
./configure_media.sh --non-interactive
```

**No installation needed - this is the default workflow.**

---

### Method 2: Install Systemd Service

#### Step 1: Copy Service File

```bash
sudo cp rs300-media-config.service /etc/systemd/system/
```

#### Step 2: Edit Service File Paths

Edit the service file to use absolute paths:

```bash
sudo nano /etc/systemd/system/rs300-media-config.service
```

Update the `ExecStart` line:
```ini
ExecStart=/home/cody/rs300-v4l2-driver/configure_media.sh --non-interactive
```

#### Step 3: Enable and Start Service

```bash
# Reload systemd to recognize new service
sudo systemctl daemon-reload

# Enable service to run at boot
sudo systemctl enable rs300-media-config.service

# Test service now (without rebooting)
sudo systemctl start rs300-media-config.service

# Check status
sudo systemctl status rs300-media-config.service
```

#### Step 4: View Service Logs

```bash
# View service logs
sudo journalctl -u rs300-media-config.service

# Follow logs in real-time
sudo journalctl -u rs300-media-config.service -f

# View logs from last boot
sudo journalctl -u rs300-media-config.service -b
```

#### Disabling Auto-Configuration

```bash
# Disable service (won't run at boot)
sudo systemctl disable rs300-media-config.service

# Stop service now
sudo systemctl stop rs300-media-config.service
```

---

### Method 3: Install Udev Rule

#### Step 1: Copy Udev Rule

```bash
sudo cp 99-rs300.rules /etc/udev/rules.d/
```

#### Step 2: Edit Rule File Paths

Edit the rule to use absolute paths:

```bash
sudo nano /etc/udev/rules.d/99-rs300.rules
```

Update script path in the rule.

#### Step 3: Reload Udev Rules

```bash
# Reload udev rules
sudo udevadm control --reload-rules

# Trigger rules for existing devices
sudo udevadm trigger
```

#### Step 4: Test Udev Rule

```bash
# Monitor udev events to verify rule triggers
sudo udevadm monitor

# In another terminal, reload the driver to trigger event
sudo rmmod rs300
sudo modprobe rs300
```

#### Debugging Udev Rules

```bash
# Test udev rule processing
udevadm test /sys/class/media/media0

# View udev logs
journalctl -f | grep udev
```

---

### Method 4: Automated Installation via setup.sh

The `setup.sh` script includes interactive installation of auto-configuration:

```bash
./setup.sh
```

During installation, you'll be prompted:
```
Enable automatic media configuration at boot? (Y/n): y
Install udev rule for hotplug support? (Y/n): y
```

This will automatically install systemd service and/or udev rule with correct paths.

---

## Troubleshooting

### Service Not Starting at Boot

**Check service status:**
```bash
sudo systemctl status rs300-media-config.service
```

**Common issues:**

1. **Service failed - media device not ready**
   - Add longer delay in service or use udev instead
   - Check dependencies: `After=dev-media0.device`

2. **Service disabled**
   ```bash
   sudo systemctl enable rs300-media-config.service
   ```

3. **Script path incorrect**
   - Edit service file with absolute path
   - `sudo nano /etc/systemd/system/rs300-media-config.service`

4. **Script not executable**
   ```bash
   chmod +x /path/to/configure_media.sh
   ```

### Udev Rule Not Triggering

**Monitor udev events:**
```bash
sudo udevadm monitor
```

**Check rule syntax:**
```bash
# Test rule
udevadm test /sys/class/media/media0
```

**Common issues:**

1. **Rule syntax error**
   - Check for typos in `/etc/udev/rules.d/99-rs300.rules`
   - Reload: `sudo udevadm control --reload-rules`

2. **Wrong device path**
   - Verify device appears: `ls -l /dev/media*`
   - Check rule KERNEL and SUBSYSTEM match

3. **Permissions issue**
   - Rule script must be executable
   - May need to run as root (use TAG+="systemd" approach)

### Camera Still Not Working After Boot

**Verify auto-configuration ran:**

```bash
# For systemd service
sudo journalctl -u rs300-media-config.service -b

# Check media controller state
media-ctl -p | grep ENABLED

# Check video device format
v4l2-ctl -d /dev/video0 --get-fmt-video
```

**Manual verification:**
```bash
# Run configuration manually
./configure_media.sh

# If manual works but auto doesn't, check timing/paths
```

### Finding the Right Timing

If auto-configuration runs too early:

**For systemd:** Add delay in service file:
```ini
[Service]
Type=oneshot
ExecStartPre=/bin/sleep 3
ExecStart=/path/to/configure_media.sh --non-interactive
```

**For udev:** Already includes delay:
```bash
RUN+="/bin/bash -c 'sleep 2 && /path/to/configure_media.sh --non-interactive'"
```

### Checking Configuration Logs

**View detailed configuration process:**
```bash
# Create detailed log
./configure_media.sh 2>&1 | tee config.log

# Check kernel messages
dmesg | grep -E "(rs300|rp1-cfe|csi2)"

# Media topology
media-ctl --print-topology
```

---

## Advanced Configuration

### Running Both Systemd and Udev

For maximum robustness, enable both:

```bash
# Enable systemd for boot
sudo systemctl enable rs300-media-config.service

# Install udev for hotplug
sudo cp 99-rs300.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

The `configure_media.sh` script is **idempotent** - safe to run multiple times. If both trigger, the second run will succeed quickly.

### Custom Configuration

Create your own configuration script based on `configure_media.sh`:

```bash
#!/bin/bash
# my_custom_config.sh

# Use specific format
./configure_media.sh --format YUYV8_1X16 --non-interactive

# Set camera controls
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=brightness=60
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=colormap=3
```

Then use this script in systemd service or udev rule.

---

## Quick Reference

### Manual Configuration
```bash
./configure_media.sh
```

### Install Systemd Auto-Config
```bash
sudo cp rs300-media-config.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable rs300-media-config.service
```

### Install Udev Auto-Config
```bash
sudo cp 99-rs300.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

### Check Status
```bash
# Systemd
sudo systemctl status rs300-media-config.service
sudo journalctl -u rs300-media-config.service

# Manual
media-ctl -p | grep ENABLED
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1
```

---

## Understanding Why This is Necessary

This boot configuration requirement is **not a bug** - it's by design in the V4L2 media controller framework:

1. **Flexibility**: Media controller allows complex pipelines with multiple paths
2. **Safety**: Default-disabled links prevent accidental data routing
3. **Dynamic Configuration**: Allows runtime pipeline changes
4. **Multi-Camera Support**: Each camera can have different routing

The RS300 driver is working correctly. The additional configuration step is a characteristic of the modern V4L2 media controller architecture on Pi 5, which provides more power and flexibility than Pi 4's simpler Unicam system.

---

For additional help, see:
- Main README: [README.md](README.md)
- Development guide: [CLAUDE.md](CLAUDE.md)
- Pipeline diagnostics: `./debug_pipeline.sh`
