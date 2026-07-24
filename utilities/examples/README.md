# RS300 Driver Examples

This directory contains example scripts and code demonstrating RS300 thermal camera usage and ISP integration.

## Available Examples

### 1. ISP Processing Example (Bash)

**File**: `isp_processing_example.sh`

Interactive script demonstrating various capture and processing modes using the Raspberry Pi 5 ISP.

**Usage**:
```bash
./isp_processing_example.sh
```

**Features**:
- Direct capture (no ISP, lowest latency)
- GStreamer preview with scaling
- Dual-stream output (full resolution + thumbnail)
- Format conversion (YUV to RGB)
- ISP capability inspection

**Requirements**:
- RS300 driver loaded and configured
- GStreamer 1.0 with V4L2 plugins
- `/dev/video0` (RS300) and `/dev/video20-27` (pispbe) available

### 2. Capture for ISP (Python)

**File**: `capture_for_isp.py`

Python script for capturing raw thermal frames and checking ISP availability.

**Usage**:
```bash
# Capture 100 frames with live preview
./capture_for_isp.py --frames 100 --preview

# Save raw YUV data for ISP processing
./capture_for_isp.py --frames 100 --output /tmp/thermal.yuv

# Check ISP device availability
./capture_for_isp.py --isp-info
```

**Requirements**:
```bash
pip3 install opencv-python numpy
```

**Features**:
- Capture frames from RS300 thermal camera
- Save raw YUV data for ISP processing
- Live preview during capture
- ISP device detection and reporting
- Frame statistics (FPS, dropped frames)

## Quick Start

### Test Basic Capture

```bash
# 1. Ensure driver is loaded
lsmod | grep rs300

# 2. Configure media pipeline
sudo rs300-configure

# 3. Run ISP example
./isp_processing_example.sh
# Choose option 1 for direct capture
```

### Capture and Process with ISP

```bash
# 1. Capture raw frames
./capture_for_isp.py --frames 100 --output /tmp/thermal_raw.yuv

# 2. View raw frames (requires ffmpeg)
ffplay -f rawvideo -pixel_format uyvy422 -video_size 640x512 /tmp/thermal_raw.yuv

# 3. Process through ISP (see RASPBERRY_PI_ISP_GUIDE.md Section 6)
```

### Check ISP Availability

```bash
./capture_for_isp.py --isp-info
```

Expected output:
```
=== PiSP Backend Information ===
  ✓ /dev/video20: pispbe-input (main input)
  ✓ /dev/video21: pispbe-tdn_input (temporal denoise)
  ✓ /dev/video22: pispbe-stitch_input (HDR stitch)
  ✓ /dev/video23: pispbe-output0 (primary output)
  ✓ /dev/video24: pispbe-output1 (secondary output)
  ✓ /dev/video25: pispbe-tdn_output (denoise output)
  ✓ /dev/video26: pispbe-stitch_output (HDR output)
  ✓ /dev/video27: pispbe-config (configuration)

All PiSP backend devices available!
```

## Troubleshooting

### "Error: /dev/video0 not found"

**Solution**:
```bash
# Check driver loaded
lsmod | grep rs300

# If not loaded, reinstall
sudo ./install.sh
sudo reboot
```

### "Error: /dev/video20 not found"

**Cause**: PiSP backend driver not loaded

**Solution**:
```bash
# Check kernel module
lsmod | grep pisp

# Check dmesg
dmesg | grep -i pisp

# Ensure device tree doesn't conflict
grep camera_auto_detect /boot/firmware/config.txt
# Should show: camera_auto_detect=0
```

### "GStreamer not found"

**Solution**:
```bash
sudo apt update
sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-{good,bad,base}
```

### Python Script Fails

**Solution**:
```bash
# Install dependencies
pip3 install opencv-python numpy

# Or use system packages
sudo apt install python3-opencv python3-numpy
```

## Advanced Examples

For advanced ISP integration examples, see:
- Section 6: Example Workflows
- Section 9: Practical Recommendations

## Creating Your Own Examples

### Bash Script Template

```bash
#!/bin/bash
# Your RS300 example script

# Check RS300 available
if [ ! -e /dev/video0 ]; then
    echo "Error: RS300 not found"
    exit 1
fi

# Configure if needed
sudo rs300-configure

# Your capture/processing logic here
gst-launch-1.0 v4l2src device=/dev/video0 ! ...
```

### Python Script Template

```python
#!/usr/bin/env python3
import cv2

# Open RS300
cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 512)

# Capture loop
while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Your processing here
    cv2.imshow('RS300', frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
```

## Contributing

When adding new examples:
1. Use clear, descriptive filenames
2. Include usage documentation in file header
3. Add error checking for dependencies
4. Update this README with new example
5. Test on clean Raspberry Pi 5 setup

## License

Same as main RS300 driver project.
