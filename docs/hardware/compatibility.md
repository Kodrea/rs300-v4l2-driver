# Hardware Compatibility

This page provides detailed compatibility information for the RS300 thermal camera across different Raspberry Pi platforms.

## Platform Testing Matrix

| Raspberry Pi Model | Module Resolution | Connection Type | OS & Kernel | Status | Notes |
| ------------------ | ----------------- | --------------- | ----------- | -------------- | -------------------------------------------------------------- |
| **Pi 5** | 640×512 | MIPI CSI-2 | Bookworm | ✅ **WORKING** | 60fps thermal streaming with media controller pipeline |
| **Pi 4B** | 640×512 | MIPI CSI-2 | Bookworm | ✅ Working | 60Hz video. Low-voltage warning; high current draw on 3.3V CSI port. Rarely causes issues |
| **Pi 4B** | 384×288 | MIPI CSI-2 | Bookworm | ✅ Working | 60Hz video |
| **Pi 4B** | 256×192 | MIPI CSI-2 | Bookworm | ⚠️ *Working | *Purple River tested the 256 with my driver and it worked. They believe my module's firmware is the issue and are sending me instructions to update it |
| **Pi Zero 2 W** | 640×512 | MIPI CSI-2 | Bookworm | ❌ Brownouts | Camera startup draws too much current, maybe possible in a later board revision |

## Platform-Specific Notes

### Raspberry Pi 5 (Recommended)
- **Status**: ✅ **Fully Working - Production Ready**
- **Architecture**: BCM2712 with dedicated CSI0 port via RP1 controller
- **I2C Bus**: i2c-10 through i2c_csi_dsi0 controller
- **Performance**: 640×512@60fps thermal imaging
- **Cable**: Requires 22-pin to 15-pin adapter for camera connection
- **Driver**: Uses rp1-cfe (Camera Front End), not legacy Unicam
- **Format Compatibility**: Only 16-bit packed formats (UYVY8_1X16, YUYV8_1X16)
- **Features**:
  - Media controller pipeline configuration
  - All 11 V4L2 controls accessible
  - Hardware ISP integration ready
  - Automatic configuration via `./configure_media.sh`

**Device Tree Configuration** (`/boot/firmware/config.txt`):
```
camera_auto_detect=0
dtoverlay=rs300
```

### Raspberry Pi 4B
- **Status**: ✅ Working (Manual Configuration Required)
- **Architecture**: Uses legacy Unicam driver system
- **I2C Bus**: i2c-1
- **Performance**: 640×512@60fps and 384×288@60fps confirmed working
- **Limitations**:
  - Requires manual driver modification for resolution selection
  - Low-voltage warnings possible due to 3.3V CSI port current draw (rarely causes issues)
  - No media controller pipeline (Unicam only)

**Known Issues**:
- **640×512 module**: High current draw on 3.3V CSI port can trigger low-voltage warnings, but rarely causes operational issues
- **256×192 module**: MIPI video troubleshooting in progress. I2C commands work and camera operates normally (shutter audible, CVBS works), but MIPI video not streaming. USB mode works at 50Hz.

### Raspberry Pi Zero 2W
- **Status**: ❌ Not Compatible (Power Limitations)
- **Issue**: Camera startup draws too much current, causing brownouts and reboots
- **Attempted Solutions**:
  - Powered 5V rail directly with DC PSU → still brownouts
  - External power via 5V/GND connectors → not possible with MIPI cable connected
- **Future**: May be possible with board revision that includes dedicated 5V pins for external power while MIPI ribbon cable is connected

## Module Resolution Testing Status

### 640×512 Module
- ✅ **Pi 5**: Fully working, 60fps
- ✅ **Pi 4B**: Working, 60fps
- ❌ **Pi Zero 2W**: Power issues

### 384×288 Module
- 🧪 **Pi 5**: Not yet tested (should work)
- ✅ **Pi 4B**: Working, 60fps

### 256×192 Module
- 🧪 **Pi 5**: Not yet tested
- ⚠️ **Pi 4B**: I2C working, MIPI video troubleshooting in progress
  - Symptoms: No video when opening camera, no data with `--streammap`
  - Confirmed working: I2C commands, shutter calibration (audible), CVBS video output
  - Workaround: USB mode provides 25/50fps streaming
  - Status: On back burner, 50Hz available via USB

## Connection Requirements

### Raspberry Pi 5
- **Cable**: 22-pin to 15-pin adapter required
- **Port**: CSI0 (CAM0) port on Pi 5
- **Board**: Custom RPi adapter board from Purple River Technology

### Raspberry Pi 4B
- **Cable**: Standard 15-pin reverse (same-sided) ribbon cable
- **Port**: Either CSI camera port
- **Board**: Custom RPi adapter board from Purple River Technology

### Important Cable Note
The original board required a same-sided ribbon cable. The updated design is compatible with the reverse head cable that Raspberry Pi cameras generally use. This is **critical** for using the 15-22 pin adapter cable for Pi 5, Zero, and Compute Modules.

## Power Considerations

### Current Draw
The RS300 thermal camera has significant current requirements during startup and operation:

- **Pi 5**: No power issues reported
- **Pi 4B**: Can trigger low-voltage warnings due to 3.3V CSI port current draw
  - **Impact**: Rarely causes operational issues
  - **Mitigation**: Ensure adequate power supply (official 3A+ recommended)
- **Pi Zero 2W**: Current draw exceeds platform capabilities
  - **Impact**: Brownouts and system reboots
  - **Status**: Not supported on current hardware

### Power Supply Recommendations
- **Pi 5**: Official 27W USB-C power supply (5.1V/5A)
- **Pi 4B**: Official 15W USB-C power supply (5.1V/3A) minimum
- **Important**: Underpowered supplies will cause issues, especially during FFC calibration and startup

## OS Requirements

### Supported
- **Raspberry Pi OS Bookworm** (current, recommended)
  - Kernel: 6.6.y or later
  - Full support for both Pi 5 (rp1-cfe) and Pi 4 (Unicam)

### Untested
- **Raspberry Pi OS Bullseye**: May work but not tested
- **Other distributions**: Ubuntu, etc. not tested

## Future Testing

### Planned
- [ ] Test 384×288 module on Pi 5
- [ ] Test 256×192 module on Pi 5
- [ ] Continue troubleshooting 256×192 MIPI data on Pi 4
- [ ] ISP integration for hardware denoising (Pi 5)

---

**Next Steps**:
- [View Technical Specifications →](specifications.md)
- [Where to Purchase Hardware →](purchasing-guide.md)
- [Install Driver →](../getting-started/)
