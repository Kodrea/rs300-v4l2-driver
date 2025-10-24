# RS300 Driver Modernization Plan

**Goal**: Transform RS300 driver into a modern, user-friendly V4L2 camera driver

**Last Updated**: 2025-10-24

---

## Overview

This document outlines the technical requirements and implementation plan for modernizing the RS300 thermal camera driver based on the following priorities:

1. **Runtime Resolution Switching** (with auto-detection fallback)
2. **Modern Framework** (eliminate manual media controller configuration)
3. **libcamera Support**
4. **Additional I2C Commands**
5. **Pi 5 ISP Integration**

---

## Priority 1: Runtime Resolution Switching

### Current State

**What Works:**
- ✅ Driver has `supported_modes[]` array with 3 resolutions (640×512, 256×192, 384×288)
- ✅ Infrastructure exists: `enum_frame_sizes`, `enum_mbus_code`, `set_fmt`, `get_fmt`
- ✅ V4L2 subdev pad operations properly implemented

**What Doesn't Work:**
- ❌ Resolution locked at driver load time via `mode` module parameter
- ❌ `rs300->mode` set once in probe, never changed
- ❌ Changing resolution requires `rmmod` + `modprobe mode=N`
- ❌ No resolution auto-detection from camera firmware

### Technical Analysis

**File**: `rs300.c`

