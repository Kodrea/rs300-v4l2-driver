# Test RS300 Camera and Driver

Run a comprehensive test of the RS300 thermal camera and driver to verify:

1. **Driver Status**: Check if the rs300 driver is loaded
2. **I2C Detection**: Verify camera is detected on I2C bus 10 at address 0x3c
3. **Media Pipeline**: Verify media controller pipeline is configured
4. **Video Streaming**: Test that the camera can stream video (capture a few frames)
5. **Core Controls**: Test essential V4L2 controls work without errors:
   - FFC (Flat Field Calibration) trigger
   - Brightness adjustment (test setting to different values)
   - Colormap switching (test a few colormaps)

## Test Procedure

Execute the following checks in order and report clear PASS/FAIL results:

### 1. Driver Check
```bash
lsmod | grep rs300
dmesg | grep -i rs300 | tail -5
```
Expected: Driver loaded, no critical errors in dmesg

### 2. I2C Detection
```bash
i2cdetect -y 10
```
Expected: Device visible at address 0x3c (showing "3c" or "UU")

### 3. Media Pipeline Status
```bash
media-ctl -p | grep -A5 "rs300"
```
Expected: rs300 entity present and pipeline shows ENABLED links

### 4. Video Streaming Test
```bash
# Run for 25 seconds (3s warm-up + 22s measurement) to get accurate stable FPS
# Camera has 2-3s warm-up period that affects early FPS readings
# Save output to file, then extract final FPS
timeout 25 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=600 2>&1 | tee /tmp/stream_test.txt
tail -5 /tmp/stream_test.txt | grep -oP '\d+\.\d+ fps' | tail -1
```
Expected: Successfully captures frames and final output shows stable FPS (typically 30.00 fps or 60.00 fps depending on driver config)

### 5. FFC Test
```bash
v4l2-ctl -d /dev/v4l-subdev2 -c ffc_trigger=1
sleep 2
dmesg | grep -i "ffc\|shutter" | tail -3
```
Expected: Command succeeds, dmesg shows FFC execution

### 6. Brightness Test
```bash
# Test setting brightness to different values
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=25
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=75
v4l2-ctl -d /dev/v4l-subdev2 -C brightness
```
Expected: Commands succeed, readback shows correct value (75)

### 7. Colormap Test
```bash
# Test switching between colormaps
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=0  # White Hot
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=1  # Ironbow
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=6  # Rainbow
v4l2-ctl -d /dev/v4l-subdev2 -C colormap
```
Expected: Commands succeed, readback shows correct value (6)

## Output Format

For each test, report:
- ✅ **PASS**: Test succeeded with expected results
- ❌ **FAIL**: Test failed (include error details)
- ⚠️ **WARN**: Test passed but with unexpected output

Provide a summary at the end:
```
=== TEST SUMMARY ===
Total Tests: 7
Passed: X
Failed: Y
Warnings: Z

Overall Status: [PASS/FAIL]
```

## Notes
- Run tests sequentially (don't parallelize)
- Capture relevant error messages from dmesg if any test fails
- If video streaming fails, check if media pipeline is configured (suggest running ./configure_media.sh)
- If driver not loaded, suggest running sudo modprobe rs300 or rebooting
- **Video streaming test takes ~25 seconds**: This accounts for the camera's 2-3s warm-up period and allows FPS measurement to stabilize. Early FPS readings (2-4 fps) are normal during warm-up and gradually increase to stable performance (30 fps) after ~18-20 seconds.
