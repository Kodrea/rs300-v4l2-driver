# RS300 Post-Reboot Functional Testing Guide

**Purpose**: Comprehensive testing protocol for AI assistants after system reboot
**Context**: Security fixes have been applied, need to validate functionality
**Expected Duration**: 10-15 minutes for complete test suite
**Prerequisites**: System has been rebooted after security fixes

---

## 🎯 Testing Objectives

1. ✅ Verify security-fixed driver loads correctly
2. ✅ Confirm device tree creates camera device
3. ✅ Validate media pipeline configuration
4. ✅ Test basic video streaming functionality
5. ✅ Verify all V4L2 controls work
6. ✅ **Capture and save thermal images for visual verification**
7. ✅ Validate security fixes don't break functionality
8. ✅ Document any issues found

---

## 📋 Phase 1: Pre-Flight Checks (CRITICAL)

**Purpose**: Verify system is in expected state before testing

### Step 1.1: Check Driver Loading
```bash
lsmod | grep rs300
```

**Expected Output**:
```
rs300    49152  0
```

**Success Criteria**:
- Module "rs300" appears in list
- Reference count is 0 or higher

**If FAIL**:
- Run: `dmesg | grep rs300 | tail -20` to check for errors
- Check: `ls /lib/modules/$(uname -r)/updates/dkms/rs300.ko.xz`
- Try: `sudo modprobe rs300`
- If still fails: Document error and STOP testing

---

### Step 1.2: Verify Device Instantiation
```bash
media-ctl -d /dev/media0 -p | grep rs300
```

**Expected Output**:
```
- entity 16: rs300 10-003c (2 pads, 2 links)
      type V4L2 subdev subtype Sensor flags 0
      device node name /dev/v4l-subdev2
```

**Success Criteria**:
- "rs300 10-003c" entity exists
- Has 2 pads, 2 links
- Device node /dev/v4l-subdev2 (or similar) exists

**If FAIL**:
- This is the device tree issue documented in DEVICE_TREE_ISSUE.md
- Check: `i2cdetect -y 10` - should show "UU" at address 0x3c
- Try: `ls /sys/bus/i2c/devices/10-003c/name` - should output "rs300"
- If device missing: Document and see DEVICE_TREE_ISSUE.md
- **STOP testing** - cannot proceed without device

---

### Step 1.3: Check I2C Communication
```bash
# This should show "Device or resource busy" (good - driver has it)
sudo i2cget -y 10 0x3c 0x00 2>&1 | head -1
```

**Expected Output**:
```
Error: Could not set address to 0x3c: Device or resource busy
```

**Success Criteria**:
- "Device or resource busy" message (means driver claimed the device)
- NOT "No such device" (would mean hardware problem)
- NOT success (would mean no driver bound to it)

**If FAIL**:
- If "No such device": Hardware not connected
- If successful read: Driver not bound to device
- Document and investigate before proceeding

---

### Step 1.4: Verify Device Nodes Created
```bash
ls -l /dev/v4l-subdev* | grep "Oct 22"
ls -l /dev/video0 /dev/video1 /dev/video2 2>/dev/null
```

**Expected Output**:
- At least one `/dev/v4l-subdev*` created today
- `/dev/video0` exists (rp1-cfe video node)

**Success Criteria**:
- Camera subdev exists (likely /dev/v4l-subdev2)
- Video capture node exists

**If FAIL**:
- Check `dmesg | grep -i "video\|v4l2"` for registration errors
- Document missing devices

---

## 📋 Phase 2: Media Pipeline Configuration

**Purpose**: Configure the media controller pipeline for video streaming

### Step 2.1: Run Automated Configuration
```bash
./configure_media.sh
```

**Expected Output**:
```
=== RS300 Media Controller Configuration ===
[✓] RS300 driver loaded
[✓] media-ctl available
[▶] Configuring media controller pipeline
[✓] Link configuration successful
[✓] Pad format set on rs300 10-003c:0
[✓] Pad format set on csi2:0
[✓] Pad format set on csi2:4
[✓] Video format set on /dev/video0
[✓] Configuration complete
```

**Success Criteria**:
- All steps show [✓] (green checkmarks)
- No error messages
- Script exits with code 0

**If FAIL**:
- Check which step failed
- Run manual configuration (see Step 2.2)
- Document error output

---

