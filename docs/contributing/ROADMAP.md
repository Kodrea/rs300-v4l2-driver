# Project Roadmap

Future development plans and priorities for the RS300 thermal camera driver.

---

## Current Status

### ✅ Completed (Production Ready)

**Raspberry Pi 5 Support**:
- [x] Media controller pipeline integration
- [x] RP1-CFE camera system support
- [x] Automatic configuration script
- [x] 60fps thermal streaming (640×512)
- [x] All 11 V4L2 controls working
- [x] I2C communication on i2c-10
- [x] Boot automation options (systemd/udev)

**Raspberry Pi 4 Support**:
- [x] Legacy Unicam integration
- [x] 60fps streaming (640×512, 384×288)
- [x] DKMS installation system
- [x] Device tree overlay
- [x] V4L2 control interface

**Driver Features**:
- [x] Complete I2C protocol implementation
- [x] 14 camera commands
- [x] 11 V4L2 controls (brightness, colormap, FFC, etc.)
- [x] Multiple resolution support
- [x] Frame rate configuration
- [x] CRC-16-CCITT error checking

**Documentation**:
- [x] Comprehensive driver analysis
- [x] I2C protocol specification
- [x] Troubleshooting guide
- [x] Quick reference guide
- [x] Media pipeline documentation
- [x] ISP integration guide
- [x] Boot configuration guide

---

## Short Term (Next 3 Months)

### High Priority

**1. ISP Integration for Pi 5** 🔥
- **Goal**: Hardware-accelerated image processing via PiSP Backend
- **Status**: Research complete, implementation pending
- **Features**:
  - Temporal noise reduction (hardware)
  - Dual-stream output (full-res + thumbnail)
  - Real-time processing without CPU overhead
  - YUV color space processing
- **Documentation**: [RASPBERRY_PI_ISP_GUIDE.md](../../RASPBERRY_PI_ISP_GUIDE.md)
- **Timeline**: Q1 2025
- **Complexity**: Medium

**2. Test 384×288 and 256×192 Modules on Pi 5** 🧪
- **Goal**: Verify all resolutions work on Pi 5
- **Status**: Hardware pending
- **Tasks**:
  - Test 384×288 @ 60fps
  - Test 256×192 @ 50fps
  - Document any issues
  - Update compatibility matrix
- **Timeline**: When hardware available
- **Complexity**: Low

**3. 256×192 MIPI Troubleshooting (Pi 4)** 🐛
- **Goal**: Resolve MIPI video streaming issues with 256×192 module
- **Status**: Investigation ongoing
- **Current State**:
  - I2C communication: ✅ Working
  - Camera operation: ✅ Working (shutter audible, CVBS works)
  - MIPI video: ❌ No data
  - USB mode: ✅ Working (50Hz)
- **Approaches**:
  - Firmware update instructions from Purple River
  - Timing/clock investigation
  - Media bus format verification
  - MIPI CSI-2 signal analysis
- **Timeline**: Ongoing, lower priority (USB workaround available)
- **Complexity**: High

### Medium Priority

**4. Video Tutorial Series** 📺
- **Goal**: Visual walkthroughs for common tasks
- **Status**: In progress
- **Topics**:
  - [ ] Hardware unboxing and connections
  - [ ] Driver installation (Pi 5 & Pi 4)
  - [ ] First thermal capture
  - [ ] Camera controls demonstration
  - [ ] GStreamer pipeline examples
  - [ ] ISP integration demo
  - [ ] Troubleshooting common issues
