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

### 🔥 Highest Priority: Driver Modernization

**NEW: Driver Modernization Initiative** 🚀
- **Goal**: Transform RS300 into a modern, user-friendly V4L2 camera driver
- **Status**: Planning complete, ready to implement
- **Documentation**: [MODERNIZATION_PLAN.md](../../MODERNIZATION_PLAN.md) ⭐ **FULL TECHNICAL DETAILS**
- **Timeline**: 12-14 weeks (3-3.5 months)

**Priority Order:**

**1. Runtime Resolution Switching** 🎯 **(Sprint 2: Week 3-4)**
- **Goal**: Change resolution without driver reload, with auto-detection fallback
- **Status**: Infrastructure exists, needs activation
- **Current State**:
  - ✅ 3 resolutions supported (640×512, 384×288, 256×192)
  - ✅ V4L2 infrastructure present (`enum_frame_sizes`, `set_fmt`)
  - ❌ Locked at load time via module parameter
- **Implementation**:
  - Modify `rs300_set_pad_fmt()` to update active mode
  - Research resolution query I2C command (need Purple River input)
  - Auto-detect resolution from firmware (if supported)
  - Fall back to module parameter if auto-detect fails
- **Benefits**:
  - Switch via `v4l2-ctl --set-fmt-video`
  - No more `rmmod` + `modprobe` cycle
  - Better user experience
- **Timeline**: 2 weeks
- **Complexity**: Medium
- **Risk**: Low-Medium (requires all 3 hardware modules for testing)

**2. Modern Framework (Eliminate configure_media.sh)** 🏗️ **(Sprint 4: Week 7-10)**
- **Goal**: Auto-configure media controller pipeline, "plug and play" operation
- **Status**: Multiple approaches evaluated
- **Current State**:
  - ❌ Requires manual `./configure_media.sh` after every boot
  - ❌ Media controller links not auto-configured
  - ✅ Pipeline configuration logic exists
- **Implementation** (Hybrid Approach):
  - **Phase 1 (Quick)**: udev-based auto-configuration **(Sprint 1: Week 1)**
    - Trigger script on device detection
    - Works immediately with no driver changes
    - Complexity: Low (1-2 days)
  - **Phase 2 (Proper)**: v4l2_async framework **(Sprint 4: Week 7-10)**
    - Implement `v4l2_async_notifier` in driver
    - Auto-setup links when CSI-2 receiver ready
    - Upstream-acceptable solution
    - Complexity: High (2-4 weeks)
- **Benefits**:
  - Camera "just works" after driver load
  - No manual configuration
  - Persistent across reboots
- **Timeline**: 1 week (udev), 3-4 weeks (async framework)
- **Complexity**: Low (udev), High (async)
- **Risk**: Medium (async requires deep V4L2 knowledge)