### Step 2.2: Manual Pipeline Configuration (If Step 2.1 Failed)
```bash
# Enable CSI-2 link
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set format on camera sensor
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set format on CSI-2 receiver input
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set format on CSI-2 receiver output
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set video device format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

**Success Criteria**:
- Each command completes without error
- No "Format mismatch!" messages
- `media-ctl -p` shows ENABLED links

---

### Step 2.3: Verify Pipeline Configuration
```bash
media-ctl -p | grep -A 3 "rs300 10-003c"
```

**Expected Output**:
```
- entity 16: rs300 10-003c (2 pads, 2 links)
             type V4L2 subdev subtype Sensor flags 0
             device node name /dev/v4l-subdev2
	pad0: Source
		[fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]
		-> "csi2":0 [ENABLED,IMMUTABLE]
```

**Success Criteria**:
- Format is UYVY8_1X16 (16-bit packed - required for Pi 5)
- Resolution is 640x512
- Link to csi2:0 is ENABLED

---

## 📋 Phase 3: Basic Streaming Test

**Purpose**: Verify video streaming works without crashes

### Step 3.1: Quick Stream Test (10 frames)
```bash
timeout 10 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10 --verbose 2>&1 | tee /tmp/stream_test.log
```

**Expected Output**:
```
<<<<<<<<<<
[... frame data ...]
```

**Success Criteria**:
- 10 `<` characters appear (one per frame)
- No error messages
- No kernel crashes (check `dmesg | tail`)
- Process completes successfully

**If FAIL**:
- Check `/tmp/stream_test.log` for errors
- Run `dmesg | tail -50` to check for kernel errors
- Common issues:
  - "Device or resource busy": Another process using camera
  - "Input/output error": Hardware communication failure
  - "Invalid argument": Format mismatch in pipeline

---

### Step 3.2: Extended Stream Test (30 seconds)
```bash
timeout 30 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 2>&1 | tee /tmp/stream_extended.log
```

**Success Criteria**:
- Runs for full 30 seconds without error
- Multiple frames captured (should see many `<` characters)
- No memory leaks (check `dmesg | tail` for OOM)
- System remains stable

---

## 📋 Phase 4: V4L2 Controls Testing

**Purpose**: Verify all camera controls work with security-fixed driver

### Step 4.1: List Available Controls
```bash
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
```

**Expected Output**:
```
brightness 0x00980900 (int)    : min=0 max=100 step=10 default=50 value=50
contrast 0x00980901 (int)      : min=0 max=100 step=10 default=50 value=50
zoom_absolute 0x009a090d (int) : min=1 max=8 step=1 default=1 value=1
colormap 0x0098c900 (menu)     : min=0 max=11 default=0 value=0
[... 11 controls total ...]
```

**Success Criteria**:
- 11 controls listed
- All have reasonable min/max/default values
- No error messages

---

### Step 4.2: Test Brightness Control
```bash
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=75
v4l2-ctl -d /dev/v4l-subdev2 -C brightness
```

**Expected Output**:
```
brightness: 75
```

**Success Criteria**:
- Set succeeds without error
- Get returns the value we set
- No kernel crashes (`dmesg | tail`)

---

### Step 4.3: Test Colormap Control (Visual Difference)
```bash
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=3  # Ironbow
v4l2-ctl -d /dev/v4l-subdev2 -C colormap
```

**Expected Output**:
```
colormap: 3
```

**Success Criteria**:
- Colormap changes without error
- Visual appearance should change (verify with captured image later)

---

### Step 4.4: Run Automated Control Tests
```bash
./test_controls.sh --quick
```

**Expected Output**:
```
[✓] All 11 controls tested successfully
```

**Success Criteria**:
- All controls pass
- No failures reported
- Script exits with code 0

---

## 📋 Phase 5: Image Capture & Visual Verification

**Purpose**: Capture actual thermal images for human visual inspection

### Step 5.1: Capture Raw Frame (UYVY)
```bash
mkdir -p ~/rs300-test-images
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY --stream-mmap --stream-count=1 --stream-to=/tmp/thermal_raw.uyvy
echo "Captured raw UYVY frame to /tmp/thermal_raw.uyvy (640x512)"
```

**Success Criteria**:
- File created: `/tmp/thermal_raw.uyvy`
- File size: 655,360 bytes (640 * 512 * 2 bytes per pixel)
- No errors during capture

---

### Step 5.2: Convert to Viewable Format (PNG)
```bash
# Convert UYVY to PNG using FFmpeg
ffmpeg -y -f rawvideo -pix_fmt uyvy422 -s 640x512 -i /tmp/thermal_raw.uyvy \
  ~/rs300-test-images/thermal_$(date +%Y%m%d_%H%M%S).png 2>&1 | tail -5