**Key Code Locations:**
- `supported_modes[]` array: Line ~474
- `rs300->mode` initialization: Line ~675, ~2675
- `rs300_set_pad_fmt()`: Line ~1627 (has mode matching but doesn't apply it)
- Module parameter: Line ~103

**Current Flow:**
```
1. modprobe rs300 mode=0  → Sets mode at load time
2. rs300_probe() → rs300->mode = &supported_modes[mode]
3. All operations use rs300->mode (never changes)
4. set_fmt finds matching mode but doesn't update rs300->mode
```

**Proposed Solution:**

**Phase 1: Basic Runtime Switching**
1. Keep module parameter as *default* mode (backward compatible)
2. Modify `rs300_set_pad_fmt()` to actually update `rs300->mode`
3. Add I2C command to switch camera resolution (if supported)
4. Test resolution changes without driver reload

**Code Changes Required:**
```c
// In rs300_set_pad_fmt() around line 1664
mode = v4l2_find_nearest_size(supported_modes, ...);

// ADD: Actually update the current mode
mutex_lock(&rs300->mutex);
rs300->mode = mode;  // <-- Add this
mutex_unlock(&rs300->mutex);

// MAY NEED: Send I2C command to camera
// ret = rs300_set_resolution(rs300, mode->width, mode->height);
```

**Phase 2: Resolution Auto-Detection**

**Approach A: Query Camera Firmware** (preferred if camera supports)
- Add I2C command to read current resolution from camera
- Implement in probe: `rs300_detect_resolution()`
- Fall back to module parameter if query fails

**Approach B: Trial-and-Error Detection**
- Try streaming at each resolution
- Check if camera responds with valid data
- Set mode based on what works
- More complex, less reliable

**Implementation Plan:**
1. Research if RS300 firmware supports resolution query command (check with Purple River)
2. If yes: Implement query command (Class 0x10, Module 0x10, SubCmd 0x8X?)
3. If no: Use module parameter with runtime switching only

**Testing Requirements:**
- Test on all 3 hardware variants (640×512, 384×288, 256×192)
- Verify media pipeline reconfiguration (Pi 5)
- Ensure no memory leaks on mode switches
- Test mode switching while streaming (should fail gracefully)
- Verify V4L2 compliance: `v4l2-compliance -d /dev/v4l-subdev2`

**Estimated Complexity**: Medium (1-2 weeks)
**Risk Level**: Low-Medium (requires hardware testing)

---

## Priority 2: Modern Framework (Auto Pipeline Configuration)

### Current State

**What Works:**
- ✅ Driver is proper V4L2 subdev
- ✅ Media controller integration functional
- ✅ `configure_media.sh` script configures pipeline correctly

**What Doesn't Work:**
- ❌ User must run `./configure_media.sh` after every boot
- ❌ Media controller links not auto-configured
- ❌ Not "plug and play" like modern cameras

### Technical Analysis

**The Problem:**

Modern camera drivers auto-configure their media controller pipelines. The RS300 driver requires manual link setup because:

1. **Upstream rp1-cfe driver** doesn't know about RS300
2. **RS300 driver** doesn't set up links in probe (timing issues)
3. **V4L2 async framework** not properly implemented

**What configure_media.sh Does:**
```bash
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 ...]"
# ... more format propagation
```

**Solution Approaches:**

### Approach A: V4L2 Async Subdev Framework (Best)

**Concept**: Use `v4l2_async_notifier` to be notified when CSI-2 receiver is ready, then auto-configure links.

**Implementation:**
```c
// In rs300_probe()
static int rs300_probe(struct i2c_client *client) {
    // ... existing code ...

    // Register async subdev
    ret = v4l2_async_register_subdev(&rs300->sd);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to register async subdev\n");
        goto error;
    }

    // Set up notifier for CSI-2 receiver
    v4l2_async_nf_init(&rs300->notifier);
    rs300->notifier.ops = &rs300_notify_ops;

    // Add async subdev to wait for (CSI-2 bridge)
    struct v4l2_async_subdev *asd;
    asd = v4l2_async_nf_add_fwnode_remote(...);

    ret = v4l2_async_nf_register(&rs300->notifier);

    return 0;
}

// Notifier callback when CSI-2 ready
static int rs300_notify_bound(struct v4l2_async_notifier *notifier,
                               struct v4l2_subdev *sd,
                               struct v4l2_async_subdev *asd) {
    struct rs300 *rs300 = container_of(notifier, struct rs300, notifier);

    // CSI-2 receiver is ready, set up links
    ret = media_create_pad_link(&rs300->sd.entity, 0,
                                 &remote_entity, remote_pad,
                                 MEDIA_LNK_FL_ENABLED);

    // Set formats
    // ...

    return 0;
}
```

**Advantages:**
- ✅ Proper kernel framework usage
- ✅ Upstream-acceptable solution
- ✅ Works with hotplug
- ✅ No manual configuration needed

**Disadvantages:**
- ⚠️ Complex to implement
- ⚠️ May require device tree changes
- ⚠️ Need to understand fwnode and device graph

**Estimated Complexity**: High (2-4 weeks)
**Risk Level**: Medium (requires deep V4L2 knowledge)

### Approach B: Auto-Configure in Module Init (Simpler)

**Concept**: Create a separate userspace helper that runs automatically via udev.

**Implementation:**
```bash
# /etc/udev/rules.d/99-rs300-autoconfig.rules
SUBSYSTEM=="video4linux", KERNEL=="v4l-subdev*", \
    ATTRS{name}=="rs300*", \
    RUN+="/usr/local/bin/rs300-autoconfig.sh"
```

**Advantages:**
- ✅ Simple to implement
- ✅ No driver changes needed (or minimal)
- ✅ Works immediately
- ✅ Easy to debug

**Disadvantages:**
- ⚠️ Not truly "in-kernel" solution
- ⚠️ Depends on udev
- ⚠️ Still requires external script

**Estimated Complexity**: Low (1-2 days)
**Risk Level**: Low

### Approach C: Hybrid (Recommended)

**Concept**: Implement Approach B immediately, work on Approach A long-term.

**Rationale**:
- Get users a working auto-configuration quickly
- Build proper async framework in parallel
- Transition seamlessly when ready

**Recommended Path Forward:**
1. **Week 1**: Implement udev-based auto-configuration (Approach B)
2. **Week 2-4**: Research and implement v4l2_async framework (Approach A)
3. **Week 5**: Testing and validation
4. **Week 6**: Upstream submission preparation

---

## Priority 3: libcamera Support

### Current State

**What Works:**
- ✅ Driver exposes V4L2 subdev properly
- ✅ All controls accessible via V4L2
- ✅ Media controller topology correct

**What Doesn't Work:**
- ❌ libcamera doesn't recognize RS300 camera
- ❌ No pipeline handler for RS300
- ❌ No tuning files

### Technical Analysis

**libcamera Architecture:**
```
┌──────────────────────────────────────┐
│         Application (libcamera API)   │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      libcamera Core                   │
│  - Camera Manager                     │
│  - Pipeline Handler                   │
│  - IPA (Image Processing Algorithms)  │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      V4L2 Kernel Driver (rs300.ko)    │
└──────────────────────────────────────┘
```

**What's Needed:**

### Option 1: Simple Pipeline Handler (Quick Path)

libcamera has a "SimplePipelineHandler" that works with generic cameras.

**Requirements:**
- Driver must expose proper V4L2 interface ✅ (done)
- Controls must be standard V4L2 ✅ (done)
- Media controller topology must be correct ✅ (done)

**Testing:**
```bash
# Check if libcamera sees the camera
libcamera-hello --list-cameras

# If it appears, test capture
libcamera-still -o test.jpg --camera 0
```

**If This Works**: We're done! Just document it.
**If This Doesn't Work**: Need custom pipeline handler.

### Option 2: Custom Pipeline Handler (Full Support)

**File**: `src/libcamera/pipeline/rs300/rs300.cpp` (new file in libcamera)

**Structure:**
```cpp
class PipelineHandlerRS300 : public PipelineHandler {
public:
    PipelineHandlerRS300(CameraManager *manager);

    CameraConfiguration *generateConfiguration(Camera *camera, ...);
    int configure(Camera *camera, CameraConfiguration *config);
    int start(Camera *camera, const ControlList *controls);
    void stop(Camera *camera);

    bool match(DeviceEnumerator *enumerator) override;

private:
    // RS300-specific methods
    int configureRS300(Camera *camera);
};

// Registration
REGISTER_PIPELINE_HANDLER(PipelineHandlerRS300)
```

**Complexity**: High - requires C++ expertise and libcamera knowledge

### Option 3: Use Existing RaspberryPi Pipeline (Pi 5 Only)

**Concept**: Modify existing Pi camera pipeline handler to recognize RS300.

**File**: `src/libcamera/pipeline/rpi/rpi.cpp`

**Changes:**
- Add RS300 to camera detection
- Handle thermal camera (no Bayer processing)
- Use ISP for noise reduction only

**Estimated Complexity**: Medium-High (2-3 weeks)

### Recommended Approach:

**Phase 1** (Immediate): Test with SimplePipelineHandler
- Try `libcamera-hello --list-cameras`
- Document any issues
- File bug reports if needed

**Phase 2** (If Simple doesn't work): Custom Pipeline Handler
- Implement minimal RS300 pipeline handler
- Focus on basic capture functionality
- Submit to libcamera upstream

**Phase 3** (Long-term): Full Integration
- Add ISP support to pipeline handler
- Create tuning files (if needed for thermal)
- Optimize performance

**Testing Requirements:**
- Test all libcamera tools: `libcamera-hello`, `libcamera-still`, `libcamera-vid`
- Verify controls accessible via libcamera API
- Check multi-camera support
- Performance benchmarks

**Estimated Complexity**: Medium-High (2-4 weeks, depends on simple pipeline success)
**Risk Level**: Medium (may require upstream libcamera changes)

---

## Priority 4: Additional I2C Commands

### Current State

**Implemented Commands** (12/16):
1. ✅ Set Brightness
2. ✅ Set Colormap
3. ✅ Trigger FFC (Flat Field Calibration)
4. ✅ Set FPS
5. ✅ Set Zoom
6. ✅ Set Contrast
7. ✅ Set DDE (Digital Detail Enhancement)
8. ✅ Set Scene Mode
9. ✅ Set Spatial NR
10. ✅ Set Temporal NR
11. ✅ Start Streaming
12. ✅ Stop Streaming

**Missing Commands** (4/16):
1. ❌ Get Device Name (0x01, 0x01, 0x81)
2. ❌ Get Colormap (0x10, 0x03, 0x85)
3. ❌ Get Brightness (0x10, 0x04, 0x87)
4. ❌ Set YUV Format (0x10, 0x03, 0x4D)

### Implementation Priority

**High Priority:**
1. **Set YUV Format** - Useful for format selection (UYVY, YUYV, etc.)
2. **Get Device Name** - Useful for auto-detection and identification

**Medium Priority:**
3. **Get Brightness** - Nice for control readback verification
4. **Get Colormap** - Nice for control readback verification

### Implementation Plan

#### 1. Set YUV Format Command

**Purpose**: Allow runtime selection of YUV byte ordering.

**I2C Command:**
- Class: 0x10
- Module: 0x03
- SubCmd: 0x4D
- Parameter: P1=format (0=UYVY, 1=VYUY, 2=YUYV, 3=YVYU)

**V4L2 Integration:**
Add custom control:
```c
static const struct v4l2_ctrl_config yuv_format_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 8,
    .name = "YUV Format",
    .type = V4L2_CTRL_TYPE_MENU,
    .min = 0,
    .max = 3,
    .def = 0,  // UYVY default
    .menu_skip_mask = 0,
};
```

**Implementation:**
```c
static int rs300_set_yuv_format(struct rs300 *rs300, int format)
{
    u8 params[12] = {0};
    params[0] = format & 0xFF;

    return rs300_send_command(rs300, 0x10, 0x03, 0x4D, params, 1, 500);
}
```

**Estimated Time**: 2-3 hours

#### 2. Get Device Name Command

**Purpose**: Read camera model/firmware version for identification.

**I2C Command:**
- Class: 0x01
- Module: 0x01
- SubCmd: 0x81
- Parameters: P1=0x01, P9=0x20
- Response: ASCII string in buffer

**Implementation:**
```c
static int rs300_get_device_name(struct rs300 *rs300, char *name, size_t len)
{
    u8 cmd_buffer[18] = {0};
    u8 result_buffer[256] = {0};
    int ret;

    // Build command
    cmd_buffer[0] = 0x01;  // Class
    cmd_buffer[1] = 0x01;  // Module
    cmd_buffer[2] = 0x81;  // SubCmd
    cmd_buffer[4] = 0x01;  // P1
    cmd_buffer[12] = 0x20; // P9 (length)

    // Calculate CRC
    calculate_crc(cmd_buffer);

    // Send command and read result
    ret = rs300_send_command_with_result(rs300, cmd_buffer, result_buffer);
    if (ret == 0) {
        snprintf(name, len, "%s", &result_buffer[4]);
    }

    return ret;
}
```

**Use in Probe:**
```c
char device_name[32];
ret = rs300_get_device_name(rs300, device_name, sizeof(device_name));
if (ret == 0) {
    dev_info(&client->dev, "Camera detected: %s\n", device_name);

    // Could use for auto-detection
    if (strstr(device_name, "640x512")) {
        rs300->mode = &supported_modes[0];
    } else if (strstr(device_name, "256x192")) {
        rs300->mode = &supported_modes[1];
    }
}
```

**Estimated Time**: 3-4 hours

#### 3. Get Brightness / Get Colormap Commands

**Purpose**: Read back current settings for verification.

**Implementation**: Similar to Set commands, but read result buffer.

**Estimated Time**: 2 hours each

### Total Estimated Time: 10-12 hours (1-2 days)

---

## Priority 5: Pi 5 ISP Integration

### Current State

**What Works:**
- ✅ Raw YUV data from camera
- ✅ V4L2 streaming functional
- ✅ Documentation exists (RASPBERRY_PI_ISP_GUIDE.md)

**What Doesn't Work:**
- ❌ No ISP processing pipeline implemented
- ❌ No hardware noise reduction
- ❌ No dual-stream output

### Technical Analysis

**Raspberry Pi 5 ISP Architecture:**
```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│   RS300      │─────▶│  PiSP Backend│─────▶│  Output      │
│   (Raw YUV)  │      │  (pispbe)    │      │  (Processed) │
└──────────────┘      └──────────────┘      └──────────────┘
                             │
                             ├─ Temporal NR (TNR)
                             ├─ Spatial NR (SNR)
                             ├─ Dual output (full + thumb)
                             └─ Zero-copy processing
```

**Implementation Approach:**

### Phase 1: V4L2 M2M (Memory-to-Memory) Pipeline

**Concept**: Use pispbe as V4L2 M2M device for post-processing.

**Pipeline:**
```
RS300 (/dev/video0) → Capture raw YUV → pispbe (/dev/video20) → Processed output
```

**GStreamer Example:**
```bash
gst-launch-1.0 \
  v4l2src device=/dev/video0 ! \
  video/x-raw,format=UYVY,width=640,height=512 ! \
  v4l2convert device=/dev/video20 ! \
  video/x-raw,format=NV12 ! \
  autovideosink
```

**Requirements:**
- Driver already works ✅
- Need to configure pispbe device
- May need custom GStreamer pipeline or app

**Estimated Time**: 1 week (mostly testing and tuning)

### Phase 2: Integration with libcamera (if libcamera support done)

**Concept**: libcamera automatically routes through ISP.

**Benefits:**
- Automatic ISP configuration
- Tuning file support
- Simplified application code

**Requires**: Priority 3 (libcamera support) completed first.

### Phase 3: Kernel-Level Integration (Advanced)

**Concept**: Integrate ISP directly in media controller pipeline.

**Pipeline:**
```
rs300 → csi2 → rp1-cfe → pispbe (auto) → output
```

**Requires**: Media controller framework expertise, kernel development.

**Estimated Time**: 3-4 weeks

**Risk Level**: High

### Recommended Approach:

**Start with Phase 1** (V4L2 M2M):
- Easiest to implement
- Provides immediate value
- Works with existing driver
- Good for testing and benchmarking

**Move to Phase 2** after libcamera support is done.

**Consider Phase 3** only if submitting upstream.

---

## Overall Implementation Timeline

### Sprint 1 (Week 1-2): Quick Wins + Priority I2C Commands

**Days 1-2: Batch 1 - Autoshutter Commands** 🔥 **(User Priority #1)**
- Implement `rs300_set_autoshutter()` - Enable/disable automatic FFC
- Implement `rs300_get_autoshutter()` - Read autoshutter state
- Implement `rs300_set_autoshutter_params()` - Configure temperature threshold and intervals
- Add 4 new V4L2 controls (main switch, temperature, min interval, max interval)
- Testing and validation
- **See**: I2C_COMMANDS_TO_IMPLEMENT.md Batch 1 for implementation details
- **Estimated time**: 6-8 hours

**Days 3-4: Batch 2 - Module Sleep Commands** 🔥 **(User Priority #2)**
- Implement `rs300_set_sleep()` - Put module to sleep/wake
- Implement `rs300_get_sleep()` - Read sleep state
- Add 1 new V4L2 control (module_sleep boolean)
- Handle sleep state tracking in driver
- Testing and validation
- **See**: I2C_COMMANDS_TO_IMPLEMENT.md Batch 2 for implementation details
- **Estimated time**: 3-4 hours

**Days 5-6: udev-based Auto-Configuration**
- Create udev rule to trigger on RS300 device detection
- Create auto-configuration script
- Test across reboots
- **Estimated time**: 1-2 days

**Day 7: Test libcamera Simple Pipeline**
- Run `libcamera-hello --list-cameras`
- Test basic capture if detected
- Document results (determines if Sprint 5 needed)
- **Estimated time**: 4-6 hours

**Day 8: Documentation Updates**
- Update DEV_QUICK_REFERENCE.md with new controls
- Update I2C_PROTOCOL.md if needed
- Create examples for autoshutter and sleep
- **Estimated time**: 4 hours

**Sprint 1 Deliverables**: 5 I2C commands implemented (autoshutter + sleep), udev auto-config working, libcamera compatibility assessed

### Sprint 2 (Week 3-4): Runtime Resolution Switching + Device Info

**Days 1-2: Batch 3 - Device Information Commands** 🎯
- Implement `rs300_get_device_name()` - **KEY for auto-detection!**
- Implement `rs300_get_firmware_version()` - Diagnostics
- Implement `rs300_get_module_temperature()` - Detector temp readout
- Use device name in probe to log camera model
- Explore using device name for resolution auto-detection
- **See**: I2C_COMMANDS_TO_IMPLEMENT.md Batch 3 for implementation details
- **Estimated time**: 6-8 hours

**Days 3-4: Research Resolution Query Command**
- Contact Purple River about resolution query I2C command
- Test device name parsing for resolution hints (e.g., "MINI2640" vs "MINI2256")
- Design auto-detection fallback strategy
- **Estimated time**: 1-2 days

**Days 5-7: Implement Runtime Switching**
- Modify `rs300_set_pad_fmt()` to update active mode (`rs300->mode = mode;`)
- Add I2C command to switch camera resolution (if supported by firmware)
- Implement resolution auto-detection using device name (if possible)
- Fall back to module parameter if detection fails
- **Estimated time**: 3 days

**Days 8-10: Hardware Testing**
- Test on 640×512 module (have)
- Test on 384×288 module (need hardware)
- Test on 256×192 module (need hardware)
- Verify mode switching works without driver reload
- Test v4l2-compliance
- **Estimated time**: 2-3 days

**Sprint 2 Deliverables**: 3 more I2C commands (8 total), runtime resolution switching working, auto-detection implemented (if firmware supports)

### Sprint 3 (Week 5-6): ISP Integration + Enhancement Commands

**Days 1-2: Batch 4 - Image Enhancement Commands** 🎨 **(Optional - if time permits)**
- Implement `rs300_set_gamma()` - Gamma curve adjustment (0-100)
- Implement `rs300_get_gamma()` - Read gamma value
- Add V4L2 standard `V4L2_CID_GAMMA` control
- **See**: I2C_COMMANDS_TO_IMPLEMENT.md Batch 4 for implementation details
- **Estimated time**: 3-4 hours
- **Status**: Optional, defer to Sprint 6 if ISP work takes full 2 weeks

**Days 3-4: Set up V4L2 M2M Pipeline**
- Configure pispbe device for thermal processing
- Test RS300 → pispbe → output pipeline
- **Estimated time**: 2 days

**Days 5-6: Test and Benchmark**
- Compare direct vs ISP processing
- Measure FPS, latency, CPU usage
- Test temporal/spatial noise reduction
- **Estimated time**: 2 days

**Days 7-8: Create Examples and Documentation**
- Create GStreamer examples with ISP
- Document ISP configuration
- Performance tuning guide
- **Estimated time**: 2 days

**Days 9-10: Optimize Settings**
- Fine-tune noise reduction parameters
- Test dual-stream output
- Benchmark zero-copy performance
- **Estimated time**: 2 days

**Sprint 3 Deliverables**: ISP V4L2 M2M pipeline working, optionally 2 more I2C commands (10 total), examples and documentation

### Sprint 4 (Week 7-10): Modern Framework
- Implement v4l2_async framework (1-2 weeks)
- Testing and validation (3-4 days)
- Documentation (2 days)

### Sprint 5 (Week 11-14): libcamera (if needed)
- Custom pipeline handler (1-2 weeks)
- Testing and tuning (3-4 days)
- Upstream submission (3-4 days)

### Sprint 6 (Future): Advanced I2C Commands

**Batch 5: Advanced Features** (When needed)
- `rs300_set_edge_position()` - Edge enhancement position (0-2)
- `rs300_set_antiburn()` - Anti-burn protection for OLED displays
- `rs300_get_hotspot_coords()` - Find hottest point in ROI (very useful for analytics!)
- **See**: I2C_COMMANDS_TO_IMPLEMENT.md Batch 5 for implementation details
- **Estimated time**: 6-8 hours

**Batch 6: System Features** (When needed)
- `rs300_set_boot_logo()` - Show/hide boot logo
- `rs300_save_params()` - Save current settings to camera flash
- `rs300_restore_params()` - Restore saved settings
- **See**: I2C_COMMANDS_TO_IMPLEMENT.md Batch 6 for implementation details
- **Estimated time**: 4-6 hours

**Sprint 6 Deliverables**: 6 more I2C commands (16 total), advanced thermal analysis features

**Total Estimated Time**: 12-14 weeks (3-3.5 months) for Sprints 1-5, Sprint 6 as needed

---

## Success Criteria

### Priority 1: Runtime Resolution Switching
- [ ] Change resolution via `v4l2-ctl --set-fmt-video`
- [ ] No driver reload required
- [ ] Auto-detection works (if supported by firmware)
- [ ] Passes `v4l2-compliance` tests
- [ ] Works on all 3 hardware variants

### Priority 2: Modern Framework
- [ ] No manual `configure_media.sh` needed
- [ ] Camera works immediately after driver load
- [ ] Links auto-configured
- [ ] Works across reboots

### Priority 3: libcamera Support
- [ ] `libcamera-hello --list-cameras` detects RS300
- [ ] `libcamera-still` captures images
- [ ] `libcamera-vid` records video
- [ ] Controls accessible via libcamera API
- [ ] Performance matches V4L2 direct access

### Priority 4: Additional I2C Commands
**Batch 1 (Sprint 1)**:
- [ ] Autoshutter enable/disable working
- [ ] Autoshutter temperature threshold configurable
- [ ] Autoshutter min/max intervals configurable
- [ ] Autoshutter GET command returns state

**Batch 2 (Sprint 1)**:
- [ ] Module sleep/wake commands working
- [ ] Sleep state tracked in driver

**Batch 3 (Sprint 2)**:
- [ ] Get Device Name returns camera model (e.g., "Camera MINI2384")
- [ ] Get Firmware Version returns version string
- [ ] Get Module Temperature returns detector temp
- [ ] Device name used for resolution auto-detection (if possible)

**Batch 4 (Sprint 3 - Optional)**:
- [ ] Gamma control implemented using standard V4L2_CID_GAMMA

**Batches 5-6 (Sprint 6 - Future)**:
- [ ] Edge position, antiburn, hotspot, boot logo, save/restore implemented

**Overall**:
- [ ] All commands documented with examples
- [ ] All commands tested and validated

### Priority 5: ISP Integration
- [ ] Hardware-accelerated noise reduction working
- [ ] Dual-stream output functional
- [ ] Performance improvement measurable (FPS, latency)
- [ ] Zero-copy pipeline operational
- [ ] Examples and documentation provided

---

## Risk Assessment

| Task | Complexity | Risk | Mitigation |
|------|------------|------|------------|
| Runtime Resolution Switching | Medium | Low-Medium | Test thoroughly on all hardware |
| Modern Framework (async) | High | Medium | Start with udev fallback |
| libcamera Support | Medium-High | Medium | Try simple pipeline first |
| Additional Commands | Low | Low | Follow existing patterns |
| ISP Integration | Medium | Low-Medium | Start with V4L2 M2M |

---

## Resources Needed

### Hardware
- ✅ RS300 640×512 module (have)
- ⚠️ RS300 384×288 module (need for testing)
- ⚠️ RS300 256×192 module (need for testing)

### Documentation
- ✅ I2C Protocol (documented)
- ✅ Driver Analysis (documented)
- ⚠️ Resolution query command spec (need from Purple River)
- ⚠️ Firmware version info (need from Purple River)

### Expertise
- V4L2 kernel development (medium expertise)
- Media controller framework (need to learn)
- libcamera architecture (need to learn)
- GStreamer pipelines (basic knowledge)

---

## Next Steps

1. **Review this plan** - Confirm priorities and timeline
2. **Contact Purple River** - Ask about resolution query command
3. **Start Sprint 1** - Quick wins to build momentum
4. **Set up testing environment** - Prepare hardware and test cases
5. **Create feature branches** - One per priority for clean development

---

**Questions? Feedback? Adjustments?**

This is a living document. Update as development progresses and new information becomes available.
