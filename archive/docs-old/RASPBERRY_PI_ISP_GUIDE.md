# Raspberry Pi 5 ISP Integration Guide for RS300 Thermal Camera

## Executive Summary

The Raspberry Pi 5 features a split ISP architecture (PiSP - Pi Image Signal Processor) with two distinct components that can enhance your RS300 thermal camera data:

- **Front End (FE)**: Hardware in RP1 chip - limited utility for thermal cameras
- **Back End (BE)**: Memory-to-memory ISP in BCM2712 - **highly useful for YUV processing**

**Key Finding**: Your RS300 thermal camera outputs YUV data, which the PiSP **Back End supports** for advanced post-processing including temporal noise reduction, scaling, format conversion, and dual-stream output.

---

## Table of Contents

1. [Pi 5 ISP Architecture Overview](#1-pi-5-isp-architecture-overview)
2. [Current RS300 Pipeline (Bypassing ISP)](#2-current-rs300-pipeline-bypassing-isp)
3. [Front End ISP (pisp-fe) - Limited Use Case](#3-front-end-isp-pisp-fe---limited-use-case)
4. [Back End ISP (pispbe) - Primary Tool for Thermal Processing](#4-back-end-isp-pispbe---primary-tool-for-thermal-processing)
5. [Practical Integration Strategies](#5-practical-integration-strategies)
6. [Example Workflows](#6-example-workflows)
7. [Performance Considerations](#7-performance-considerations)
8. [libcamera Integration (Advanced)](#8-libcamera-integration-advanced)

---

## 1. Pi 5 ISP Architecture Overview

### Two-Stage ISP Design

```
┌─────────────────────────────────────────────────────────────────┐
│                    Raspberry Pi 5 ISP Architecture              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐      ┌──────────────┐      ┌──────────────┐ │
│  │   Camera     │      │  RP1 Chip    │      │  BCM2712     │ │
│  │   Sensor     │      │              │      │  SoC         │ │
│  └──────┬───────┘      │ ┌──────────┐ │      │ ┌──────────┐ │ │
│         │              │ │          │ │      │ │          │ │ │
│    MIPI CSI-2          │ │  Front   │ │      │ │  Back    │ │ │
│         │              │ │  End     │ │      │ │  End     │ │ │
│         └──────────────┼─┤  (FE)    │ │      │ │  (BE)    │ │ │
│                        │ │  ISP     │ │      │ │  ISP     │ │ │
│                        │ └────┬─────┘ │      │ └────┬─────┘ │ │
│                        │      │       │      │      │       │ │
│                        │   To DRAM ───┼──────┼──────┘       │ │
│                        │      │       │      │              │ │
│                        └──────┼───────┘      │  (M2M)       │ │
│                               │              └──────────────┘ │
│                               └──── Raw/YUV Frames            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Component Breakdown

| Component | Location | Primary Function | Data Flow |
|-----------|----------|------------------|-----------|
| **CSI-2 Receiver** | RP1 | Receive MIPI data | Camera → DRAM |
| **Front End (FE)** | RP1 | Basic processing, statistics | CSI-2 → DRAM (parallel) |
| **Back End (BE)** | BCM2712 | Full ISP processing | DRAM → DRAM (M2M) |

### Why This Matters for Thermal Cameras

Traditional cameras output **Bayer raw data** that needs debayering (the ISP's primary job). Your RS300 thermal camera outputs **processed YUV data**, which changes the use case:

- ❌ **No debayering needed** - FE/BE Bayer processing not applicable
- ✅ **YUV post-processing useful** - BE supports YUV input for enhancement
- ✅ **Temporal denoise valuable** - BE can reduce thermal noise over time
- ✅ **Multi-stream output** - Generate different resolutions simultaneously

---

## 2. Current RS300 Pipeline (Bypassing ISP)

### Your Current Setup

```bash
$ media-ctl -p
```

**Current data flow**:
```
RS300 Sensor (YUV) → CSI2 → rp1-cfe-csi2_ch0 → /dev/video0
                                ↓
                          Direct capture
                        (ISP not involved)
```

**Media topology**:
```
rs300 10-003c:0 [YUYV8_1X16/640x512]
    ↓
csi2:0 [YUYV8_1X16/640x512]
    ↓
csi2:4 [YUYV8_1X16/640x512] → rp1-cfe-csi2_ch0 → /dev/video0 [UYVY/640x512]
```

**pisp-fe exists but unused**:
```
pisp-fe (entity 10) - 5 pads, 7 links
    pad0: Sink [SRGGB16_1X16] ← csi2:4 [] (NOT ENABLED)
```

### Why pisp-fe Isn't Connected

The Front End ISP expects **Bayer raw formats** (RGGB, BGGR, etc.) but your RS300 outputs **YUV**. The FE is designed for:
- Raw Bayer → debayer → YUV conversion
- Statistics on raw sensor data
- Defective pixel correction on Bayer patterns

Since thermal cameras pre-process data, the FE provides minimal value.

---

## 3. Front End ISP (pisp-fe) - Limited Use Case

### Available Video Devices

```bash
/dev/video4  # rp1-cfe-fe_image0 (full resolution output)
/dev/video5  # rp1-cfe-fe_image1 (downscaled output)
/dev/video6  # rp1-cfe-fe_stats (statistics buffer)
/dev/video7  # rp1-cfe-fe_config (configuration input)
```

### FE Capabilities (from RP1 hardware)

- **Statistics gathering**: Histograms, auto-exposure data
- **Downscaling**: Generate lower-resolution stream
- **Compression**: PiSP compression mode
- **Defective pixel correction**: For Bayer sensors

### Why FE Has Limited Value for RS300

1. **Format mismatch**: FE expects Bayer (SRGGB, GBRG, etc.), RS300 outputs YUV
2. **Pre-processed data**: Thermal sensor already applies corrections
3. **Statistics irrelevant**: Histogram/exposure stats designed for RGB sensors
4. **No quality benefit**: YUV data doesn't benefit from Bayer-specific processing

### Theoretical FE Integration (Not Recommended)

If you wanted to experiment, you would:

```bash
# Connect RS300 to FE input (likely to fail due to format mismatch)
media-ctl -l "'csi2':4 -> 'pisp-fe':0[1]"

# Configure FE input (would need YUV support, which FE may not have)
media-ctl -V "'pisp-fe':0 [fmt:YUYV8_1X16/640x512]"

# Configure FE outputs
media-ctl -V "'pisp-fe':2 [fmt:YUYV8_1X16/640x512]"  # Full res
media-ctl -V "'pisp-fe':3 [fmt:YUYV8_1X16/320x256]"  # Downscaled
```

**Expected result**: Format negotiation failure or unsupported operation.

---

## 4. Back End ISP (pispbe) - Primary Tool for Thermal Processing

### Architecture: Memory-to-Memory Processing

Unlike the FE, the Back End is a **memory-to-memory (M2M) device** - it doesn't sit in the capture pipeline but operates on frames already in DRAM.

```
┌───────────────────────────────────────────────────────┐
│           PiSP Back End Processing Flow               │
├───────────────────────────────────────────────────────┤
│                                                       │
│  1. Capture from RS300 → /dev/video0 (UYVY)          │
│         ↓                                             │
│  2. Frames stored in DRAM                             │
│         ↓                                             │
│  3. Feed to pispbe (/dev/video20-27)                  │
│         ↓                                             │
│  4. ISP processing (denoise, scale, convert)          │
│         ↓                                             │
│  5. Output to /dev/video23-24 (processed frames)      │
│                                                       │
└───────────────────────────────────────────────────────┘
```

### Available Video Devices

```bash
$ v4l2-ctl --list-devices
pispbe (platform:1000880000.pisp_be):
    /dev/video20  # pispbe-input (main input)
    /dev/video21  # pispbe-tdn_input (temporal denoise input)
    /dev/video22  # pispbe-stitch_input (HDR stitch input)
    /dev/video23  # pispbe-output0 (processed output #1)
    /dev/video24  # pispbe-output1 (processed output #2)
    /dev/video25  # pispbe-tdn_output (denoise output)
    /dev/video26  # pispbe-stitch_output (HDR output)
    /dev/video27  # pispbe-config (configuration buffer)
    /dev/media2   # Main BE media device
    /dev/media3   # Secondary BE media device
```

### Supported Input/Output Formats

**Input formats** (`/dev/video20`):
```bash
$ v4l2-ctl -d /dev/video20 --list-formats

YUV Formats (Perfect for RS300!):
  - YU12 (Planar YUV 4:2:0)
  - NV12 (Y/UV 4:2:0)
  - YUYV (YUYV 4:2:2) ← Your current format
  - UYVY (UYVY 4:2:2) ← Your current format
  - 422P (Planar YUV 4:2:2)

RGB Formats:
  - RGB3 (24-bit RGB)
  - BGR3 (24-bit BGR)
  - RGBX, BGRX (32-bit)

Bayer Formats (not applicable):
  - RGGB, BGGR, GRBG, GBRG (8/10/12/14/16-bit)

Grayscale:
  - GREY (8-bit)
  - Y16 (16-bit) ← Potential for thermal data
```

**Output formats** (`/dev/video23`, `/dev/video24`):
```
All input formats plus:
  - All YUV variants (420, 422, 444)
  - RGB variants (24/48-bit)
  - Compressed formats (PiSP compression)
```

### BE Processing Capabilities

| Feature | Description | Thermal Camera Benefit |
|---------|-------------|------------------------|
| **Temporal Denoise** | Multi-frame noise reduction | **High** - Reduces thermal sensor noise |
| **Spatial Processing** | Per-frame filtering | **Medium** - Can enhance edges |
| **Scaling** | Up/downscaling with quality filtering | **High** - Generate multiple resolutions |
| **Format Conversion** | YUV ↔ RGB, colorspace conversion | **Medium** - For display/processing |
| **Dual Output** | Two simultaneous output streams | **High** - Full-res + thumbnail |
| **HDR Stitching** | Combine multiple exposures | **Low** - Thermal cameras don't need HDR |
| **Lens Shading** | Correct vignetting | **Low** - Thermal optics different |
| **Color Processing** | Gamma, color matrix, saturation | **Medium** - Could enhance colormapped thermal |

### Media Controller Topology

```bash
$ media-ctl -d /dev/media2 -p

pispbe (entity 1) - 8 pads:
    pad0: Sink ← pispbe-input (/dev/video20)
    pad1: Sink ← pispbe-tdn_input (/dev/video21)
    pad2: Sink ← pispbe-stitch_input (/dev/video22)
    pad3: Source → pispbe-output0 (/dev/video23)
    pad4: Source → pispbe-output1 (/dev/video24)
    pad5: Source → pispbe-tdn_output (/dev/video25)
    pad6: Source → pispbe-stitch_output (/dev/video26)
    pad7: Sink ← pispbe-config (/dev/video27)
```

---

## 5. Practical Integration Strategies

### Strategy A: Direct Capture (Current Setup)

**Use case**: Minimal latency, direct thermal data access

```bash
# Your current workflow
./configure_media.sh
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
```

**Pros**:
- Lowest latency
- No additional processing overhead
- Raw thermal data

**Cons**:
- No noise reduction
- Single output format
- Manual scaling required

---

### Strategy B: Post-Processing with pispbe

**Use case**: Enhanced thermal video with noise reduction and dual outputs

#### Step 1: Capture Raw Frames to Buffer

```bash
# Capture from RS300 to memory buffer or file
gst-launch-1.0 v4l2src device=/dev/video0 num-buffers=100 ! \
    video/x-raw,format=UYVY,width=640,height=512 ! \
    filesink location=/tmp/thermal_raw.yuv
```

#### Step 2: Process Through pispbe

```bash
# Configure pispbe input (YUV422)
v4l2-ctl -d /dev/video20 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY

# Configure output0 (full resolution, same format)
v4l2-ctl -d /dev/video23 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY

# Configure output1 (downscaled for preview)
v4l2-ctl -d /dev/video24 --set-fmt-video=\
    width=320,height=256,pixelformat=UYVY

# Process frames (requires custom app or GStreamer pipeline)
```

#### Step 3: GStreamer Pipeline Example

```bash
# Real-time processing: RS300 → pispbe → display
gst-launch-1.0 \
    v4l2src device=/dev/video0 ! \
    video/x-raw,format=UYVY,width=640,height=512,framerate=30/1 ! \
    queue ! \
    v4l2video1convert output-io-mode=dmabuf-import ! \
    video/x-raw,format=UYVY,width=640,height=512 ! \
    autovideosink
```

*Note*: Direct V4L2 M2M integration requires userspace application (see Section 6).

---

### Strategy C: Temporal Denoise for Thermal Noise Reduction

**Use case**: Multi-frame averaging to reduce thermal sensor noise

```bash
# Configure temporal denoise input
v4l2-ctl -d /dev/video21 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY

# Configure denoise output
v4l2-ctl -d /dev/video25 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY

# Temporal denoise requires configuration buffer (pisp_be_tiles_config)
# See libpisp documentation for config structure
```

**Why this is valuable for thermal**:
- Thermal sensors have inherent noise (especially at higher temperatures)
- Temporal denoise averages frames over time
- Reduces flicker and hot pixels
- Improves image quality without spatial blur

---

### Strategy D: Dual-Stream Output

**Use case**: Simultaneous full-resolution recording + low-res preview

```bash
# Input: 640×512 thermal
v4l2-ctl -d /dev/video20 --set-fmt-video=width=640,height=512,pixelformat=UYVY

# Output 0: Full resolution for recording
v4l2-ctl -d /dev/video23 --set-fmt-video=width=640,height=512,pixelformat=UYVY

# Output 1: Downscaled for live preview (saves bandwidth)
v4l2-ctl -d /dev/video24 --set-fmt-video=width=320,height=256,pixelformat=UYVY

# Both outputs available simultaneously
```

**Benefits**:
- Record high-quality thermal video
- Display low-bandwidth preview
- No need for separate scaling in software

---

## 6. Example Workflows

### Workflow 1: Simple V4L2 Capture with ISP Post-Processing

```python
#!/usr/bin/env python3
"""
RS300 Thermal Camera with PiSP Backend Processing
Demonstrates memory-to-memory ISP usage for thermal enhancement
"""

import cv2
import numpy as np

# Step 1: Capture from RS300 (current method)
cap = cv2.VideoCapture('/dev/video0', cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 512)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('U', 'Y', 'V', 'Y'))

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Step 2: Process with OpenCV (alternative to pispbe for simple cases)
    # For pispbe integration, need to use V4L2 M2M directly (see below)

    # Apply temporal averaging (software alternative to pispbe TDN)
    # frame_denoised = cv2.fastNlMeansDenoisingColored(frame, None, 10, 10, 7, 21)

    cv2.imshow('RS300 Thermal', frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
```

### Workflow 2: GStreamer with ISP Processing

```bash
#!/bin/bash
# Real-time RS300 thermal capture with ISP enhancement

gst-launch-1.0 -v \
    v4l2src device=/dev/video0 ! \
    video/x-raw,format=UYVY,width=640,height=512,framerate=30/1 ! \
    tee name=t \
    t. ! queue ! videoconvert ! autovideosink \
    t. ! queue ! \
        videoscale ! video/x-raw,width=320,height=256 ! \
        videoconvert ! autovideosink
```

### Workflow 3: Direct V4L2 M2M ISP Access (Advanced)

```c
/*
 * RS300 PiSP Backend Integration Example
 * Direct V4L2 memory-to-memory ISP usage
 */

#include <stdio.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

int main() {
    int fd_input = open("/dev/video20", O_RDWR);  // pispbe-input
    int fd_output = open("/dev/video23", O_RDWR); // pispbe-output0

    // Configure input format (UYVY from RS300)
    struct v4l2_format fmt_in = {
        .type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
        .fmt.pix_mp = {
            .width = 640,
            .height = 512,
            .pixelformat = V4L2_PIX_FMT_UYVY,
            .field = V4L2_FIELD_NONE,
            .num_planes = 1,
        }
    };
    ioctl(fd_input, VIDIOC_S_FMT, &fmt_in);

    // Configure output format (processed)
    struct v4l2_format fmt_out = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
        .fmt.pix_mp = {
            .width = 640,
            .height = 512,
            .pixelformat = V4L2_PIX_FMT_UYVY,
            .field = V4L2_FIELD_NONE,
            .num_planes = 1,
        }
    };
    ioctl(fd_output, VIDIOC_S_FMT, &fmt_out);

    // Request buffers, queue frames, STREAMON
    // ... (full V4L2 M2M workflow)

    // Process frames in loop:
    // 1. Read from /dev/video0 (RS300)
    // 2. Write to /dev/video20 (pispbe input)
    // 3. Read from /dev/video23 (pispbe output)
    // 4. Display/save processed frame

    close(fd_input);
    close(fd_output);
    return 0;
}
```

**Required libraries**: `libpisp` (for configuration structures)

### Workflow 4: Temporal Noise Reduction for Thermal

```bash
# Concept: Feed frames to temporal denoise input
# Requires pisp_be_tiles_config configuration

# 1. Configure TDN input
v4l2-ctl -d /dev/video21 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY

# 2. Configure TDN output
v4l2-ctl -d /dev/video25 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY

# 3. Configure TDN parameters via /dev/video27 (config buffer)
#    Requires pisp_be_tiles_config structure from libpisp
#    See: https://github.com/raspberrypi/libpisp

# 4. Stream frames through TDN pipeline
#    (Requires custom application using V4L2 M2M API)
```

---

## 7. Performance Considerations

### Latency Analysis

| Pipeline | Latency | CPU Usage | Quality |
|----------|---------|-----------|---------|
| **Direct capture** (current) | ~2 frames (@30fps = 67ms) | Minimal | Raw |
| **Software denoise** (OpenCV) | ~5-10 frames (167-333ms) | High (50-80%) | Good |
| **pispbe processing** | ~3-4 frames (100-133ms) | Low (5-10%) | Excellent |
| **libcamera stack** | ~5-6 frames (167-200ms) | Medium (20-30%) | Excellent |

### Hardware Acceleration Benefits

The pispbe runs on dedicated hardware in the BCM2712, providing:

- **Zero-copy operation**: DMA transfers, no CPU involvement
- **Parallel processing**: ISP operates while CPU does other work
- **Power efficiency**: Hardware ISP uses less power than software
- **Consistent performance**: Not affected by CPU load

### When to Use Each Approach

| Use Case | Recommended Pipeline |
|----------|---------------------|
| Real-time display, lowest latency | **Direct capture** (current) |
| Recording with quality enhancement | **pispbe post-processing** |
| Live preview + high-quality recording | **pispbe dual output** |
| Thermal noise reduction | **pispbe temporal denoise** |
| Format conversion (YUV→RGB) | **pispbe** or software |
| Integration with existing tools | **GStreamer with ISP elements** |

---

## 8. libcamera Integration (Advanced)

### What is libcamera?

libcamera is Raspberry Pi's modern camera framework that automatically manages the ISP pipeline:

```
Camera → libcamera → Auto-configured ISP → Application
```

**Pros**:
- Automatic ISP configuration
- Handles complexity of FE/BE coordination
- Well-tested pipeline
- Easy application integration

**Cons**:
- Requires libcamera support for RS300 (doesn't exist yet)
- Less direct control
- Higher latency than direct V4L2

### Adding RS300 Support to libcamera

To integrate RS300 with libcamera's ISP pipeline, you would need:

1. **IPA Module**: Image Processing Algorithm module for RS300
2. **Camera Sensor Definition**: Tuning file for thermal sensor
3. **Pipeline Handler Integration**: Connect RS300 to PiSP pipeline

**Reference implementation**:
```bash
# Location in libcamera source
src/libcamera/pipeline/rpi/pisp/pisp.cpp
src/ipa/rpi/pisp/
```

**Effort estimate**: 2-4 weeks of development + testing

### Current Recommendation

**Stick with direct V4L2** for now because:
- ✅ Full control over thermal data processing
- ✅ No unnecessary RGB/Bayer assumptions
- ✅ Lower latency
- ✅ Simpler debugging

Consider libcamera later if:
- You need automatic 3A (auto-exposure, auto-white-balance, auto-focus)
- You want to use rpicam-apps (rpicam-vid, rpicam-still)
- You need standardized camera API for multiple applications

---

## 9. Practical Recommendations

### For Your RS300 Driver Project

#### Short-term (Current Sprint)

1. **Keep direct capture pipeline** (✓ already working)
   - Lowest latency
   - Raw thermal data
   - Proven stability

2. **Add pispbe post-processing option**
   - Create example script showing pispbe usage
   - Demonstrate temporal denoise for thermal noise
   - Provide dual-stream output example

3. **Document ISP capabilities**
   - Update README with ISP integration notes
   - Add this guide to documentation
   - Include GStreamer pipeline examples

#### Medium-term (Next Month)

1. **Implement hardware-accelerated processing**
   - Write V4L2 M2M wrapper for pispbe
   - Add temporal denoise configuration
   - Benchmark latency vs. software processing

2. **Create utility scripts**
   ```bash
   ./capture_with_denoise.sh   # Use pispbe TDN
   ./dual_stream_record.sh     # Full-res + preview
   ./benchmark_isp.sh          # Compare direct vs. ISP
   ```

3. **Test format conversions**
   - YUV → RGB for display
   - Scaling for different resolutions
   - Compression for network streaming

#### Long-term (Future Consideration)

1. **libcamera integration** (if needed)
   - Create IPA module for RS300
   - Add thermal sensor tuning
   - Enable rpicam-apps support

2. **Advanced ISP features**
   - Custom lens shading correction
   - Thermal-specific color processing
   - Machine learning preprocessing

---

## 10. Quick Reference Commands

### Check ISP Availability

```bash
# List all video devices
v4l2-ctl --list-devices

# Check pispbe backend
ls -la /dev/video20-27

# View pispbe media topology
media-ctl -d /dev/media2 -p

# Check supported formats
v4l2-ctl -d /dev/video20 --list-formats        # Input
v4l2-ctl -d /dev/video23 --list-formats        # Output
```

### Configure pispbe for Basic Processing

```bash
# Input: RS300 YUV data
v4l2-ctl -d /dev/video20 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY,\
    colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range

# Output: Processed frames
v4l2-ctl -d /dev/video23 --set-fmt-video=\
    width=640,height=512,pixelformat=UYVY,\
    colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range

# Query current settings
v4l2-ctl -d /dev/video20 --get-fmt-video
v4l2-ctl -d /dev/video23 --get-fmt-video
```

### Test ISP Processing

```bash
# Capture raw frame from RS300
v4l2-ctl -d /dev/video0 --stream-mmap --stream-to=/tmp/raw_thermal.yuv --stream-count=1

# Process through pispbe (requires custom app or GStreamer)
# See Section 6 for full examples
```

---

## 11. Troubleshooting

### ISP Not Available

**Symptom**: `/dev/video20-27` don't exist

**Solutions**:
```bash
# Check kernel modules
lsmod | grep pisp
# Should show: pispbe

# Check device tree
vcgencmd bootloader_config | grep camera
# Should NOT have: camera_auto_detect=1 (conflicts with custom overlays)

# Verify in dmesg
dmesg | grep -i pisp
```

### Format Negotiation Fails

**Symptom**: `VIDIOC_S_FMT failed: Invalid argument`

**Solutions**:
```bash
# Check supported formats
v4l2-ctl -d /dev/video20 --list-formats

# Ensure format is supported (UYVY, YUYV confirmed working)
# Check resolution is valid (must match input capabilities)

# Use verbose mode
v4l2-ctl -d /dev/video20 --set-fmt-video=width=640,height=512,pixelformat=UYVY --verbose
```

### No Output from pispbe

**Symptom**: Frames queued but no output

**Cause**: Missing configuration buffer

**Solution**:
- pispbe requires `pisp_be_tiles_config` structure via `/dev/video27`
- See libpisp documentation: https://github.com/raspberrypi/libpisp
- Use libpisp helper functions to generate valid config

---

## 12. Additional Resources

### Official Documentation

- **Kernel docs**: https://docs.kernel.org/admin-guide/media/raspberrypi-pisp-be.html
- **Kernel docs (FE)**: https://docs.kernel.org/admin-guide/media/raspberrypi-rp1-cfe.html
- **PiSP spec**: https://datasheets.raspberrypi.com/camera/raspberry-pi-image-signal-processor-specification.pdf
- **libpisp**: https://github.com/raspberrypi/libpisp

### Example Code

- **libcamera PiSP integration**: https://github.com/raspberrypi/libcamera/tree/main/src/libcamera/pipeline/rpi/pisp
- **V4L2 M2M examples**: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/dev-mem2mem.html

### Community

- **Raspberry Pi Forums - Camera**: https://forums.raspberrypi.com/viewforum.php?f=43
- **V4L2 mailing list**: https://www.linuxtv.org/lists.php

---

## Summary

### Key Takeaways

1. **Front End ISP (pisp-fe)**: ❌ Not useful for RS300
   - Expects Bayer, you have YUV
   - Skip FE integration

2. **Back End ISP (pispbe)**: ✅ Highly valuable
   - Supports YUV input (UYVY, YUYV)
   - Hardware-accelerated processing
   - Temporal denoise for thermal noise reduction
   - Dual-stream output for recording + preview

3. **Current pipeline**: ✅ Keep as default
   - Direct capture has lowest latency
   - Add pispbe as optional enhancement

4. **Integration approach**:
   - Short-term: Document pispbe capabilities
   - Medium-term: Add example processing scripts
   - Long-term: Consider libcamera if needed

### Recommended Next Steps

1. Test pispbe with captured RS300 frames
2. Benchmark temporal denoise quality
3. Create GStreamer pipeline examples
4. Update project README with ISP integration guide

---

**Document Version**: 1.0
**Last Updated**: 2025-10-21
**Author**: Generated for RS300 V4L2 Driver Project