ls -lh ~/rs300-test-images/thermal_*.png | tail -1
```

**Success Criteria**:
- PNG file created in ~/rs300-test-images/
- File size reasonable (typically 50-200KB)
- FFmpeg completes without error

**Human Action Required**:
- View the PNG file to verify thermal image looks correct
- Check for: proper colormap, thermal gradient visible, no corruption

---

### Step 5.3: Capture Multiple Colormaps
```bash
# Create test images with different colormaps for comparison
for colormap in 0 3 4 10; do
  colormap_name=$(case $colormap in
    0) echo "WhiteHot";;
    3) echo "Ironbow";;
    4) echo "Rainbow";;
    10) echo "BlackHot";;
  esac)

  echo "Capturing $colormap_name (colormap=$colormap)..."
  v4l2-ctl -d /dev/v4l-subdev2 -c colormap=$colormap
  sleep 1

  v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=/tmp/thermal_$colormap_name.uyvy

  ffmpeg -y -f rawvideo -pix_fmt uyvy422 -s 640x512 -i /tmp/thermal_$colormap_name.uyvy \
    ~/rs300-test-images/thermal_${colormap_name}_$(date +%Y%m%d_%H%M%S).png 2>&1 | grep -q "Output"

  echo "✓ Saved: ~/rs300-test-images/thermal_${colormap_name}_*.png"
done

ls -lh ~/rs300-test-images/
```

**Success Criteria**:
- 4 PNG files created (WhiteHot, Ironbow, Rainbow, BlackHot)
- Each file 50-200KB
- Files have different colormaps (human verification needed)

---

### Step 5.4: Capture Video Clip (10 seconds)
```bash
# Capture 10 seconds of thermal video
timeout 10 gst-launch-1.0 v4l2src device=/dev/video0 num-buffers=300 ! \
  video/x-raw,format=UYVY,width=640,height=512,framerate=30/1 ! \
  videoconvert ! \
  x264enc bitrate=4000 ! \
  mp4mux ! \
  filesink location=~/rs300-test-images/thermal_video_$(date +%Y%m%d_%H%M%S).mp4 \
  2>&1 | grep -E "Setting|ERROR"

