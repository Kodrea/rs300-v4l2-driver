# Changelog

All notable changes and updates to the RS300 thermal camera driver project.

## Format

This changelog follows a date-based format with notable announcements and feature additions.

---

## [Unreleased]

### In Development
- ISP integration for hardware denoising (Pi 5)
- Runtime resolution switching (currently requires driver rebuild)
- 256×192 module MIPI video support (Pi 4)
- Additional scene mode presets

---

## 2025

### May 2, 2025

**🎉 Hardware Now Available for Purchase**

The RS300 Mini2 thermal camera modules with custom Raspberry Pi adapter boards are now available for purchase from Purple River Technology.

**What's New**:
- Direct purchase link available at [thermal-image.com](https://www.thermal-image.com/product/mini2-640x512-9mm-thermal-imaging-camera-module-for-drones/)
- Currently available: 640×512 module with 9mm lens
- Custom RPi adapter board included with discount code
- Other resolutions (384×288, 256×192) available via customer support
- Alternative lens options (15mm, 25mm, custom) available on request

**Discount Code**: `HXZUK8WG`
- **Savings**: $30 USD off
- **Important**: Ensures order includes custom Raspberry Pi adapter board (not standard board)

**Customer Support**:
- Excellent support from Purple River Technology
- Contact for different configurations, lens options, or technical assistance

**Tutorial Videos**:
- Setup and usage videos coming soon
- Available at: [linktr.ee/kodrea](https://linktr.ee/kodrea)

---

### March 27, 2025

**📢 Announcement: MIPI CSI-2 Boards Coming Soon**

Custom MIPI CSI-2 adapter boards for Raspberry Pi will be available from Purple River Technology in approximately two weeks.

**Expected Availability**: Mid-April 2025

**What This Enables**:
- Proper Raspberry Pi connectivity via standard 15-pin FPC connector
- 60fps operation with 640×512 modules
- Compatibility with Pi 5 (via 22-pin to 15-pin adapter)
- Standard camera cable support (reverse head FPC)

---

## 2024

### Late 2024

**✅ Raspberry Pi 5 Support - FULLY WORKING**

Complete Raspberry Pi 5 support with RP1-CFE camera system.

**Features Added**:
- Media controller pipeline integration
- Automatic configuration script (`configure_media.sh`)
- 16-bit packed format support (UYVY8_1X16, YUYV8_1X16)
- I2C communication on i2c-10 bus
- Full 60fps thermal streaming at 640×512
- All 11 V4L2 controls accessible

**Technical Details**:
- Uses RP1-CFE Camera Front End (not legacy Unicam)
- Dedicated CSI0 port via RP1 controller
- Requires 22-pin to 15-pin adapter cable

**Status**: Production ready

---

### Mid 2024

**✅ Raspberry Pi 4 Support - WORKING**

Raspberry Pi 4B support with legacy Unicam driver.

**Features Added**:
- DKMS-based driver installation
- Device tree overlay for Pi 4
- 60fps support for 640×512 and 384×288 modules
- Legacy Unicam integration
- V4L2 control interface

**Tested Configurations**:
- 640×512 @ 60fps - ✅ Working
- 384×288 @ 60fps - ✅ Working
- 256×192 @ 50fps - ⚠️ MIPI troubleshooting in progress

**Known Issues**:
- Low-voltage warnings possible with 640×512 (rarely causes issues)
- Requires manual driver configuration before building
- 256×192 MIPI video not streaming (I2C works, CVBS works)

---

### Early 2024

**🚀 Initial Release**

First public release of RS300 V4L2 kernel driver.

**Features**:
- Custom V4L2 kernel driver for RS300 Mini2 thermal camera
- I2C protocol implementation (18-byte packets with CRC-16)
- 14 camera commands via I2C
- 11 V4L2 controls (brightness, colormap, FFC, zoom, etc.)
- MIPI CSI-2 interface support
- Multiple resolution support (640×512, 384×288, 256×192)
- Frame rate options (25/30/50/60 fps)

**Supported Formats**:
- YUV 4:2:2 (YUYV, UYVY)
- 8-bit and 16-bit modes

**Initial Platform Support**:
- Raspberry Pi 4B (primary target)

---

## Credits & Acknowledgments

### Based On

**Setup Script**:
- Credit: [will127534](https://github.com/will127534) for setup script from imx294 driver
- Source: [will127534/IMX294-RPi5](https://github.com/will127534)

**Driver Template**:
- Based on: Raspberry Pi IMX219 driver
- Source: [raspberrypi/linux - drivers/media/i2c/imx219.c](https://github.com/raspberrypi/linux/blob/rpi-6.6.y/drivers/media/i2c/imx219.c)

### Hardware Partner

**Purple River Technology**:
- Custom Raspberry Pi adapter board design
- Excellent customer support
- Hardware testing collaboration
- Store: [thermal-image.com](https://www.thermal-image.com/)
- Alibaba: [Purple River Store](https://purpleriver.en.alibaba.com/)

### Testing & Development

**Community Contributors**:
- Beta testers on Raspberry Pi 4 and Pi 5
- Issue reporters and feedback providers
- Documentation reviewers

---

## Version History

### Driver Versioning

**Current Version**: 0.0.1 (DKMS module)

**Version Scheme**: Major.Minor.Patch
- **Major**: Breaking changes, major feature additions
- **Minor**: New features, non-breaking changes
- **Patch**: Bug fixes, minor improvements

### Future Releases

**Planned for 0.1.0**:
- Runtime resolution switching
- ISP integration (Pi 5)
- Automated FFC scheduling
- Python SDK/bindings

**Planned for 0.2.0**:
- 256×192 MIPI support (Pi 4)
- Additional camera commands
- Performance optimizations
- Extended control interface

---

## Development Milestones

### Completed ✅

- [x] Basic I2C communication protocol
- [x] CRC-16-CCITT implementation
- [x] V4L2 subdevice integration
- [x] MIPI CSI-2 streaming (Pi 4 & 5)
- [x] 11 V4L2 controls
- [x] DKMS installation system
- [x] Device tree overlays
- [x] Media controller support (Pi 5)
- [x] Automatic configuration script
- [x] Comprehensive documentation
- [x] Testing scripts
- [x] Boot automation options

### In Progress 🚧

- [ ] ISP hardware acceleration (Pi 5)
- [ ] 256×192 MIPI troubleshooting (Pi 4)
- [ ] Runtime configuration improvements
- [ ] Video tutorial series

### Planned 📋

- [ ] Python SDK
- [ ] OpenCV integration examples
- [ ] GStreamer element
- [ ] Temperature measurement support (radiometric variant)
- [ ] Multi-camera support
- [ ] Advanced processing examples

---

## Migration Guide

### From Manual to Automated Setup (Pi 5)

**Old Method** (manual media pipeline configuration):
```bash
# Manual link and format configuration
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 ...]"
# ... more commands
```

**New Method** (automated):
```bash
./configure_media.sh
```

**Benefits**:
- Auto-detection of camera
- Interactive format selection
- Automatic testing
- Error checking

### From Pi 4 to Pi 5

**Key Differences**:
1. **No driver modification needed** on Pi 5 (runtime configuration)
2. **Different subdevice**: `/dev/v4l-subdev0` (Pi 4) → `/dev/v4l-subdev2` (Pi 5)
3. **Media pipeline required** on Pi 5 (run `./configure_media.sh`)
4. **Different I2C bus**: i2c-1 (Pi 4) → i2c-10 (Pi 5)
5. **Format requirement**: Must use 16-bit packed formats on Pi 5

**See**: [Installation Guide - Pi 5](../getting-started/installation-pi5.md)

---

## Breaking Changes

### None Yet

Current version (0.0.1) is the initial release. Future breaking changes will be documented here.

---

## Community

**Get Involved**:
- 🐛 [Report Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- 💡 [Request Features](https://github.com/Kodrea/rs300-v4l2-driver/issues/new)
- 📖 [Contribute Documentation](CONTRIBUTING.md)
- 💬 [Discussions](https://github.com/Kodrea/rs300-v4l2-driver/discussions)

**Follow Development**:
- 📺 [Video Tutorials](https://linktr.ee/kodrea)
- 📧 Project updates via GitHub Watch

---

**Last Updated**: 2025-10-21
