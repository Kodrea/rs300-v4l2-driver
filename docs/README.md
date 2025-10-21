# RS300 Driver Documentation

Complete documentation hub for the RS300 thermal camera driver project.

---

## Quick Navigation

| I want to... | Go to | Time |
|--------------|-------|------|
| **Install the driver** | [Getting Started](#getting-started) | 10 min |
| **Stream thermal video** | [Basic Usage](guides/basic-usage.md) | 5 min |
| **Adjust camera settings** | [Camera Controls](guides/camera-controls.md) | 10 min |
| **Buy hardware** | [Purchasing Guide](hardware/purchasing-guide.md) | 5 min |
| **Fix a problem** | [Troubleshooting](../TROUBLESHOOTING.md) | Varies |
| **Understand the driver** | [Driver Analysis](../DRIVER_ANALYSIS.md) | 30 min |
| **Contribute code** | [Contributing](contributing/CONTRIBUTING.md) | 15 min |

---

## Documentation Structure

```
docs/
├── getting-started/     → Installation & first capture
├── hardware/            → Specs, compatibility, purchasing
├── guides/              → Usage guides & controls
├── contributing/        → Contribution guidelines
└── [Root docs]          → Advanced topics (ISP, troubleshooting, etc.)
```

---

## Getting Started

**New to RS300? Start here.**

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Installation - Pi 5](getting-started/installation-pi5.md)** | Complete Pi 5 setup with RP1-CFE | 10 min | Pi 5 users |
| **[Installation - Pi 4](getting-started/installation-pi4.md)** | Complete Pi 4 setup with Unicam | 10 min | Pi 4 users |
| **[First Thermal Capture](getting-started/first-capture.md)** | Quick start guide to your first image | 5 min | All users |

**Choose your platform**:
- ✅ **Raspberry Pi 5**: Use [Pi 5 Installation Guide](getting-started/installation-pi5.md)
- ✅ **Raspberry Pi 4B**: Use [Pi 4 Installation Guide](getting-started/installation-pi4.md)
- ❌ **Pi Zero 2W**: Not supported (power limitations)

---

## Hardware Information

**Planning your purchase or setup? Check hardware docs.**

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Compatibility Matrix](hardware/compatibility.md)** | Platform testing results & known issues | 5 min | All users |
| **[Technical Specifications](hardware/specifications.md)** | Module specs, formats, resolutions | 10 min | Technical users |
| **[Purchasing Guide](hardware/purchasing-guide.md)** | Where to buy, what's included, pricing | 5 min | Buyers |

**Key specs**:
- **Resolutions**: 640×512, 384×288, 256×192
- **Frame Rates**: 25/30/50/60 fps (varies by resolution)
- **NETD**: 40mK thermal sensitivity
- **Interfaces**: MIPI CSI-2, USB 2.0, CVBS

---

## User Guides

**Using your installed camera.**

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Basic Usage](guides/basic-usage.md)** | Streaming, recording, network streaming | 15 min | All users |
| **[Camera Controls Reference](guides/camera-controls.md)** | Complete guide to all 11 V4L2 controls | 20 min | All users |

**Quick examples**:
```bash
# Live view
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0

# Trigger FFC calibration
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=ffc_trigger=0  # Pi 5
v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=ffc_trigger=0  # Pi 4

# Change colormap
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=colormap=3
```

---

## Advanced Topics

**Deep dives into specific features.**

### Raspberry Pi 5 Specific

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Media Pipeline Guide](../RS300_Media_Pipeline_Guide.md)** | Media controller concepts & configuration | 15 min | Pi 5 users |
| **[ISP Integration Guide](../RASPBERRY_PI_ISP_GUIDE.md)** | Hardware ISP processing with PiSP Backend | 30 min | Advanced Pi 5 |
| **[Boot Configuration](../BOOT_CONFIGURATION.md)** | Auto-start media pipeline at boot | 10 min | Pi 5 users |

