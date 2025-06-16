# RS300 Media Topology Analysis

## Summary
Analysis of working media controller topology for RS300 thermal camera on Raspberry Pi 5 CSI-2 camera port 0.

**Date**: June 16, 2025  
**Media Device**: `/dev/media0` ✅ WORKING  
**CSI Port**: Camera 0 (22-pin connector with 15-pin adapter)  
**Status**: 🎉 **STREAMING SUCCESSFULLY**

## Current Working Setup

### Media Device Information
```
driver          rp1-cfe
model           rp1-cfe
serial          
bus info        platform:1f00110000.csi
hw revision     0x114666
driver version  6.12.25
```

### Active Data Path (Working Configuration)

```
[RS300 Sensor] (entity: rs300 10-003c)
    pad0: Source → CSI-2 pad0 (YUYV8_1X16/640x512)
    pad1: Source → CSI-2 pad1 (metadata)
           ↓
[CSI-2 Receiver] (entity: csi2)
    pad0: Sink ← RS300 pad0 (video input)
    pad1: Sink ← RS300 pad1 (metadata input)
    pad4: Source → rp1-cfe-csi2_ch0 pad0 [ENABLED] ✅
    pad5: Source → rp1-cfe-embedded pad0 (metadata)
           ↓
[RP1-CFE CSI2 Channel 0] (entity: rp1-cfe-csi2_ch0)
    pad0: Sink ← CSI-2 pad4
           ↓
[Video Device] /dev/video0 → 🎥 **60fps thermal streaming**
```

### Current Working Format Configuration

| Component | Entity Name | Pad | Format | Resolution | Status |
|-----------|-------------|-----|--------|------------|--------|
| RS300 | `'rs300 10-003c'` | 0 | YUYV8_1X16 | 640x512 | ✅ Active |
| RS300 | `'rs300 10-003c'` | 1 | YUYV8_1X16 | 16384x1 | ✅ Metadata |
| CSI-2 | `'csi2'` | 0 | YUYV8_1X16 | 640x512 | ✅ Input |
| CSI-2 | `'csi2'` | 1 | YUYV8_1X16 | 16384x1 | ✅ Metadata |
| CSI-2 | `'csi2'` | 4 | YUYV8_1X16 | 640x512 | ✅ Output |
| CSI-2 | `'csi2'` | 5 | YUYV8_1X16 | 16384x1 | ✅ Metadata |
| Video | `/dev/video0` | - | YUYV 4:2:2 | 640x512 | 🎥 Streaming |

### Pad Assignment Reference

| Component | Entity Name | Pad | Type | Purpose | Connected To |
|-----------|-------------|-----|------|---------|--------------| 
| RS300 | `'rs300 10-003c'` | 0 | Source | Main video data | `'csi2':0` ✅ |
| RS300 | `'rs300 10-003c'` | 1 | Source | Metadata | `'csi2':1` ✅ |
| CSI-2 | `'csi2'` | 0 | Sink | Video input | From RS300:0 |
| CSI-2 | `'csi2'` | 1 | Sink | Metadata input | From RS300:1 |
| CSI-2 | `'csi2'` | 4 | Source | Video output | To `'rp1-cfe-csi2_ch0':0` ✅ |
| CSI-2 | `'csi2'` | 5 | Source | Metadata output | To `'rp1-cfe-embedded':0` |
| RP1-CFE | `'rp1-cfe-csi2_ch0'` | 0 | Sink | Video capture | From `'csi2':4` |

### Alternative Paths Available

#### ISP Processing Path (Available but Not Currently Used)
```
[RS300] → [CSI-2] → [PISP-FE] → [rp1-cfe-fe_image0] → /dev/video4
```
- **Purpose**: Hardware denoising and image enhancement
- **Benefits**: Spatial/temporal denoising for thermal images
- **Setup**: Requires different media controller link configuration

#### Additional CSI-2 Channels (Unused)
- **CSI-2 pad 6** → `'rp1-cfe-csi2_ch2'` → `/dev/video2`
- **CSI-2 pad 7** → `'rp1-cfe-csi2_ch3'` → `/dev/video3`
- **Purpose**: Additional camera modules or multi-stream capture

## Configure Media Script Integration

### ✅ Working Configuration Commands

The current working setup can be reproduced with:

```bash
# Automatic configuration with format selection
./configure_media.sh

# Non-interactive YUYV setup
./configure_media.sh --format YUYV8_1X16 --pixelformat YUYV --non-interactive

# With visualization
./configure_media.sh --format YUYV8_1X16 --pixelformat YUYV --visualize
```

### Current Working Pipeline Commands
```bash
# Enable critical link
media-ctl -d /dev/media0 -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set consistent YUYV formats throughout pipeline
media-ctl -d /dev/media0 -V "'csi2':0 [fmt:YUYV8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -d /dev/media0 -V "'csi2':4 [fmt:YUYV8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Configure video device
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV,colorspace=smpte170m,xfer=709,ycbcr:601,quantization=lim-range
```

### Format Support
- **Current Working**: YUYV8_1X16 (YUYV 4:2:2)
- **Also Supported**: UYVY8_1X16 (UYVY 4:2:2)
- **Pi 5 Requirement**: 16-bit packed formats only (`*_1X16`)
- **Not Supported**: 8-bit dual lane formats (`*_2X8`) cause format mismatch errors

## Testing and Validation

### Stream Testing Commands
```bash
# Basic stream test
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10

# Live viewing
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0

# GStreamer pipeline
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=512 ! videoconvert ! autovideosink
```

### V4L2 Controls
```bash
# List available controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls

# Trigger FFC calibration  
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=ffc_trigger=0

# Adjust brightness
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=brightness=50
```

## Success Metrics

### ✅ Current Working Status
- **Driver Loading**: RS300 module loads successfully
- **I2C Communication**: Device responds on i2c-10 bus at 0x3c
- **Media Pipeline**: Properly configured and validated
- **Format Consistency**: YUYV8_1X16 throughout entire pipeline
- **Link Status**: Critical CSI2→RP1-CFE link enabled
- **Streaming**: 60fps thermal video capture working
- **Controls**: FFC calibration and settings accessible

### Performance Characteristics
- **Resolution**: 640x512 pixels
- **Frame Rate**: 60fps sustained
- **Format**: YUV 4:2:2 (thermal imaging data)
- **Latency**: Direct CSI2 capture (minimal processing delay)
- **Bandwidth**: ~40MB/s (640×512×2bytes×60fps)

## Future Enhancements

### ISP Integration (Planned)
- **Denoising**: Hardware spatial/temporal noise reduction
- **Enhancement**: Contrast and sharpening for thermal images
- **Pipeline**: CSI2 → PISP-FE → /dev/video4 configuration

### Multi-Format Support
- **Interactive Selection**: UYVY/YUYV choice in configure script
- **Automatic Detection**: RS300 format auto-detection working
- **Validation**: Format mismatch prevention and error reporting