- **Platform**: YouTube, linked via [linktr.ee/kodrea](https://linktr.ee/kodrea)
- **Timeline**: Q1-Q2 2025
- **Complexity**: Low-Medium

**5. Python SDK/Bindings** 🐍
- **Goal**: Python library for easy RS300 integration
- **Status**: Planned
- **Features**:
  - Simple capture interface
  - Control management (brightness, colormap, etc.)
  - Automatic FFC scheduling
  - NumPy array output
  - OpenCV integration
  - Example scripts
- **Timeline**: Q2 2025
- **Complexity**: Medium

---

## Medium Term (3-6 Months)

### Driver Improvements

**6. Runtime Resolution Switching**
- **Goal**: Change resolution without driver rebuild
- **Current Limitation**: Resolution set at compile time (Pi 4) or driver load
- **Proposed Solution**:
  - Add V4L2 selection API support
  - Runtime mode switching via v4l2-ctl
  - Automatic pipeline reconfiguration (Pi 5)
- **Benefits**:
  - User-friendly configuration
  - Dynamic resolution changes
  - No recompilation needed
- **Timeline**: Q2 2025
- **Complexity**: High

**7. Code Refactoring**
- **Goal**: Improve code quality and maintainability
- **Current Issues** (from DRIVER_ANALYSIS.md):
  - ~500 lines of duplicate command execution code
  - Hardcoded CRC in zoom command
  - Opportunities for helper functions
- **Proposed Changes**:
  - Create `rs300_send_command()` helper function
  - Consolidate duplicate code
  - Fix hardcoded CRC values
  - Improve error handling
- **Benefits**:
  - Reduced code size (~500 → ~150 lines)
  - Easier maintenance
  - Better testability
- **Timeline**: Q2 2025
- **Complexity**: Medium

**8. Advanced Control Features**
- **Goal**: Additional camera controls and features
- **Planned Controls**:
  - [ ] Temperature readout (if supported by module)
  - [ ] Automatic FFC scheduling (periodic calibration)
  - [ ] Custom scene mode presets
  - [ ] Image flip/mirror
  - [ ] Region of interest (ROI) selection
- **Timeline**: Q2-Q3 2025
- **Complexity**: Low-Medium per feature

### Integration & Examples

**9. GStreamer Element Plugin**
- **Goal**: Native GStreamer element for RS300
- **Benefits**:
  - Simplified pipeline construction
  - Auto-configuration
  - Property-based controls
  - Standard GStreamer integration
- **Example**:
  ```bash
  gst-launch-1.0 rs300src ! autovideosink
  ```
- **Timeline**: Q3 2025
- **Complexity**: Medium-High

**10. OpenCV Integration Examples**
- **Goal**: Complete OpenCV examples for thermal processing
- **Planned Examples**:
  - [ ] Basic capture and display
  - [ ] Temperature analysis
  - [ ] Object detection (hot spots)
  - [ ] Motion tracking
  - [ ] Image processing (filters, enhancement)
  - [ ] Video recording
  - [ ] Real-time analytics
- **Languages**: Python and C++
- **Timeline**: Q2-Q3 2025
- **Complexity**: Low-Medium

---

## Long Term (6-12 Months)

### Advanced Features

**11. Multi-Camera Support**
- **Goal**: Support multiple RS300 cameras simultaneously
- **Use Cases**:
  - Stereo thermal imaging
  - Wide-area monitoring (multiple angles)
  - Multi-resolution setups
- **Challenges**:
  - I2C address conflicts (currently all 0x3c)
  - CSI port limitations
  - Compute resource requirements
- **Solutions**:
  - I2C address modification (if hardware supports)
  - Multiple CSI ports (Pi 4 has 2, Pi 5 has 2)
  - USB fallback for additional cameras
- **Timeline**: Q3-Q4 2025
- **Complexity**: High

**12. Radiometric Support**
- **Goal**: Temperature measurement for "Mini" (radiometric) variant
- **Current Status**: Driver is for "Mini2" (imaging only)
- **Requirements**:
  - Different hardware (radiometric module)
  - Temperature calibration data
  - Additional I2C commands for temp readout
  - V4L2 control for temperature queries
- **Note**: Requires different module variant (not currently available for testing)
- **Timeline**: TBD (hardware dependent)
- **Complexity**: High

**13. AI/ML Integration Examples**
- **Goal**: Demonstrate thermal imaging + AI/ML
- **Possible Applications**:
  - Person detection (thermal signature)
  - Equipment anomaly detection
  - Fire/smoke detection
  - Energy loss detection (building inspection)
- **Technologies**:
  - TensorFlow Lite
  - PyTorch Mobile
  - ONNX Runtime
  - Custom models
- **Timeline**: Q4 2025
- **Complexity**: Medium-High

---

## Research & Exploration

### Under Investigation

**14. Pi Zero 2W Support**
- **Current Status**: ❌ Not compatible (power issues)
- **Challenge**: Camera draws too much current, causes brownouts
- **Attempted Solutions**:
  - External 5V power supply: Still brownouts
  - External power via module pins: Not possible with MIPI connected
- **Potential Solutions**:
  - Custom PCB revision with dedicated 5V power pins
  - Power management IC on adapter board
  - Lower power mode (if camera supports)
- **Timeline**: Hardware revision needed (TBD)
- **Priority**: Low (limited use case)

**15. Other SBCs / Platforms**
- **Potential Targets**:
  - Jetson Nano / Orin
  - Orange Pi 5
  - Rock Pi 4
  - Other MIPI CSI-2 capable boards
- **Requirements**:
  - MIPI CSI-2 support
  - I2C interface
  - V4L2 subsystem
  - Linux kernel compatibility
- **Timeline**: Community-driven (TBD)
- **Priority**: Low (Pi 5/4 are primary targets)

---

## Community Requests

### Feature Requests (GitHub Issues)

**Vote on features**: [GitHub Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)

**Top Requested** (to be updated):
1. ISP integration (Pi 5) - 🔥 In Progress
2. Python SDK - Planned Q2 2025
3. Runtime resolution switching - Planned Q2 2025
4. Video tutorials - In Progress
5. OpenCV examples - Planned Q2-Q3 2025

**Submit your request**: [New Feature Request](https://github.com/Kodrea/rs300-v4l2-driver/issues/new)

---

## Known Issues

### Active Issues

**High Priority**:
1. **256×192 MIPI video (Pi 4)**: Investigation ongoing
   - Workaround: Use USB mode (50Hz)

**Medium Priority**:
2. **Pi 4 low-voltage warnings (640×512)**: Rarely causes issues
   - Recommendation: Use adequate power supply (3A+)
3. **Runtime resolution switching**: Not implemented
   - Workaround: Edit `rs300.c`, rebuild

**Low Priority**:
4. **Hardcoded CRC in zoom command**: Works but not ideal
   - Impact: None (functional)

**See**: [TROUBLESHOOTING.md](../../TROUBLESHOOTING.md) for workarounds

---

## Contribution Opportunities

Want to contribute? Here are areas where help is needed:

### High Impact
- 🔥 **ISP integration testing** - Pi 5 owners with thermal experience
- 🧪 **Multi-resolution testing** - Test 384×288 and 256×192 on Pi 5
- 📺 **Video tutorials** - Screen recordings, voice-over
- 🐍 **Python SDK development** - Python developers

### Medium Impact
- 📝 **Documentation improvements** - Technical writers
- 🔧 **Code refactoring** - Kernel developers
- 🎨 **Example applications** - OpenCV, GStreamer experts
- 🧪 **Platform testing** - Other SBCs (Jetson, Orange Pi, etc.)

### Beginner Friendly
- 🐛 **Bug reports** - Detailed issue reports
- 📖 **Documentation fixes** - Typos, clarity improvements
- ✅ **Testing** - Verify fixes, test on different platforms
- 💬 **Community support** - Help others on GitHub Discussions

**See**: [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines

---

## Versioning Plan

### Release Schedule

**0.1.0** - Q1 2025
- ISP integration (Pi 5)
- Python SDK (basic)
- Video tutorials (first batch)
- Code refactoring

**0.2.0** - Q2 2025
- Runtime resolution switching
- Advanced controls
- GStreamer element
- OpenCV examples

**0.3.0** - Q3 2025
- Multi-camera support
- Additional platform support
- Performance optimizations

**1.0.0** - Q4 2025 (Stable)
- Feature complete
- Comprehensive testing
- Full documentation
- Production ready for all use cases

---

## Feedback

**Shape the roadmap!**

- 💬 [GitHub Discussions](https://github.com/Kodrea/rs300-v4l2-driver/discussions) - Propose ideas
- 🐛 [Issue Tracker](https://github.com/Kodrea/rs300-v4l2-driver/issues) - Request features
- 📧 Direct feedback: See project README for contact info

---

**Last Updated**: 2025-10-21

**Next Review**: 2025-11-21 (monthly updates)
