# RS300 Media Topology Analysis

## Summary
Analysis of actual media controller topology for RS300 thermal camera on Raspberry Pi 5 CSI-2 camera port 0.

**Date**: June 16, 2025  
**Media Device**: `/dev/media1`  
**CSI Port**: Camera 0 (22-pin connector with 15-pin adapter)

## Actual Media Controller Topology

### Media Device Information
```
driver          rp1-cfe
model           rp1-cfe
serial          
bus info        platform:1f00110000.csi
hw revision     0x114666
driver version  6.12.25
```

### Complete Data Path for Camera Port 0

```
[RS300 Sensor] (entity 16: rs300 10-003c)
    pad0: Source → CSI-2 pad0 (main video data)
    pad1: Source → CSI-2 pad1 (metadata)
           ↓
[CSI-2 Receiver] (entity 1: csi2)
    pad0: Sink ← RS300 pad0 (video input)
    pad1: Sink ← RS300 pad1 (metadata input)
    pad4: Source → rp1-cfe-csi2_ch0 pad0 (video output)
    pad5: Source → rp1-cfe-embedded pad0 (metadata output)
           ↓
[RP1-CFE Channel 0] (entity 19: rp1-cfe-csi2_ch0)
    pad0: Sink ← CSI-2 pad4
           ↓
[Video Device] /dev/video0
```

### Pad Assignment for Camera Port 0

| Component | Entity Name | Pad | Type | Purpose | Connected To |
|-----------|-------------|-----|------|---------|--------------|
| RS300 | `'rs300 10-003c'` | 0 | Source | Main video data | `'csi2':0` |
| RS300 | `'rs300 10-003c'` | 1 | Source | Metadata | `'csi2':1` |
| CSI-2 | `'csi2'` | 0 | Sink | Video input | From RS300:0 |
| CSI-2 | `'csi2'` | 1 | Sink | Metadata input | From RS300:1 |
| CSI-2 | `'csi2'` | 4 | Source | Video output | To `'rp1-cfe-csi2_ch0':0` |
| CSI-2 | `'csi2'` | 5 | Source | Metadata output | To `'rp1-cfe-embedded':0` |
| RP1-CFE | `'rp1-cfe-csi2_ch0'` | 0 | Sink | Video capture | From `'csi2':4` |

### Other CSI-2 Channels (Camera Port 1, etc.)
- **CSI-2 pad 6** → `'rp1-cfe-csi2_ch2'` → `/dev/video2`
- **CSI-2 pad 7** → `'rp1-cfe-csi2_ch3'` → `/dev/video3`

These would be used for additional camera modules on different CSI ports.

## Script Validation

### ✅ Correct Pad Numbers (configure_media.sh was right!)
The script's hardcoded pad numbers were actually correct for camera port 0:

```bash
# These pad assignments are CORRECT for camera port 0:
media-ctl -V "'rs300 10-003c':0"     # RS300 main output
media-ctl -V "'csi2':0"              # CSI-2 video input 
media-ctl -V "'csi2':1"              # CSI-2 metadata input
media-ctl -V "'csi2':4"              # CSI-2 video output
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"  # Critical link
```

### ❌ Issues Found and Fixed

#### 1. Media Device Discovery
- **Issue**: Script searches `/dev/media0` first
- **Reality**: RS300 is on `/dev/media1`
- **Fix**: Script logic was already correct (searches all devices), just needed better error reporting

#### 2. Link Not Enabled
- **Issue**: Critical link `'csi2':4 -> 'rp1-cfe-csi2_ch0':0` was disabled by default
- **Fix**: Enable with `[1]` flag in media-ctl command

#### 3. Format Mismatch
- **Issue**: CSI-2 had default format `SRGGB10_1X10/640x480` instead of RS300's `UYVY8_1X16/640x512`
- **Fix**: Set consistent format throughout pipeline

## Current Status

### ✅ Fixed Issues
- Media device discovery enhanced with better validation
- Correct pad numbers confirmed and documented
- Format pipeline properly configured
- Critical link enabled

### ❌ Remaining Issue
**Error -32: "Failed to start media pipeline"** - Still investigating root cause

### Working Configuration Commands
```bash
# Enable link
media-ctl -d /dev/media1 -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set formats
media-ctl -d /dev/media1 -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -d /dev/media1 -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set video device
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

## Conclusion

**Your original question about pad selection was excellent** - it revealed that while the pad numbers were correct, there were critical issues with:
1. Media device discovery
2. Link enablement 
3. Format consistency

The configure_media.sh script's pad assignments were actually correct for CSI-2 camera port 0. The issues were in the pipeline configuration logic, not the pad mappings.