**3. libcamera Support** 📷 **(Sprint 5: Week 11-14, if needed)**
- **Goal**: Work with libcamera stack (Raspberry Pi's modern camera API)
- **Status**: Requirements analyzed, testing approach defined
- **Current State**:
  - ✅ Driver V4L2 interface compliant
  - ✅ Controls properly exposed
  - ❓ Unknown if SimplePipelineHandler works
- **Implementation** (Phased):
  - **Phase 1**: Test with SimplePipelineHandler **(Sprint 1: Week 1)**
    - Try `libcamera-hello --list-cameras`
    - May work without changes!
    - Complexity: Low (1 day testing)
  - **Phase 2**: Custom Pipeline Handler (if Phase 1 fails) **(Sprint 5: Week 11-14)**
    - Create `src/libcamera/pipeline/rs300/rs300.cpp`
    - Implement basic capture functionality
    - Submit upstream to libcamera
    - Complexity: Medium-High (2-3 weeks)
- **Benefits**:
  - Use modern Raspberry Pi camera tools
  - ISP integration (if using RaspberryPi pipeline)
  - Better application compatibility
- **Timeline**: 1 day (testing), 2-3 weeks (custom handler if needed)
- **Complexity**: Low (testing), Medium-High (custom handler)
- **Risk**: Medium (may require upstream libcamera changes)

**4. Additional I2C Commands** 📡 **(Sprint 1-3, then Sprint 6)**
- **Goal**: Integrate high-priority I2C protocol commands in batches
- **Status**: Detailed implementation plan created
- **Documentation**: [I2C_COMMANDS_TO_IMPLEMENT.md](../../I2C_COMMANDS_TO_IMPLEMENT.md) ⭐ **FULL DETAILS**
- **Current State**:
  - ✅ 12 commands already implemented (all critical SET commands work)
  - 📋 18 commands planned in 6 batches
  - ❌ ~50 commands excluded (calibration, non-MIPI outputs, dangerous commands)

**Batch 1: Autoshutter Commands** 🔥 **(Sprint 1: Days 1-2)**
- `rs300_set_autoshutter()` - Enable/disable automatic FFC
- `rs300_get_autoshutter()` - Read autoshutter state
- `rs300_set_autoshutter_params()` - Configure temp threshold and intervals
- **Priority**: HIGHEST - User requested as #1 priority
- **Timeline**: 6-8 hours
- **Complexity**: Low

**Batch 2: Module Sleep Commands** 🔥 **(Sprint 1: Days 3-4)**
- `rs300_set_sleep()` - Put module to sleep or wake up
- `rs300_get_sleep()` - Read sleep state
- **Priority**: HIGHEST - User requested as #2 priority
- **Timeline**: 3-4 hours
- **Complexity**: Low

**Batch 3: Device Information Commands** 🎯 **(Sprint 2: Days 1-2)**
- `rs300_get_device_name()` - **KEY for resolution auto-detection!**
- `rs300_get_firmware_version()` - Diagnostics
- `rs300_get_module_temperature()` - Detector temp readout
- **Priority**: HIGH - Enables Priority 1 (resolution auto-detection)
- **Timeline**: 6-8 hours
- **Complexity**: Low

**Batch 4: Image Enhancement** 🎨 **(Sprint 3: Days 1-2, Optional)**
- `rs300_set_gamma()` - Gamma curve adjustment
- `rs300_get_gamma()` - Read gamma value
- **Priority**: MEDIUM - Nice to have
- **Timeline**: 3-4 hours
- **Complexity**: Low

**Batches 5-6: Advanced Features** ⏳ **(Sprint 6: Future)**
- Edge position, antiburn protection, hotspot coordinates
- Boot logo, save/restore parameters
- **Priority**: LOW - Advanced users only
- **Timeline**: 10-14 hours total

- **Benefits**:
  - Autoshutter: Automatic FFC without user intervention
  - Sleep: Power saving for battery-powered applications
  - Device info: Better diagnostics and auto-detection
  - Complete thermal camera control
- **Total Timeline**:
  - Sprint 1: 5 commands (9-12 hours)
  - Sprint 2: 3 commands (6-8 hours)
  - Sprint 3: 2 commands (3-4 hours, optional)
  - Sprint 6: 6 commands (10-14 hours, future)
- **Complexity**: Low (follow existing patterns)
- **Risk**: Low

**5. Pi 5 ISP Integration** 🎨 **(Sprint 3: Week 5-6)**
- **Goal**: Hardware-accelerated image processing via PiSP Backend
- **Status**: Documented, ready to implement
- **Current State**:
  - ✅ Documentation complete ([RASPBERRY_PI_ISP_GUIDE.md](../../RASPBERRY_PI_ISP_GUIDE.md))
  - ❌ No ISP pipeline implemented
- **Implementation** (Phased):
  - **Phase 1**: V4L2 M2M Pipeline **(Sprint 3: Week 5-6)**
    - Use pispbe as memory-to-memory device
    - RS300 → Capture → pispbe → Processed output
    - Complexity: Medium (1 week)
  - **Phase 2**: libcamera Integration (if Priority 3 done)
    - Automatic ISP routing via libcamera
    - Complexity: Low (already done by libcamera)
  - **Phase 3**: Kernel-Level Integration (long-term)
    - Direct media controller pipeline
    - Complexity: High (3-4 weeks)
- **Features**:
  - Temporal noise reduction (hardware TNR)
  - Spatial noise reduction (hardware SNR)
  - Dual-stream output (full-res + thumbnail)
  - Zero-copy processing
- **Benefits**:
  - Better image quality
  - Hardware acceleration (no CPU overhead)
  - Dual-stream for preview + recording
- **Timeline**: 1 week (V4L2 M2M)
- **Complexity**: Medium
- **Risk**: Low-Medium

### Medium Priority

**6. Video Tutorial Series** 📺
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

**7. Python SDK/Bindings** 🐍
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

**8. Test 384×288 and 256×192 Modules on Pi 5** 🧪
- **Goal**: Verify all resolutions work on Pi 5
- **Status**: Hardware pending (blocked by Priority 1)
- **Tasks**:
  - Test 384×288 @ 60fps
  - Test 256×192 @ 50fps
  - Document any issues
  - Update compatibility matrix
- **Note**: Will be done as part of Priority 1 testing
- **Timeline**: When hardware available
- **Complexity**: Low

---

## Medium Term (3-6 Months)

### Driver Improvements

**9. Code Refactoring** ⚠️ **CAUTION - See lessons-learned/002**
- **Goal**: Improve code quality and maintainability
- **Status**: **POSTPONED** - Previous consolidation attempt caused kernel crashes
- **Current Issues** (from DRIVER_ANALYSIS.md):
  - ~500 lines of duplicate command execution code
  - Hardcoded CRC in zoom command
  - Opportunities for helper functions
- **Lessons Learned**:
  - ❌ Function signature changes can cause mysterious kernel crashes
  - ❌ Code consolidation in kernel drivers is risky
  - ✅ "Working code > Pretty code" for kernel drivers
  - ✅ Accept duplication for stability
- **Proposed Approach** (if revisited):
  - **DO NOT** modify existing function signatures
  - Use parameter structs if adding parameters
  - Create NEW functions rather than modifying existing ones
  - Test extensively on all hardware
  - See `.claude/lessons-learned/002-consolidation-kernel-crash.md`
- **Timeline**: TBD (low priority, stability concerns)
- **Complexity**: High
- **Risk**: High (kernel crashes possible)

**10. Advanced Control Features**
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

**11. GStreamer Element Plugin**
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

**12. OpenCV Integration Examples**
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

**13. Multi-Camera Support**
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

**14. Radiometric Support**
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

**15. AI/ML Integration Examples**
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

**16. Pi Zero 2W Support**
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

**17. Other SBCs / Platforms**
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

**Top Requested** (Updated 2025-10-24):
1. **Runtime resolution switching** - 🔥 **HIGH PRIORITY** - Planned Sprint 2 (Week 3-4)
2. **Auto-configure pipeline (no configure_media.sh)** - 🔥 **HIGH PRIORITY** - udev Sprint 1, async Sprint 4
3. **libcamera support** - 🔥 **HIGH PRIORITY** - Testing Sprint 1, custom handler Sprint 5 (if needed)
4. **ISP integration (Pi 5)** - 🔥 **HIGH PRIORITY** - Planned Sprint 3 (Week 5-6)
5. **Additional I2C commands** - 🔥 **HIGH PRIORITY** - Planned Sprint 1 (Week 1-2)
6. Python SDK - Medium Priority - Planned Q2 2025
7. Video tutorials - Medium Priority - In Progress
8. OpenCV examples - Medium Priority - Planned Q2-Q3 2025

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
3. **Runtime resolution switching**: Not implemented (⚠️ **IN PROGRESS** - Sprint 2)
   - Workaround: Use module parameter `mode=0/1/2` until implemented
4. **Manual media pipeline configuration**: Required after every boot (⚠️ **IN PROGRESS** - Sprint 1 & 4)
   - Workaround: Run `./configure_media.sh` or use boot automation options

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

**v0.9.0** - Q1 2025 (Modernization Release) 🚀
- ✅ Runtime resolution switching
- ✅ Auto-configure pipeline (udev + async framework)
- ✅ libcamera support (simple or custom handler)
- ✅ Additional I2C commands (Get Device Name, Set YUV Format, etc.)
- ✅ ISP integration (V4L2 M2M pipeline)
- ✅ Complete modernization plan
- **Status**: Modern, user-friendly driver

**v1.0.0** - Q2 2025 (Stable Release)
- Python SDK (basic)
- Video tutorials (first batch)
- Enhanced documentation
- Comprehensive testing on all hardware
- **Status**: Production-ready for all use cases

**v1.1.0** - Q3 2025 (Enhanced Features)
- Advanced controls (temp readout, auto-FFC, ROI)
- GStreamer element plugin
- OpenCV examples
- Performance optimizations

**v1.2.0** - Q4 2025 (Advanced Features)
- Multi-camera support
- Additional platform support
- AI/ML integration examples

**v2.0.0** - 2026 (Future)
- Radiometric support (temperature measurement)
- Full ISP kernel integration
- Upstream kernel submission

---

## Feedback

**Shape the roadmap!**

- 💬 [GitHub Discussions](https://github.com/Kodrea/rs300-v4l2-driver/discussions) - Propose ideas
- 🐛 [Issue Tracker](https://github.com/Kodrea/rs300-v4l2-driver/issues) - Request features
- 📧 Direct feedback: See project README for contact info

---

**Last Updated**: 2025-10-24

**Next Review**: 2025-11-24 (monthly updates)

**Major Update**: Complete reprioritization based on driver modernization requirements. See [MODERNIZATION_PLAN.md](../../MODERNIZATION_PLAN.md) for full technical details.
