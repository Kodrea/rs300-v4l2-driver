# RS300 Driver - Developer Quick Reference

**Quick navigation for common development tasks**

---

## Table of Contents
1. [Quick Start](#quick-start)
2. [Common Commands](#common-commands)
3. [V4L2 Control Reference](#v4l2-control-reference)
4. [Debug & Troubleshooting](#debug--troubleshooting)
5. [Code Locations](#code-locations)
6. [Testing Checklist](#testing-checklist)

---

## Quick Start

### Build & Install
```bash
# Clean build and install
./setup.sh

# Reboot to load driver
sudo reboot

# Configure media pipeline (after boot)
./configure_media.sh

# Verify driver loaded
lsmod | grep rs300
```

### Quick Test
```bash
# Check device detection
v4l2-ctl --list-devices

# View current format
v4l2-ctl -d /dev/video0 --get-fmt-video

# Test brightness control
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=75

# Capture test frame
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=test.yuv
```

---

## Common Commands

### Device Information
```bash
# List all V4L2 devices
v4l2-ctl --list-devices

# Show driver info
v4l2-ctl -d /dev/video0 --info

# Show subdevice info
v4l2-ctl -d /dev/v4l-subdev2 --info

# Show all controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls-menus

# Media controller topology
media-ctl -p
```

### Format Configuration
```bash
# Show current format
v4l2-ctl -d /dev/video0 --get-fmt-video

# Set 640x512 @ 60fps
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY

# View supported formats
v4l2-ctl -d /dev/video0 --list-formats-ext
```

### Control Operations
```bash
# List all controls with current values
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls

# Set brightness (0-100)
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=50

# Set colormap (0-11, see colormap table)
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=3

# Trigger FFC (flat field correction)
v4l2-ctl -d /dev/v4l-subdev2 -c ffc_trigger=1

# Set zoom (1-8)
v4l2-ctl -d /dev/v4l-subdev2 -c zoom_absolute=2

# Set scene mode (0-9, see scene mode table)
v4l2-ctl -d /dev/v4l-subdev2 -c scene_mode=3
```

### Video Capture
```bash
# Capture 10 frames to file
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10 --stream-to=capture.yuv

# Capture with verbose output
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=test.yuv --verbose

# Convert YUV to viewable format (requires ffmpeg)
ffmpeg -f rawvideo -pix_fmt uyvy422 -s 640x512 -i capture.yuv output.png
```

### I2C Direct Access
```bash
# Scan I2C bus 10 for device at 0x3c
i2cdetect -y 10

# Read status register (0x0200)
i2cget -y 10 0x3c 0x0200 w

# Dump registers (use with caution)
i2cdump -y 10 0x3c
```

---

## V4L2 Control Reference

### Standard Controls

| Control Name | ID | Type | Range | Default | Description |
|--------------|-----|------|-------|---------|-------------|
| `brightness` | V4L2_CID_BRIGHTNESS | Integer | 0-100 | 50 | Thermal brightness level |
| `contrast` | V4L2_CID_CONTRAST | Integer | 0-100 | 50 | Image contrast |
| `zoom_absolute` | V4L2_CID_ZOOM_ABSOLUTE | Integer | 1-8 | 1 | Digital zoom (1x-8x) |

### Custom Controls

| Control Name | Custom ID | Type | Range | Default | Description |
|--------------|-----------|------|-------|---------|-------------|
| `colormap` | CUSTOM+1 | Menu | 0-11 | 0 | Color palette selection |
| `ffc_trigger` | CUSTOM+2 | Button | - | - | Trigger flat field calibration |
| `scene_mode` | CUSTOM+3 | Menu | 0-9 | 3 | Scene optimization mode |
| `digital_detail_enhancement` | CUSTOM+4 | Integer | 0-100 | 50 | Edge enhancement (DDE) |
| `spatial_noise_reduction` | CUSTOM+5 | Integer | 0-100 | 50 | Spatial noise filter |
| `temporal_noise_reduction` | CUSTOM+6 | Integer | 0-100 | 50 | Temporal noise filter |

### Read-Only Controls

| Control Name | Value | Description |
|--------------|-------|-------------|
| `pixel_rate` | 200-400 MHz | Dynamic based on format |
| `link_freq` | 80 MHz | MIPI CSI-2 link frequency |

### Colormap Options (0-11)

```
0  = White Hot          - Hot objects appear white
1  = Reserved           - Not implemented
2  = Sepia              - Classic sepia tone
3  = Ironbow            - Iron/rainbow gradient
4  = Rainbow            - Full spectrum rainbow
5  = Night              - Night vision palette
6  = Aurora             - Aurora-style colors
7  = Red Hot            - Hot objects appear red
8  = Jungle             - Green-based palette
9  = Medical            - Medical imaging optimized
10 = Black Hot          - Hot objects appear black (inverted)
11 = Golden Red Glory   - Golden-red gradient
```

**Example:**
```bash
# Set to Ironbow palette
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=3
```

### Scene Mode Options (0-9)

```
0 = Low                 - Low dynamic range
1 = Linear Stretch      - Linear histogram stretch
2 = Low Contrast        - Minimal contrast
3 = General Mode        - Default balanced mode
4 = High Contrast       - Maximum contrast
5 = Highlight           - Highlight hot spots
6 = Reserved 1          - Not implemented
7 = Reserved 2          - Not implemented
8 = Reserved 3          - Not implemented
9 = Outline Mode        - Edge detection mode
```

**Example:**
```bash
# Set to High Contrast mode
v4l2-ctl -d /dev/v4l-subdev2 -c scene_mode=4
```

---

## Debug & Troubleshooting

### Kernel Logs

```bash
# Watch driver messages in real-time
dmesg -wH | grep rs300

# View recent driver messages
dmesg | grep rs300 | tail -50

# Clear kernel log and watch fresh
sudo dmesg -C
dmesg -wH

# Check for errors only
dmesg | grep -i "rs300.*error"
```

### Common Log Messages

| Message | Location | Meaning |
|---------|----------|---------|
| `Starting rs300_probe` | rs300.c:2765 | Driver initialization started |
| `Sensor powered on successfully` | rs300.c:2818 | Power-on complete |
| `Control handler initialized` | rs300.c:2585 | V4L2 controls ready |
| `=== RS300_SET_STREAM CALLED` | rs300.c:2105 | Stream start/stop |
| `Stream started successfully` | rs300.c:2191 | Video streaming active |
| `Format code 0x... not found` | rs300.c:394 | Invalid format requested |

### Media Pipeline Verification

```bash
# Check media links are enabled
media-ctl -p | grep "\[ENABLED\]"

# Should see:
# 'csi2':4 -> 'rp1-cfe-csi2_ch0':0 [ENABLED]

# Check pad formats match
media-ctl -p | grep -A5 "rs300"

# Verify format on all pads
media-ctl -p | grep "fmt:UYVY8_1X16"
```

### I2C Communication Test

```bash
# Test I2C bus is accessible
i2cdetect -y 10

# Should show device at 0x3c:
#      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:          -- -- -- -- -- -- -- -- -- -- -- -- --
# 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 30: -- -- -- -- -- -- -- -- -- -- -- -- 3c -- -- --

# Read status register
i2cget -y 10 0x3c 0x02 b 0x00

# 0x00 = idle, 0x01 = busy, 0x02 = failed
```

### Driver Module Info

```bash
# Check module is loaded
lsmod | grep rs300

# Show module parameters
cat /sys/module/rs300/parameters/mode
cat /sys/module/rs300/parameters/fps
cat /sys/module/rs300/parameters/type
cat /sys/module/rs300/parameters/debug

# Module details
modinfo rs300

# Reload with different parameters
sudo rmmod rs300
sudo modprobe rs300 mode=0 fps=60 debug=1
```

### Performance Monitoring

```bash
# Check frame rate
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 2>&1 | grep fps

# Monitor I2C errors
watch -n1 'dmesg | grep -i "i2c.*error" | tail -5'

# Check CSI-2 errors
cat /sys/kernel/debug/csi2/csi2_regs
```

---

## Code Locations

### Key Functions by Feature

| Feature | Function | File Location |
|---------|----------|---------------|
| **Driver Init** | `rs300_probe()` | rs300.c:2759-2893 |
| **Stream Control** | `rs300_set_stream()` | rs300.c:2097-2256 |
| **Format Negotiation** | `rs300_set_pad_fmt()` | rs300.c:1850-1951 |
| **Control Handler** | `rs300_set_ctrl()` | rs300.c:1626-1680 |
| **I2C Read** | `read_regs()` | rs300.c:205-231 |
| **I2C Write** | `write_regs()` | rs300.c:233-267 |
| **CRC Calculation** | `do_crc()` | rs300.c:152-170 |

### Camera Commands

| Command | Function | Line Range |
|---------|----------|------------|
| Brightness GET | `rs300_get_brightness()` | 502-633 |
| Brightness SET | `rs300_brightness_correct()` | 1327-1456 |
| Colormap GET | `rs300_get_colormap()` | 991-1078 |
| Colormap SET | `rs300_set_colormap()` | 1081-1213 |
| FFC Trigger | `rs300_shutter_cal()` | 1215-1325 |
| Zoom | `rs300_set_zoom()` | 1458-1539 |
| Scene Mode | `rs300_set_scene_mode()` | 1541-1624 |
| Contrast | `rs300_set_contrast()` | 778-847 |
| DDE | `rs300_set_dde()` | 635-704 |
| Spatial NR | `rs300_set_spatial_nr()` | 849-918 |
| Temporal NR | `rs300_set_temporal_nr()` | 920-989 |
| YUV Format | `rs300_set_yuv_format()` | 706-776 |
| FPS Setting | `rs300_set_fps()` | 1997-2080 |

### Data Structures

| Structure | Purpose | Line |
|-----------|---------|------|
| `struct rs300` | Main driver state | 303-340 |
| `struct rs300_mode` | Video mode definition | 275-280 |
| `struct ioctl_data` | Legacy IOCTL structure | 116-124 |
| `supported_modes[]` | Available resolutions | 342-371 |
| `codes[]` | Supported media bus formats | 295-301 |

### Important Constants

| Constant | Value | Line | Purpose |
|----------|-------|------|---------|
| `RS300_LINK_RATE` | 80 MHz | 43 | MIPI CSI-2 link frequency |
| `RS300_PIXEL_RATE` | 200 MHz | 44 | 8-bit format pixel rate |
| `RS300_PIXEL_RATE_16BIT` | 400 MHz | 45 | 16-bit format pixel rate |
| `I2C_VD_BUFFER_RW` | 0x1d00 | 128 | Command/data buffer register |
| `I2C_VD_BUFFER_STATUS` | 0x0200 | 135 | Status register |

---

## Testing Checklist

### After Code Changes

- [ ] Clean build: `./setup.sh`
- [ ] Reboot system
- [ ] Check driver loaded: `lsmod | grep rs300`
- [ ] Configure pipeline: `./configure_media.sh`
- [ ] Verify no errors in dmesg: `dmesg | grep -i error`
- [ ] Test basic capture: `v4l2-ctl --stream-count=1`
- [ ] Test all controls (see test script)
- [ ] Check frame rate matches expected

### Control Testing

```bash
# Brightness range test
for i in 0 25 50 75 100; do
    echo "Testing brightness=$i"
    v4l2-ctl -d /dev/v4l-subdev2 -c brightness=$i
    sleep 1
done

# Colormap cycle test
for i in {0..11}; do
    echo "Testing colormap=$i"
    v4l2-ctl -d /dev/v4l-subdev2 -c colormap=$i
    sleep 2
done

# Zoom test
for i in {1..8}; do
    echo "Testing zoom=$i"
    v4l2-ctl -d /dev/v4l-subdev2 -c zoom_absolute=$i
    sleep 1
done

# FFC test
echo "Triggering FFC (expect 1-2 second delay)"
v4l2-ctl -d /dev/v4l-subdev2 -c ffc_trigger=1
```

### Format Testing

```bash
# Test all supported modes (requires driver reload for mode change)
for mode in 0 1 2; do
    echo "Testing mode $mode"
    sudo modprobe -r rs300
    sudo modprobe rs300 mode=$mode
    ./configure_media.sh
    v4l2-ctl -d /dev/video0 --stream-count=1 --stream-to=mode${mode}.yuv
done
```

### I2C Protocol Testing

```bash
# Monitor all I2C traffic (requires i2c-tools and i2c-dev)
sudo i2cdump -y 10 0x3c

# Watch status register during capture
watch -n0.1 'i2cget -y 10 0x3c 0x02 b 0x00'
```

### Performance Testing

```bash
# Measure sustained frame rate
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=300 2>&1 | grep fps

# Expected results:
# - 640x512@60fps: ~60 fps
# - 256x192@25fps: ~25 fps
# - 384x288@30fps: ~30 fps
```

---

## Quick Reference Cards

### Media Pipeline Setup (One Command)
```bash
# Complete pipeline configuration
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]" && \
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]" && \
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]" && \
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]" && \
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

### Full Driver Reset
```bash
# Complete reset sequence
sudo rmmod rs300 && \
sudo dmesg -C && \
sudo modprobe rs300 mode=0 fps=60 debug=1 && \
./configure_media.sh && \
dmesg | grep rs300
```

### Capture & Convert Pipeline
```bash
# Capture 1 frame and convert to PNG
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=thermal.yuv && \
ffmpeg -y -f rawvideo -pix_fmt uyvy422 -s 640x512 -i thermal.yuv thermal.png && \
echo "Saved to thermal.png"
```

---

## Additional Resources

- **Full Analysis**: See `DRIVER_ANALYSIS.md` for comprehensive technical documentation
- **Boot Configuration**: See `BOOT_CONFIGURATION.md` for auto-start setup
- **Media Pipeline**: See `RS300_Media_Pipeline_Guide.md` for detailed pipeline info
- **Device Tree**: See `rs300-overlay.dts` for hardware configuration
- **Main Source**: See `rs300.c` for driver implementation

---

**Document Version**: 1.0
**Last Updated**: 2025-10-21
**Driver Version**: 0.01.01