**Pi 5 key differences**:
- Uses RP1-CFE (not Unicam)
- Requires media pipeline configuration
- Supports hardware ISP acceleration
- Only 16-bit packed formats (UYVY8_1X16, YUYV8_1X16)

### Technical Deep Dives

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Driver Analysis](../DRIVER_ANALYSIS.md)** | Complete architecture, code structure, internals | 45 min | Developers |
| **[I2C Protocol Specification](../I2C_PROTOCOL.md)** | 18-byte packet structure, CRC, commands | 20 min | Developers |
| **[Quick Reference](../DEV_QUICK_REFERENCE.md)** | Command cheat sheet & code locations | 5 min | Developers |

**For developers**:
- Want to modify the driver? Start with [Driver Analysis](../DRIVER_ANALYSIS.md)
- Adding I2C commands? See [I2C Protocol](../I2C_PROTOCOL.md)
- Quick command lookup? Use [Quick Reference](../DEV_QUICK_REFERENCE.md)

---

## Troubleshooting & Support

**Something not working?**

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Troubleshooting Guide](../TROUBLESHOOTING.md)** | Complete debug guide with decision trees | Varies | All users |

**Quick diagnostics**:
```bash
# Check driver loaded
lsmod | grep rs300

# Check I2C communication
i2cdetect -y 10  # Pi 5
i2cdetect -y 1   # Pi 4

# Check video device
v4l2-ctl --list-devices

# Pi 5: Check media pipeline
media-ctl -p | grep ENABLED
```

