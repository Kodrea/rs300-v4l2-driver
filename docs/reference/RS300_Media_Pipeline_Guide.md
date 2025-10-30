# RS300 Media Pipeline Guide

## Architecture

**Pi 5 pipeline** (RP1-CFE unlike Pi 4 Unicam):
```
RS300 (I2C 0x3c) → MIPI CSI-2 (2-lane, 80MHz) → RP1 Controller → CSI2 Receiver → RP1-CFE → /dev/video0
```

**Pad topology**:
```
RS300 pad 0 → CSI2 pad 0 → CSI2 pad 4 → RP1-CFE pad 0 → /dev/video0
RS300 pad 1 (metadata) → independent
```

| Aspect | Pi 4 (Legacy) | Pi 5 (Media Ctrl) |
|--------|---------------|-------------------|
| Interface | Unicam | RP1-CFE |
| Config | Direct V4L2 | Media Controller API |
| Pipeline | Simple | Multi-stage |
| Format | Automatic | Manual |

---

## Supported Formats & Pads

### RS300 Pads

**Pad 0 (IMAGE)**: YUYV8_2X8 / YUYV8_1X16 / UYVY8_2X8 / UYVY8_1X16, 640×512, 60fps

**Pad 1 (METADATA)**: SENSOR_DATA format

### CSI2

**Pad 0 (input)**: Receives RS300 pad 0 format

**Pad 4 (output)**: Connects to RP1-CFE pad 0

### Format Matrix

| RS300 Format | CSI2 | RP1-CFE | Video Device |
|--------------|------|---------|--------------|
| YUYV8_2X8 | ✓ | ✓ | YUYV |
| YUYV8_1X16 | ✓ | ✓ | YUYV |
| UYVY8_2X8 | ✓ | ✓ | UYVY |
| UYVY8_1X16 | ✓ | ✓ | UYVY |

**CRITICAL**: RP1-CFE only supports **16-bit packed** (*8_1X16), not 8-bit dual lane

---

## Setup

### Prerequisites
```bash
lsmod | grep rs300              # Driver loaded
i2cdetect -y 10                 # Device at 0x3c
ls /dev/media* /dev/video*      # Devices exist
```

### Automated Configuration
```bash
./configure_media.sh --format YUYV8_2X8 --width 640 --height 512
```

### Manual Configuration
```bash
media-ctl -d /dev/media0 -r
media-ctl -d /dev/media0 -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -d /dev/media0 -V "'rs300 10-003c':0 [fmt:YUYV8_2X8/640x512]"
media-ctl -d /dev/media0 -V "'csi2':0 [fmt:YUYV8_2X8/640x512]"
media-ctl -d /dev/media0 -V "'csi2':4 [fmt:YUYV8_2X8/640x512]"
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

### Verification
```bash
media-ctl -d /dev/media0 --get-v4l2 "'rs300 10-003c':0"
media-ctl -d /dev/media0 --get-v4l2 "'csi2':0"
media-ctl -d /dev/media0 --get-v4l2 "'csi2':4"
v4l2-ctl -d /dev/video0 --get-fmt-video
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=5
```

---

## Troubleshooting

### Format Mismatch
- **Issue**: Streaming fails, different formats at each stage
- **Check**: `./debug_pipeline.sh --visualize`
- **Fix**: Reset & reconfigure, verify each stage

### Link Disabled
- **Issue**: Links show DISABLED, no video device
- **Check**: `media-ctl --print-topology | grep ENABLED`
- **Fix**: Enable critical links manually, check device tree

### I2C Not Detected
- **Issue**: RS300 not found at 0x3c
- **Check**: `i2cdetect -y 10`, `i2cget -y 10 0x3c 0x00`
- **Fix**: Hardware connections, power supply, I2C speed

### Streaming Fails
- **Issue**: Video capture timeout, buffer allocation error
- **Check**: `v4l2-ctl --stream-mmap --stream-count=1 --verbose`, `dmesg | grep -E "(rs300|rp1-cfe)"`
- **Fix**: Verify full pipeline, memory, buffer count

---

## Quick Reference

### Essential Commands
```bash
./debug_pipeline.sh
./configure_media.sh --format YUYV8_2X8
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=5
media-ctl -d /dev/media0 -r
```

### Checklist
- [ ] Driver loaded
- [ ] I2C device detected
- [ ] Media device present
- [ ] Pipeline configured
- [ ] Links enabled
- [ ] Formats compatible
- [ ] Streaming works

---

**Reference Guide** | **Last Updated**: 2025-10-21
