# RS300 Media Pipeline Setup and Troubleshooting Guide

## Raspberry Pi 5 Camera Driver Configuration and Visualization

---

## Table of Contents

1. [Media Controller Architecture Overview](#1-media-controller-architecture-overview)
2. [Media Pipeline Visualization](#2-media-pipeline-visualization)
3. [Supported Formats and Pads](#3-supported-formats-and-pads)
4. [Step-by-Step Setup Process](#4-step-by-step-setup-process)
5. [Troubleshooting Common Issues](#5-troubleshooting-common-issues)
6. [Advanced Configuration](#6-advanced-configuration)
7. [Visualization Tools](#7-visualization-tools)
8. [Integration with Existing Scripts](#8-integration-with-existing-scripts)

---

## 1. Media Controller Architecture Overview

### Pi 5 RP1-CFE Pipeline Architecture

The Raspberry Pi 5 uses a fundamentally different camera architecture compared to previous Pi models:

```
RS300 Thermal Camera → I2C Bus 10 (0x3c)
         ↓
MIPI CSI-2 Interface (2-lane, 80MHz)
         ↓
RP1 Controller (BCM2712)
         ↓
CSI2 Receiver → RP1-CFE → /dev/video0
```

### Media Controller vs Legacy V4L2

| Aspect | Pi 4 (Legacy) | Pi 5 (Media Controller) |
|--------|---------------|-------------------------|
| Interface | Unicam | RP1-CFE |
| Configuration | Direct V4L2 | Media Controller API |
| Pipeline | Simple | Multi-stage |
| Format Negotiation | Automatic | Manual |
| Debug Complexity | Low | High |

### Pad Topology: RS300 → CSI2 → RP1-CFE → /dev/video0

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   RS300     │    │    CSI2     │    │  RP1-CFE    │    │ /dev/video0 │
│             │    │             │    │             │    │             │
│   pad 0 ────┼────┼─→ pad 0     │    │             │    │             │
│   (image)   │    │   (input)   │    │             │    │             │
│             │    │             │    │             │    │             │
│   pad 1     │    │   pad 4 ────┼────┼─→ pad 0     │    │             │
│   (metadata)│    │   (output)  │    │   (input)   │    │             │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
```

---

## 2. Media Pipeline Visualization

### Python Script for DOT File Generation

The visualization system converts media controller topology into visual diagrams:

1. **Parse media-ctl topology** → Extract entities, pads, links
2. **Generate DOT graph** → Create Graphviz-compatible format
3. **Render PNG diagram** → Visual pipeline representation
4. **Status overlay** → Show current formats and link states

### Visualization Features

- **Color-coded status**: Green (working), Red (error), Yellow (warning)
- **Format propagation**: Shows data flow through pipeline
- **Link status**: ENABLED/DISABLED visual indicators
- **Interactive troubleshooting**: Click-to-diagnose capabilities

### Example Output

```
RS300 Media Pipeline Visualization
==================================
┌─────────────────────────────────────────────────────────────┐
│                    Media Controller /dev/media0             │
├─────────────────────────────────────────────────────────────┤
│  [RS300]────────[CSI2]────────[RP1-CFE]────────[VIDEO0]    │
│   640x512        640x512       640x512         640x512     │
│   YUYV8_2X8      YUYV8_2X8     YUYV8_2X8       YUYV        │
│   🟢 ACTIVE      🟢 ENABLED     🟢 STREAMING    🟢 READY    │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Supported Formats and Pads

### RS300 Pad Configuration

#### Pad 0 (IMAGE_PAD)
- **Primary format**: `YUYV8_2X8` (640x512@60fps)
- **Alternative formats**: `YUYV8_1X16`, `UYVY8_2X8`, `UYVY8_1X16`
- **Resolution**: 640x512 (fixed for Pi 5 configuration)
- **Frame rate**: 60fps

#### Pad 1 (METADATA_PAD)
- **Format**: `SENSOR_DATA`
- **Purpose**: Thermal calibration and metadata
- **Size**: Variable

### CSI2 Pad Configuration

#### Input Pad (0)
- **Receives from**: RS300 pad 0
- **Format**: Must match RS300 output
- **Validation**: Automatic format propagation

#### Output Pad (4)
- **Connects to**: RP1-CFE pad 0
- **Format**: Must match input pad
- **Link**: Critical for video streaming

### Format Compatibility Matrix

| RS300 Format | CSI2 Compatible | RP1-CFE Compatible | Video Device |
|--------------|-----------------|-------------------|--------------|
| YUYV8_2X8    | ✅ Yes          | ✅ Yes            | YUYV         |
| YUYV8_1X16   | ✅ Yes          | ✅ Yes            | YUYV         |
| UYVY8_2X8    | ✅ Yes          | ✅ Yes            | UYVY         |
| UYVY8_1X16   | ✅ Yes          | ✅ Yes            | UYVY         |

---

## 4. Step-by-Step Setup Process

### Prerequisites Check

```bash
# 1. Verify driver is loaded
lsmod | grep rs300

# 2. Check I2C communication
i2cdetect -y 10

# 3. Verify media controller device
ls /dev/media*

# 4. Check video device
ls /dev/video*
```

### Automated Configuration

```bash
# Use enhanced configure script
./configure_media.sh --format YUYV8_2X8 --width 640 --height 512

# With visualization
./configure_media.sh --format YUYV8_2X8 --visualize
```

### Manual Configuration Commands

```bash
# Step 1: Reset media controller
media-ctl -d /dev/media0 -r

# Step 2: Enable critical link
media-ctl -d /dev/media0 -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Step 3: Set RS300 format
media-ctl -d /dev/media0 -V "'rs300 10-003c':0 [fmt:YUYV8_2X8/640x512]"

# Step 4: Set CSI2 input format
media-ctl -d /dev/media0 -V "'csi2':0 [fmt:YUYV8_2X8/640x512]"

# Step 5: Set CSI2 output format
media-ctl -d /dev/media0 -V "'csi2':4 [fmt:YUYV8_2X8/640x512]"

# Step 6: Set video device format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

### Pipeline Verification

```bash
# Check all formats
media-ctl -d /dev/media0 --get-v4l2 "'rs300 10-003c':0"
media-ctl -d /dev/media0 --get-v4l2 "'csi2':0"
media-ctl -d /dev/media0 --get-v4l2 "'csi2':4"
v4l2-ctl -d /dev/video0 --get-fmt-video

# Test streaming
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=5
```

---

## 5. Troubleshooting Common Issues

### Format Mismatch Diagnostics

#### Symptoms
- Streaming fails with format errors
- Pipeline shows different formats at each stage

#### Diagnosis
```bash
# Generate visual pipeline status
./debug_pipeline.sh --visualize

# Check format propagation
./media-topology-visualizer.py --check-formats
```

#### Solutions
1. **Reset and reconfigure pipeline**
2. **Use compatible format combinations**
3. **Verify each stage individually**

### Link Configuration Failures

#### Symptoms
- Links show as DISABLED
- No video device available
- CSI2 connection errors

#### Diagnosis
```bash
# Check link status
media-ctl -d /dev/media0 --print-topology | grep ENABLED

# Visual link status
./media-topology-visualizer.py --show-links
```

#### Solutions
1. **Enable critical links manually**
2. **Check device tree overlay**
3. **Verify RP1-CFE driver loading**

### I2C Communication Problems

#### Symptoms
- RS300 not detected at 0x3c
- Format setting fails
- Control commands timeout

#### Diagnosis
```bash
# Hardware detection
i2cdetect -y 10

# Communication test
i2cget -y 10 0x3c 0x00

# Visual I2C status
./debug_pipeline.sh --check-i2c
```

#### Solutions
1. **Check hardware connections**
2. **Verify power supply**
3. **Test with different I2C speeds**

### Streaming Failures

#### Symptoms
- Video capture fails
- Timeout errors
- Buffer allocation issues

#### Diagnosis
```bash
# Streaming test with debug
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --verbose

# Check kernel messages
dmesg | grep -E "(rs300|rp1-cfe|csi2)"
```

#### Solutions
1. **Verify complete pipeline configuration**
2. **Check memory allocation**
3. **Test with different buffer counts**

---

## 6. Advanced Configuration

### Custom Format Support

#### Adding New Formats
1. **Modify driver source** (rs300.c)
2. **Update supported_modes array**
3. **Add format validation**
4. **Test compatibility**

#### Format Testing Script
```bash
# Test all available formats
./test_formats.sh --comprehensive

# Test specific format combination
./test_formats.sh --format YUYV8_1X16 --test-streaming
```

### Multiple Format Testing

#### Automated Testing
```bash
# Test format matrix
for fmt in YUYV8_2X8 YUYV8_1X16 UYVY8_2X8; do
    ./configure_media.sh --format $fmt --test
done
```

#### Performance Optimization

1. **Link frequency tuning**
2. **Buffer management**
3. **Clock optimization**
4. **Power management**

### Debug Techniques

#### Kernel Message Analysis
```bash
# Enable debug messages
echo 8 > /proc/sys/kernel/printk

# Monitor in real-time
dmesg -wH | grep -E "(rs300|media|v4l2)"
```

#### V4L2 Debug Tools
```bash
# List all controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls

# Monitor events
v4l2-ctl -d /dev/video0 --wait-for-event=vsync
```

---

## 7. Visualization Tools

### media-topology-visualizer.py

#### Features
- **Parse media-ctl output** → Extract topology information
- **Generate DOT graphs** → Create Graphviz-compatible files
- **Render PNG diagrams** → Visual pipeline representation
- **Status overlays** → Show current configuration state

#### Usage
```bash
# Basic visualization
python3 media-topology-visualizer.py

# With format checking
python3 media-topology-visualizer.py --check-formats

# Generate multiple formats
python3 media-topology-visualizer.py --all-formats
```

### Pipeline Status Overlay

#### Visual Elements
- **Entity boxes** with current formats
- **Link arrows** with status colors
- **Error indicators** for problematic connections
- **Format compatibility** warnings

### Format Flow Diagrams

#### Data Path Visualization
```
┌─────────────┐ YUYV8_2X8 ┌─────────────┐ YUYV8_2X8 ┌─────────────┐
│    RS300    │─────────→ │    CSI2     │─────────→ │  RP1-CFE    │
│   640x512   │           │   640x512   │           │   640x512   │
│   60fps     │           │             │           │             │
└─────────────┘           └─────────────┘           └─────────────┘
                                                           │
                                                           │ YUYV
                                                           ▼
                                                   ┌─────────────┐
                                                   │ /dev/video0 │
                                                   │   640x512   │
                                                   └─────────────┘
```

### Error State Visualization

#### Problem Identification
- **Red entities**: Configuration errors
- **Yellow links**: Warnings or suboptimal settings
- **Dashed lines**: Disabled connections
- **Error annotations**: Specific problem descriptions

---

## 8. Integration with Existing Scripts

### Enhanced debug_pipeline.sh

#### New Features
- **PNG output generation**
- **Visual status reports**
- **Interactive problem diagnosis**
- **Format compatibility checking**

#### Usage
```bash
# Generate visual debug report
./debug_pipeline.sh --visual-report

# Save topology diagram
./debug_pipeline.sh --save-topology debug_output.png
```

### Extended configure_media.sh

#### Visualization Integration
- **Real-time pipeline status**
- **Configuration validation diagrams**
- **Success/failure visualization**
- **Working configuration snapshots**

#### Usage
```bash
# Configure with visualization
./configure_media.sh --visualize

# Save working configuration diagram
./configure_media.sh --save-config working_setup.png
```

### Visual Validation in test_formats.sh

#### Testing Enhancements
- **Format compatibility matrix**
- **Visual test results**
- **Performance comparison charts**
- **Error pattern analysis**

#### Usage
```bash
# Test with visual output
./test_formats.sh --visual

# Generate compatibility matrix
./test_formats.sh --matrix-png
```

### Working Configuration Documentation

#### Automatic Documentation
- **Generate working_config.png** for successful setups
- **Create troubleshooting flowcharts**
- **Document format decisions**
- **Provide visual setup guides**

---

## Quick Reference

### Essential Commands

```bash
# Check system status
./debug_pipeline.sh

# Configure pipeline
./configure_media.sh --format YUYV8_2X8

# Test streaming
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=5

# Generate visualization
python3 media-topology-visualizer.py

# Reset pipeline
media-ctl -d /dev/media0 -r
```

### Common Format Combinations

| Use Case | RS300 Format | Video Format | Notes |
|----------|--------------|--------------|-------|
| Standard | YUYV8_2X8 | YUYV | Recommended |
| High-performance | YUYV8_1X16 | YUYV | Faster processing |
| Alternative | UYVY8_2X8 | UYVY | Different byte order |

### Troubleshooting Checklist

- [ ] Driver loaded (`lsmod | grep rs300`)
- [ ] I2C communication (`i2cdetect -y 10`)
- [ ] Media device available (`ls /dev/media*`)
- [ ] Pipeline configured (`./configure_media.sh`)
- [ ] Links enabled (`media-ctl --print-topology`)
- [ ] Formats compatible (`./debug_pipeline.sh`)
- [ ] Streaming functional (`v4l2-ctl --stream-mmap`)

---

*This guide provides comprehensive setup and troubleshooting procedures for the RS300 thermal camera on Raspberry Pi 5, with visual diagnostic capabilities for effective problem resolution.*