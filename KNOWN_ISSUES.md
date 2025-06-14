# RS300 Driver - Known Issues

## Current Status
The RS300 V4L2 driver successfully loads and detects the thermal camera on Raspberry Pi 5 (BCM2712), but video streaming has remaining issues.

## Known Issues

### 1. Video Streaming Format Mismatch (HIGH PRIORITY)

**Problem:**
- `VIDIOC_STREAMON` fails with "Invalid argument" error
- Kernel reports: `rp1-cfe 1f00110000.csi: Format mismatch!`
- `rp1-cfe 1f00110000.csi: Failed to start media pipeline: -22`

**Technical Details:**
- RS300 outputs: `YUYV8_2X8/640x512` format
- CSI2 interface configured for same format but pipeline validation fails
- Media controller links are properly established
- All subdevice formats appear correctly configured

**Impact:**
- Camera detection and control work properly
- Cannot capture video frames from thermal camera
- V4L2 controls (brightness, colormap, FFC) function correctly

**Investigation Needed:**
- Format negotiation between RS300 driver and rp1-cfe
- Potential colorspace/quantization mismatch
- Clock domain or timing synchronization issues
- MIPI CSI-2 D-PHY configuration compatibility

**Workaround:**
None currently available.

---

### 2. Device Tree Overlay Boot Loading (MEDIUM PRIORITY)

**Problem:**
Device tree overlay doesn't load automatically at boot despite being configured in `/boot/firmware/config.txt`.

**Current Behavior:**
- Manual loading with `sudo dtoverlay rs300` works correctly
- Boot-time loading via `dtoverlay=rs300` in config.txt fails silently
- No error messages in kernel log about overlay loading

**Impact:**
- Requires manual overlay loading after each boot
- Driver won't initialize automatically on system startup

**Workaround:**
Add `sudo dtoverlay rs300` to system startup scripts or load manually.

---

### 3. Empty Test Frame Capture (LOW PRIORITY)

**Problem:**
Test frame capture creates empty file due to streaming failure.

**Technical Details:**
- `v4l2-ctl --stream-mmap` creates `/tmp/test_frame.yuv` with 0 bytes
- Related to primary streaming issue above

**Impact:**
- Cannot verify camera image quality or thermal data output
- Unable to test colormap and image processing features

**Dependencies:**
This issue will be resolved when streaming format mismatch is fixed.

---

## Testing Commands

### Verify Driver Status
```bash
# Check module loading
lsmod | grep rs300

# Check I2C communication  
i2cdetect -y 10

# Check media controller topology
media-ctl -d /dev/media3 -p
```

### Reproduce Streaming Issue
```bash
# Configure pipeline
media-ctl -d /dev/media3 -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -d /dev/media3 -V "'rs300 10-003c':0 [fmt:YUYV8_2X8/640x512]"
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV

# Attempt streaming (will fail)
v4l2-ctl -d /dev/video0 --verbose --stream-mmap --stream-count=1
```

### Check Error Messages
```bash
# Monitor kernel messages
dmesg -wH | grep -E "(rs300|rp1-cfe|format|error)"
```

---

## Development Notes

### Next Steps
1. **Investigate rp1-cfe format validation** - Compare with working Pi 5 camera drivers
2. **Review MIPI CSI-2 timing parameters** - Verify link frequencies and clock settings  
3. **Test alternative pixel formats** - Try different mediabus formats if supported
4. **Debug format negotiation** - Add detailed logging to RS300 driver format functions

### Working Components
- ✅ Device tree overlay compilation and loading
- ✅ I2C communication (camera responds at 0x3c)
- ✅ Driver initialization and probe functions
- ✅ Media controller entity registration
- ✅ V4L2 subdevice and controls creation
- ✅ Format configuration through media-ctl

### Hardware Verification
- RS300 thermal camera properly connected via 22-pin to 15-pin adapter
- I2C communication established on bus i2c-10
- MIPI CSI-2 lanes configured for 2-lane operation
- Clock frequency set to 24MHz (cam0_clk)

---

## Contact Information
For questions or contributions related to these issues, please refer to the project repository and development branch `pi5-testing`.