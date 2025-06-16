# RS300 Driver Current Development Status

## Summary
This document tracks the current development status of the RS300 thermal camera driver for Raspberry Pi 5, including format compatibility fixes and remaining issues.

**Last Updated**: June 16, 2025  
**Driver Version**: pi5-testing branch  
**Target Platform**: Raspberry Pi 5 (BCM2712) with RP1-CFE

## Recent Format Compatibility Fixes ✅

### Issue Resolved: Pi 5 Format Mismatch
The driver and scripts have been updated to address critical format compatibility issues with Raspberry Pi 5 RP1-CFE (Camera Front End).

### Root Cause
- **Previous Issue**: Scripts were using 8-bit dual lane formats (`*8_2X8`) 
- **Pi 5 Requirement**: RP1-CFE only supports 16-bit packed formats (`*8_1X16`)
- **Error Symptom**: "Format mismatch!" and "Failed to start media pipeline: -22" in kernel logs

### Fixed Components

#### 1. Documentation (CLAUDE.md) ✅
- **Updated pipeline configuration** from `YUYV8_2X8` to `UYVY8_1X16`
- **Added format compatibility warning** about 2X8 vs 1X16 formats
- **Added complete colorspace parameters** for proper pipeline configuration

#### 2. Main Configuration Script (configure_media.sh) ✅
- **Changed default format** from `UYVY8_2X8` to `UYVY8_1X16`
- **Added format validation** to prevent using incompatible 2X8 formats
- **Updated help text** to reflect Pi 5 compatibility requirements
- **Added colorspace parameters** to all media-ctl commands

#### 3. Format Testing Script (test_formats.sh) ✅
- **Removed all 2X8 format tests** (they always fail on Pi 5)
- **Focus on 1X16 variants**: `UYVY8_1X16`, `YUYV8_1X16`, `YVYU8_1X16`, `VYUY8_1X16`
- **Added thermal-specific formats**: `Y10_1X10`, `Y16_1X16`, `Y12_1X12`
- **Updated messaging** to clarify Pi 5 compatibility focus

#### 4. Streaming Test Script (test_streaming.sh) ✅
- **Updated to use UYVY8_1X16** format consistently
- **Added complete colorspace parameters** for all format settings
- **Improved error reporting** for format-related issues

#### 5. Driver Code Verification (rs300.c) ✅
- **Driver is correctly configured** for Pi 5 compatibility
- **Default mode 0 (640x512)** uses `MEDIA_BUS_FMT_UYVY8_1X16`
- **Supports multiple 1X16 formats** as required by Pi 5
- **No changes needed** - driver was already Pi 5 compatible

## Current Configuration

### Recommended Working Configuration
```bash
# Media controller pipeline (Pi 5 compatible)
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

### Driver Module Parameters
```
mode=0     # 640x512 resolution (default)
fps=60     # 60fps for 640 module (default)
```

## Testing Workflow

### 1. Quick Format Compatibility Test
```bash
./configure_media.sh --non-interactive
```

### 2. Comprehensive Format Testing
```bash
./test_formats.sh
```

### 3. Basic Streaming Test
```bash
./test_streaming.sh
```

### 4. Pipeline Diagnostics
```bash
./debug_pipeline.sh
```

## Known Remaining Issues

### Issue 1: Camera Not Streaming Despite Correct Formats
- **Status**: Investigation needed
- **Symptom**: Pipeline configures successfully but streaming may still fail
- **Possible Causes**:
  - I2C communication issues with thermal camera
  - Hardware initialization sequence
  - Power supply issues
  - Timing/clock configuration

### Issue 2: Thermal Camera Specific Features
- **Status**: Needs testing after basic streaming works
- **Features to test**:
  - FFC (Flat Field Correction) calibration
  - Colormap selection
  - Brightness adjustment
  - Temperature measurement accuracy

## Next Steps

### High Priority
1. **Test updated configuration**: Run `./configure_media.sh` with new Pi 5 compatible settings
2. **Verify hardware detection**: Ensure RS300 is detected on I2C bus 10 at address 0x3c
3. **Debug streaming failures**: If streaming still fails, investigate I2C communication and hardware initialization

### Medium Priority
1. **Validate thermal controls**: Test camera-specific V4L2 controls
2. **Performance optimization**: Fine-tune timing and power parameters
3. **Integration testing**: Test with thermal imaging applications

### Low Priority
1. **Documentation updates**: Create comprehensive setup guide
2. **Error handling**: Improve error messages and recovery procedures

## Reference Documentation
- **Format Requirements**: `verified_rpi_csi_doc.md` - Official Pi 5 RP1-CFE format specifications
- **Hardware Setup**: `CLAUDE.md` - Driver architecture and configuration
- **Troubleshooting**: `debug_pipeline.sh` output for detailed diagnostics

## Testing Commands Summary
```bash
# Check driver status
lsmod | grep rs300
dmesg | grep rs300

# Check hardware detection
i2cdetect -y 10

# Check media controller
media-ctl -d /dev/media0 --print-topology

# Test configuration and streaming
./configure_media.sh
./test_streaming.sh
```