**Common issues**:
- [Media Pipeline Issues (Pi 5)](../TROUBLESHOOTING.md#media-pipeline-issues)
- [Format Mismatch Errors](../TROUBLESHOOTING.md#format-issues)
- [I2C Communication Failures](../TROUBLESHOOTING.md#i2c-communication-issues)
- [Low Frame Rate](../TROUBLESHOOTING.md#performance-issues)

**Still stuck?**
- 🐛 [Report an Issue](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- 📺 [Video Tutorials](https://linktr.ee/kodrea)

---

## Contributing

**Want to help improve the project?**

| Document | Description | Time | Audience |
|----------|-------------|------|----------|
| **[Contributing Guidelines](contributing/CONTRIBUTING.md)** | How to contribute code, docs, tests | 15 min | Contributors |
| **[Development Roadmap](contributing/ROADMAP.md)** | Planned features & timeline | 10 min | All |
| **[Changelog](contributing/CHANGELOG.md)** | Project history & updates | 5 min | All |

**Ways to contribute**:
- 💻 **Code**: Bug fixes, new features, refactoring
- 📖 **Documentation**: Improvements, examples, translations
- 🧪 **Testing**: Platform testing, bug reports
- 💡 **Ideas**: Feature requests, use cases
- 📺 **Tutorials**: Videos, blog posts, guides

**Good first issues**:
- Documentation improvements
- Testing on different modules (384×288, 256×192)
- Example applications (OpenCV, GStreamer)
- Bug reports with detailed reproduction steps

**See**: [ROADMAP.md](contributing/ROADMAP.md) for planned features

---

## Documentation by Use Case

### I want to install and use the camera

1. **[Choose your platform](hardware/compatibility.md)** - Verify compatibility
2. **[Buy hardware](hardware/purchasing-guide.md)** - Get the module
3. **[Install driver](getting-started/)** - Pi 5 or Pi 4 guide
4. **[First capture](getting-started/first-capture.md)** - Quick start
5. **[Learn controls](guides/camera-controls.md)** - Adjust settings

### I want to build an application

1. **[Basic Usage](guides/basic-usage.md)** - Streaming methods
2. **[Quick Reference](../DEV_QUICK_REFERENCE.md)** - Commands
3. **[Camera Controls](guides/camera-controls.md)** - All settings
4. **[OpenCV Examples](guides/basic-usage.md#processing-and-analysis)** - Python integration
5. **[GStreamer Pipelines](guides/basic-usage.md#streaming-methods)** - Video processing

### I want to understand the system

1. **[Driver Analysis](../DRIVER_ANALYSIS.md)** - Architecture overview
2. **[I2C Protocol](../I2C_PROTOCOL.md)** - Communication details
3. **[Media Pipeline Guide](../RS300_Media_Pipeline_Guide.md)** - Pi 5 pipeline
4. **[ISP Guide](../RASPBERRY_PI_ISP_GUIDE.md)** - Hardware acceleration

### I want to contribute

1. **[Roadmap](contributing/ROADMAP.md)** - See what's planned
2. **[Contributing Guide](contributing/CONTRIBUTING.md)** - Development setup
3. **[Driver Analysis](../DRIVER_ANALYSIS.md)** - Understand the code
4. **[Quick Reference](../DEV_QUICK_REFERENCE.md)** - Code locations

---

## Complete File Inventory

**All documentation files** (organized by location):

### docs/getting-started/
- `installation-pi5.md` - Raspberry Pi 5 installation
- `installation-pi4.md` - Raspberry Pi 4 installation
- `first-capture.md` - Quick start guide

### docs/hardware/
- `compatibility.md` - Platform compatibility matrix
- `specifications.md` - Technical specifications
- `purchasing-guide.md` - Where to buy & what's included

### docs/guides/
- `basic-usage.md` - Streaming, recording, examples
- `camera-controls.md` - Complete V4L2 control reference

### docs/contributing/
- `CONTRIBUTING.md` - Contribution guidelines
- `ROADMAP.md` - Development roadmap
- `CHANGELOG.md` - Project history

### Root Documentation (../)
- `README.md` - Project overview & quick start
- `TROUBLESHOOTING.md` - Complete troubleshooting guide
- `DRIVER_ANALYSIS.md` - Driver architecture & analysis
- `I2C_PROTOCOL.md` - I2C command specification
- `DEV_QUICK_REFERENCE.md` - Developer quick reference
- `RASPBERRY_PI_ISP_GUIDE.md` - ISP integration guide
- `RS300_Media_Pipeline_Guide.md` - Media controller guide
- `BOOT_CONFIGURATION.md` - Auto-start configuration
- `FILE_INVENTORY.md` - Complete file listing
- `CLAUDE.md` - AI assistant navigation guide

**See**: [FILE_INVENTORY.md](../FILE_INVENTORY.md) for complete project file listing

---

## Documentation Reading Levels

### Skim Level (1-5 minutes)
**Good for**: Quick lookups, finding commands

- Read section headers only
- Scan tables and code examples
- Use Ctrl+F to find specific terms

**Example**: "Where is the brightness control command?"
→ Skim [Camera Controls](guides/camera-controls.md) → Find brightness section

### Standard Level (10-20 minutes)
**Good for**: Understanding workflows, troubleshooting

- Read relevant sections fully
- Understand concepts, skip implementation details
- Review decision trees and tables

**Example**: "How do I debug I2C timeouts?"
→ Read [Troubleshooting](../TROUBLESHOOTING.md) → I2C section → Follow procedure

### Deep Level (30-60 minutes)
**Good for**: Code modifications, system integration

- Read complete documents
- Study code examples and algorithms
- Cross-reference between documents

**Example**: "I need to add a new V4L2 control"
→ Read [Driver Analysis](../DRIVER_ANALYSIS.md) Section 4 → [I2C Protocol](../I2C_PROTOCOL.md) → Code

---

## Learning Paths

### Path 1: Quick Start (30 minutes)
For users who want to get thermal video streaming quickly.

1. [Installation Guide](getting-started/) - 10 min
2. [First Capture](getting-started/first-capture.md) - 5 min
3. [Basic Usage](guides/basic-usage.md) (skim) - 5 min
4. [Camera Controls](guides/camera-controls.md) (essential controls only) - 10 min

**Outcome**: Streaming thermal video with basic control adjustments

### Path 2: Application Developer (2 hours)
For developers building applications with RS300.

1. [Installation Guide](getting-started/) - 10 min
2. [First Capture](getting-started/first-capture.md) - 5 min
3. [Basic Usage](guides/basic-usage.md) (complete) - 30 min
4. [Camera Controls](guides/camera-controls.md) (complete) - 30 min
5. [Quick Reference](../DEV_QUICK_REFERENCE.md) - 15 min
6. [Troubleshooting](../TROUBLESHOOTING.md) (skim) - 15 min
7. Examples & experimentation - 15 min

**Outcome**: Can build applications using RS300 thermal camera

### Path 3: Driver Developer (4-6 hours)
For contributors modifying the driver code.

1. [Installation Guide](getting-started/) - 10 min
2. [Driver Analysis](../DRIVER_ANALYSIS.md) (complete) - 60 min
3. [I2C Protocol](../I2C_PROTOCOL.md) (complete) - 30 min
4. [Quick Reference](../DEV_QUICK_REFERENCE.md) - 10 min
5. [Contributing Guide](contributing/CONTRIBUTING.md) - 20 min
6. [Pi 5: Media Pipeline Guide](../RS300_Media_Pipeline_Guide.md) - 20 min
7. [Pi 5: ISP Guide](../RASPBERRY_PI_ISP_GUIDE.md) - 30 min
8. Code review (rs300.c) - 60-90 min
9. [Troubleshooting](../TROUBLESHOOTING.md) - 30 min

**Outcome**: Understand driver architecture, ready to contribute code

---

## Video Tutorials

**Visual walkthroughs** available at [linktr.ee/kodrea](https://linktr.ee/kodrea)

**Current topics**:
- Hardware unboxing & connections
- Driver installation walkthrough
- First thermal capture
- Camera controls demonstration

**Coming soon**:
- GStreamer pipeline examples
- ISP integration demo
- Troubleshooting common issues

---

## External Resources

### Official Documentation
- [V4L2 API](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2.html) - Video4Linux2 reference
- [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/) - Pi hardware & software
- [RP1 Peripherals](https://datasheets.raspberrypi.com/rp1/rp1-peripherals.pdf) - Pi 5 RP1 chip details

### Community Resources
- [GitHub Repository](https://github.com/Kodrea/rs300-v4l2-driver) - Source code
- [Issue Tracker](https://github.com/Kodrea/rs300-v4l2-driver/issues) - Bug reports & features
- [Video Tutorials](https://linktr.ee/kodrea) - Visual guides

### Hardware Vendors
- [Purple River Technology](https://www.thermal-image.com/) - Module purchase (use code HXZUK8WG)
- [Purple River Alibaba](https://purpleriver.en.alibaba.com/) - Alternative store

---

## Documentation Contributions

**Help improve these docs!**

- 🐛 **Found an error?** [Report it](https://github.com/Kodrea/rs300-v4l2-driver/issues/new)
- 💡 **Have a suggestion?** [Open a discussion](https://github.com/Kodrea/rs300-v4l2-driver/discussions)
- ✍️ **Want to contribute?** See [CONTRIBUTING.md](contributing/CONTRIBUTING.md)

**Documentation standards**:
- Clear, concise language
- Step-by-step instructions
- Code examples that work
- Time estimates for tasks
- Cross-references to related docs

---

## Support

**Need help?**

1. **Check documentation** (you're in the right place!)
2. **Search issues**: [GitHub Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
3. **Troubleshooting guide**: [TROUBLESHOOTING.md](../TROUBLESHOOTING.md)
4. **Video tutorials**: [linktr.ee/kodrea](https://linktr.ee/kodrea)
5. **Ask a question**: [Open an issue](https://github.com/Kodrea/rs300-v4l2-driver/issues/new)

---

**Last Updated**: 2025-10-21

**Documentation Version**: 1.0 (aligned with driver v0.0.1)

[← Back to Main README](../README.md)