ls -lh ~/rs300-test-images/*.mp4 | tail -1
```

**Success Criteria**:
- MP4 file created in ~/rs300-test-images/
- File size > 1MB (indicates video was captured)
- No GStreamer errors

**Human Action Required**:
- Play the MP4 file to verify video quality
- Check for smooth playback, correct frame rate

---

## 📋 Phase 6: Security Fix Validation

**Purpose**: Verify security fixes don't break functionality

### Step 6.1: Test Module Removal (CRITICAL-004 Fix)
```bash
echo "Testing module removal (was guaranteed crash before fix)..."
sudo rmmod rs300 && echo "✓ Module removed successfully - CRITICAL-004 FIX VERIFIED" || echo "✗ Module removal failed"

# Wait a moment
sleep 2

# Check for kernel crashes
dmesg | tail -20 | grep -i "oops\|panic\|segfault" && echo "✗ KERNEL CRASH DETECTED" || echo "✓ No kernel crashes"

# Reload module
sudo modprobe rs300 && echo "✓ Module reloaded" || echo "✗ Reload failed (known device tree issue)"
```

**Success Criteria**:
- Module removes without crash (CRITICAL-004 fix working)
- No kernel oops/panic in dmesg
- System remains stable

**Note**: Module reload may fail due to device tree issue documented in DEVICE_TREE_ISSUE.md. This is expected and not a security issue.

---

### Step 6.2: Test ioctl Bounds Checking (CRITICAL-002 Fix)
```bash
# Reconfigure pipeline after reload (if reload worked)
./configure_media.sh 2>&1 | grep -q "Configuration complete" && echo "✓ Pipeline reconfigured"

# Test that normal operations still work (validates ioctl security fixes)
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls | head -3 && echo "✓ ioctl bounds checking not breaking normal operation"
```

**Success Criteria**:
- Controls still accessible
- No crashes from ioctl operations
- Security fixes transparent to normal use

---

### Step 6.3: Test Concurrent Streaming (CRITICAL-001 Fix - Race Condition)
```bash
echo "Testing concurrent operations (race condition fix)..."

# Start two captures in background simultaneously
timeout 5 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=50 >/tmp/stream1.log 2>&1 &
PID1=$!

timeout 5 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=50 >/tmp/stream2.log 2>&1 &
PID2=$!

# Wait for both to complete
wait $PID1
RET1=$?
wait $PID2
RET2=$?

echo "Stream 1 exit code: $RET1"
echo "Stream 2 exit code: $RET2"

# Check for crashes or data corruption
dmesg | tail -20 | grep -i "oops\|panic\|corruption" && echo "✗ Issues detected" || echo "✓ Concurrent access safe (CRITICAL-001 fix working)"
```

**Success Criteria**:
- Both streams complete (one might fail with "busy" - expected)
- No kernel crashes
- No data corruption messages
- Race condition fix prevents corruption

---

## 📋 Phase 7: Results Summary

### Step 7.1: Generate Test Report
```bash
cat > ~/rs300-test-images/TEST_REPORT_$(date +%Y%m%d_%H%M%S).txt <<EOF
RS300 Security-Fixed Driver Test Report
========================================
Test Date: $(date)
Kernel: $(uname -r)
Driver: rs300 (security fixes applied 2025-10-22)

PHASE 1: Pre-Flight Checks
---------------------------
Driver Loaded: $(lsmod | grep -q rs300 && echo "PASS" || echo "FAIL")
Device Instantiated: $(media-ctl -d /dev/media0 -p | grep -q rs300 && echo "PASS" || echo "FAIL")
I2C Communication: $(sudo i2cget -y 10 0x3c 0x00 2>&1 | grep -q "busy" && echo "PASS" || echo "FAIL")

PHASE 2: Media Pipeline
------------------------
Pipeline Configured: $(media-ctl -p | grep rs300 | grep -q ENABLED && echo "PASS" || echo "CHECK LOG")

PHASE 3: Streaming
------------------
Basic Stream Test: [Check /tmp/stream_test.log]
Extended Stream Test: [Check /tmp/stream_extended.log]

PHASE 4: Controls
-----------------
Controls Accessible: $(v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls | grep -q brightness && echo "PASS" || echo "FAIL")
Control Test Script: [Check ./test_controls.sh output]

PHASE 5: Image Capture
----------------------
Captured Images: $(ls ~/rs300-test-images/*.png 2>/dev/null | wc -l) PNG files
Captured Videos: $(ls ~/rs300-test-images/*.mp4 2>/dev/null | wc -l) MP4 files

PHASE 6: Security Validation
-----------------------------
Module Removal (CRITICAL-004): $(dmesg | tail -50 | grep -q "rs300.*removed" && echo "PASS" || echo "CHECK DMESG")
ioctl Operations (CRITICAL-002/003): [Check if controls work after all tests]
Concurrent Access (CRITICAL-001): [Check /tmp/stream1.log and /tmp/stream2.log]

OVERALL STATUS
--------------
$(ls ~/rs300-test-images/*.png 2>/dev/null | wc -l | grep -q "[1-9]" && echo "✓ TESTS PASSED - Images captured successfully" || echo "⚠ Check individual test logs")

Captured Files:
$(ls -lh ~/rs300-test-images/)
EOF

cat ~/rs300-test-images/TEST_REPORT_*.txt
```

---

### Step 7.2: Human Review Checklist

**AI: Present this checklist to the human:**

Please review the following:

1. **Visual Image Quality**:
   - [ ] Open PNG images in ~/rs300-test-images/
   - [ ] Verify thermal gradient visible (hot=bright or reversed based on colormap)
   - [ ] Check for image corruption, artifacts, or noise
   - [ ] Compare different colormaps (WhiteHot, Ironbow, Rainbow, BlackHot)

2. **Video Quality**:
   - [ ] Play MP4 video file
   - [ ] Verify smooth playback at ~30fps
   - [ ] Check for frame drops or stuttering
   - [ ] Thermal content should be visible throughout

3. **Test Logs**:
   - [ ] Review /tmp/stream_test.log for any warnings
   - [ ] Review /tmp/stream_extended.log for stability
   - [ ] Check dmesg for any kernel warnings: `dmesg | tail -100`

4. **Security Validation**:
   - [ ] Confirm no kernel crashes after all tests
   - [ ] Verify module removal didn't crash system
   - [ ] Check that all security fixes are transparent (no performance impact)

---

## 🚨 Troubleshooting Decision Tree

### Issue: Device Not Found After Reboot

**Symptoms**: `media-ctl -e "rs300"` returns "Entity not found"

**Diagnosis**:
1. Check: `lsmod | grep rs300` - Is driver loaded?
   - No: `sudo modprobe rs300`
2. Check: `i2cdetect -y 10` - Is device at 0x3c?
   - UU: Driver has it (good)
   - Number: Nothing bound to it
   - --: Hardware not detected
3. Check: `dmesg | grep rs300` - Any errors?

**Solution**: See DEVICE_TREE_ISSUE.md for comprehensive investigation

---

### Issue: "Format mismatch!" Error

**Symptoms**: Pipeline configuration fails with format mismatch

**Cause**: Attempting to use 8-bit formats on Pi 5 (only 16-bit packed supported)

**Solution**:
- Always use `UYVY8_1X16` or `YUYV8_1X16` (16-bit packed)
- Never use `*8_2X8` formats on Pi 5
- Check: `media-ctl -p` to see current formats

---

### Issue: Streaming Returns "Device or resource busy"

**Symptoms**: `v4l2-ctl --stream-mmap` fails with EBUSY

**Diagnosis**:
1. Check: `sudo lsof /dev/video0` - What has it open?
2. Kill competing process or wait for it to finish

**Solution**:
```bash
# Find and kill process using camera
sudo fuser -k /dev/video0
# Wait a moment
sleep 2
# Retry streaming
```

---

### Issue: No Frames Captured (Timeout)

**Symptoms**: Stream command waits forever, no `<` characters

**Diagnosis**:
1. Check: `dmesg | tail -50` - Hardware errors?
2. Check: `v4l2-ctl -d /dev/video0 --all` - Format correct?
3. Check: `media-ctl -p` - Links enabled?

**Solution**:
- Run `./configure_media.sh` again
- Verify camera hardware connected properly
- Check cable connections (22-pin to 15-pin adapter)

---

## ✅ Success Criteria Summary

**Minimum Requirements for PASS**:
1. ✅ Driver loads without errors
2. ✅ Device tree instantiates camera
3. ✅ Media pipeline configures successfully
4. ✅ At least one image captured and saved
5. ✅ Module removal doesn't crash (CRITICAL-004 fix verified)
6. ✅ No kernel crashes during any test

**Ideal Success (All Tests Pass)**:
1. ✅ All 11 V4L2 controls accessible and functional
2. ✅ Multiple images with different colormaps captured
3. ✅ Video clip captured successfully
4. ✅ No security regression (ioctl, memory management work)
5. ✅ Concurrent operations safe (race condition fixed)
6. ✅ System remains stable throughout all tests

---

## 📊 Expected Timeline

| Phase | Duration | Critical |
|-------|----------|----------|
| Phase 1: Pre-Flight | 2 min | YES |
| Phase 2: Pipeline Config | 1 min | YES |
| Phase 3: Streaming Tests | 2 min | YES |
| Phase 4: Controls Tests | 3 min | NO |
| Phase 5: Image Capture | 5 min | YES |
| Phase 6: Security Validation | 2 min | YES |
| Phase 7: Report Generation | 1 min | NO |
| **Total** | **15-20 min** | |

---

## 📝 Final Notes for AI Assistants

**Execution Strategy**:
1. Follow phases sequentially - don't skip ahead
2. If a CRITICAL phase fails, document and STOP
3. Non-critical failures can be noted and continued past
4. Always capture output of failing commands
5. Check `dmesg | tail` after any failure

**Success Definition**:
- **MINIMUM**: Can capture at least one thermal image file
- **FULL**: All tests pass, all images captured, no crashes

**Documentation**:
- Save all test outputs to ~/rs300-test-images/
- Generate comprehensive TEST_REPORT
- List all captured files with timestamps
- Note any failures or anomalies

**Human Handoff**:
- Present test report summary
- Show paths to captured images
- Request human visual verification of image quality
- Report any security concerns immediately

---

**Document Version**: 1.0
**Last Updated**: October 22, 2025
**Next Review**: After first complete test run
