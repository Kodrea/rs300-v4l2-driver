# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
This is a Linux V4L2 driver for the RS300 thermal camera module (also known as "mini2") that interfaces with Raspberry Pi 5 via MIPI CSI-2. The driver is configured for the 640x512 module running at 60fps through the V4L2 subsystem using the BCM2712 architecture.


## Development Workflow
The recommended development workflow uses DKMS (Dynamic Kernel Module Support):

1. **Install/reinstall driver**: `./setup.sh` (handles DKMS installation and rebuilding)
2. **Check DKMS status**: `dkms status`
3. **View loaded modules**: `lsmod | grep rs300`
4. **Debug kernel messages**: `dmesg | grep rs300` or `dmesg -wH`

## Driver Architecture
- **Main driver file**: `rs300.c` - Complete V4L2 subdevice driver implementation
- **Device tree overlay**: `rs300-overlay.dts` - Pi 5 BCM2712 hardware description for I2C and MIPI CSI-2 interface
- **DKMS configuration**: `dkms.conf` and `dkms.postinst` - Dynamic module building setup
- **I2C address**: 0x3c (thermal camera communication on i2c-6 bus)
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
- **Main video device**: `/dev/video0` (Unicam bridge)
- **Subdevice controls**: `/dev/v4l-subdev0` (camera-specific controls)
- **I2C bus**: `/dev/i2c-6` (Pi 5 specific)
- **Test video capture**: Use v4l2-ctl to set 640x512 format and gstreamer for live view
- **Available controls**: FFC calibration, colormap selection, brightness adjustment

## Raspberry Pi 5 Specific Notes
- Uses BCM2712 architecture with dedicated CSI0 port
- I2C communication on bus 6 (i2c-6)
- Configured for 640x512@60fps thermal imaging
- Requires 22-pin to 15-pin adapter cable for camera module connection