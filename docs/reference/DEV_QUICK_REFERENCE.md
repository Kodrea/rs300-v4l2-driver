# Developer Quick Reference

---

## Quick Start

```bash
sudo ./install.sh
sudo reboot

./configure_media.sh
lsmod | grep rs300
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=90 --stream-to=test.yuv
# Extract frame 60+ (after 2s warm-up): dd if=test.yuv of=frame.yuv bs=655360 count=1 skip=59
```

---

## Essential Commands

### Device Check
```bash
v4l2-ctl --list-devices              # All V4L2 devices
v4l2-ctl -d /dev/video0 --info       # Driver info
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls-menus  # All controls
media-ctl -p                          # Media topology
i2cdetect -y 10                       # I2C device at 0x3c
```

### Format
```bash
v4l2-ctl -d /dev/video0 --get-fmt-video
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY
v4l2-ctl -d /dev/video0 --list-formats-ext
```

### Controls
```bash
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=50        # 0-100
v4l2-ctl -d /dev/v4l-subdev2 -c colormap=3           # 0-11
v4l2-ctl -d /dev/v4l-subdev2 -c zoom_absolute=2      # 1-8
v4l2-ctl -d /dev/v4l-subdev2 -c ffc_trigger=1        # FFC
v4l2-ctl -d /dev/v4l-subdev2 -c scene_mode=3         # 0-9
```

---

## V4L2 Controls Reference

| Control | Range | Default | Notes |
|---------|-------|---------|-------|
| `brightness` | 0-100 | 50 | Thermal brightness |
| `contrast` | 0-100 | 50 | Image contrast |
| `zoom_absolute` | 1-8 | 1 | Digital zoom |
| `colormap` | 0-11 | 0 | Color palette |
| `ffc_trigger` | - | - | Button, triggers FFC |
| `scene_mode` | 0-9 | 3 | Scene optimization |
| `digital_detail_enhancement` | 0-100 | 50 | Edge enhancement |
| `spatial_noise_reduction` | 0-100 | 50 | Spatial noise |
| `temporal_noise_reduction` | 0-100 | 50 | Temporal noise |
| `pixel_rate` | read-only | varies | Dynamic |
| `link_freq` | read-only | 80MHz | MIPI CSI-2 |

### Colormap (0-11)
```
0=White Hot  1=Reserved  2=Sepia  3=Ironbow  4=Rainbow
5=Night  6=Aurora  7=Red Hot  8=Jungle  9=Medical
10=Black Hot  11=Golden Red
```

### Scene Mode (0-9)
```
0=Low  1=Linear  2=Low Contrast  3=General  4=High Contrast
5=Highlight  6-8=Reserved  9=Outline
```

---

## Kernel Logs

```bash
dmesg -wH | grep rs300                # Watch in real-time
dmesg | grep rs300 | tail -50         # Recent messages
dmesg -C && dmesg -wH                 # Clear and watch
dmesg | grep -i "rs300.*error"        # Errors only
```

### Media Pipeline Check
```bash
media-ctl -p | grep "\[ENABLED\]"
# Should show: 'csi2':4 -> 'rp1-cfe-csi2_ch0':0 [ENABLED]

media-ctl -p | grep "fmt:UYVY8_1X16"   # Verify format
```

### I2C Check
```bash
i2cdetect -y 10                       # Device at 0x3c
i2cget -y 10 0x3c 0x02 b             # Read status (0x00=idle, 0x01=busy, 0x02=failed)
```

### Driver Module
```bash
lsmod | grep rs300                    # Loaded?
cat /sys/module/rs300/parameters/mode # Current mode
modinfo rs300                         # Module details
sudo rmmod rs300                      # Unload
sudo modprobe rs300 mode=0 fps=60     # Load with params
```

---

## Code Locations

| Feature | Function | Location |
|---------|----------|----------|
| Driver init | `rs300_probe()` | rs300.c:2759 |
| Stream control | `rs300_set_stream()` | rs300.c:2097 |
| Format setup | `rs300_set_pad_fmt()` | rs300.c:1850 |
| Controls | `rs300_set_ctrl()` | rs300.c:1626 |
| I2C read | `read_regs()` | rs300.c:205 |
| I2C write | `write_regs()` | rs300.c:233 |
| CRC | `do_crc()` | rs300.c:152 |

### Camera Commands
| Command | Line | Function |
|---------|------|----------|
| Brightness GET | 502 | `rs300_get_brightness()` |
| Brightness SET | 1327 | `rs300_brightness_correct()` |
| Colormap GET | 991 | `rs300_get_colormap()` |
| Colormap SET | 1081 | `rs300_set_colormap()` |
| FFC | 1215 | `rs300_shutter_cal()` |
| Zoom | 1458 | `rs300_set_zoom()` |
| Scene Mode | 1541 | `rs300_set_scene_mode()` |
| Contrast | 778 | `rs300_set_contrast()` |
| DDE | 635 | `rs300_set_dde()` |
| Spatial NR | 849 | `rs300_set_spatial_nr()` |
| Temporal NR | 920 | `rs300_set_temporal_nr()` |
| YUV Format | 706 | `rs300_set_yuv_format()` |
| FPS | 1997 | `rs300_set_fps()` |

---

## Testing

### Brightness Range
```bash
for i in 0 25 50 75 100; do
  v4l2-ctl -d /dev/v4l-subdev2 -c brightness=$i
  sleep 1
done
```

### Colormap Cycle
```bash
for i in {0..11}; do
  v4l2-ctl -d /dev/v4l-subdev2 -c colormap=$i
  sleep 2
done
```

### Frame Rate Test
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 2>&1 | grep fps
# Expected: 60 fps (640x512 mode 0), 30 fps (384x288 mode 2), etc.
```

---

## Full Reset Sequence

```bash
sudo rmmod rs300
sudo dmesg -C
sudo modprobe rs300 mode=0 fps=60 debug=1
./configure_media.sh
dmesg | grep rs300
```

---

## Capture & Convert

```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=thermal.yuv
ffmpeg -y -f rawvideo -pix_fmt uyvy422 -s 640x512 -i thermal.yuv thermal.png
```

---

## References

- **DRIVER_ANALYSIS.md** - Technical deep-dive
- **SETUP_AND_TROUBLESHOOTING.md** - Setup & problem diagnosis
- **RS300_Media_Pipeline_Guide.md** - Pipeline architecture
