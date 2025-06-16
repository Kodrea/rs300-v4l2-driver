# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
This is a Linux V4L2 driver for the RS300 thermal camera module (also known as "mini2") that interfaces with Raspberry Pi 5 via MIPI CSI-2. The driver is configured for the 640x512 module running at 60fps through the V4L2 subsystem using the BCM2712 architecture.


## Development Workflow
The recommended development workflow uses DKMS (Dynamic Kernel Module Support):

1. **Install/reinstall driver**: `./setup.sh` (handles DKMS installation and rebuilding)
2. **Reboot**: `sudo reboot`
3. **Check DKMS status**: `dkms status`
4. **View loaded modules**: `lsmod | grep rs300`
5. **Debug kernel messages**: `dmesg | grep rs300` or `dmesg -wH`

## Driver Architecture
- **Main driver file**: `rs300.c` - Complete V4L2 subdevice driver implementation
- **Device tree overlay**: `rs300-overlay.dts` - Pi 5 BCM2712 hardware description for I2C and MIPI CSI-2 interface
- **DKMS configuration**: `dkms.conf` and `dkms.postinst` - Dynamic module building setup
- **I2C address**: 0x3c (thermal camera communication on i2c-10 bus)
- **MIPI interface**: Uses 2-lane CSI-2 with 80MHz link frequency on csi0 port

## Key Configuration Parameters
The driver is pre-configured for 640x512 module:
```c
static int mode = 0; //0-640 (configured for Pi 5)
static int fps = 60; //60fps for 640 module
```

## Device Tree Integration
The driver requires the device tree overlay to be enabled in `/boot/firmware/config.txt`:
```
camera_auto_detect=0
dtoverlay=rs300
```

## V4L2 Controls and Testing
- **Main video device**: `/dev/video0` (rp1-cfe platform)
- **Subdevice controls**: `/dev/v4l-subdev2` (camera-specific controls)
- **I2C bus**: `/dev/i2c-10` (Pi 5 specific - actual bus used)
- **Media controller**: Required for Pi 5 - use `media-ctl` to configure pipeline
- **Available controls**: FFC calibration, colormap selection, brightness adjustment

## Media Controller Pipeline Configuration
Pi 5 requires media controller API setup before video capture:

**IMPORTANT**: Pi 5 RP1-CFE only supports 16-bit packed formats (`*8_1X16`), not 8-bit dual lane formats (`*8_2X8`).

```bash
# Enable media links
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set formats throughout pipeline (using Pi 5 compatible 16-bit packed format)
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set video device format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

## Raspberry Pi 5 Specific Notes
- Uses BCM2712 architecture with dedicated CSI0 port via RP1 controller
- I2C communication on bus 10 (i2c-10) through i2c_csi_dsi0
- Configured for 640x512@60fps thermal imaging
- Requires 22-pin to 15-pin adapter cable for camera module connection
- Uses rp1-cfe (Camera Front End) driver instead of legacy Unicam
- **Format Compatibility**: Only supports 16-bit packed formats (`UYVY8_1X16`, `YUYV8_1X16`) - 8-bit dual lane formats (`*8_2X8`) will cause "Format mismatch!" errors