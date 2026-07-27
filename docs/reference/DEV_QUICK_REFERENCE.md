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
# Do not unload the module on Pi 5: it crashes rp1_cfe.
# Change parameters through modprobe.d and reboot instead:
#   echo "options rs300 mode=2 fps=60" | sudo tee /etc/modprobe.d/rs300.conf
#   sudo reboot
```

---

## Code Locations

Function names only. Line numbers are deliberately left out because they go
stale on every edit. To find one:

```bash
grep -n 'rs300_set_zoom' rs300.c
```

| Feature | Function |
|-|-|
| Driver init | `rs300_probe()` |
| Stream control | `rs300_set_stream()` |
| Format setup | `rs300_set_pad_fmt()` |
| Controls | `rs300_set_ctrl()` |
| I2C read | `read_regs()` |
| I2C write | `write_regs()` |
| CRC | `do_crc()` |

### Camera Commands

| Command | Function |
|-|-|
| Brightness GET | `rs300_get_brightness()` |
| Brightness SET | `rs300_brightness_correct()` |
| Colormap GET | `rs300_get_colormap()` |
| Colormap SET | `rs300_set_colormap()` |
| FFC | `rs300_shutter_cal()` |
| Zoom | `rs300_set_zoom()` |
| Scene Mode | `rs300_set_scene_mode()` |
| Contrast | `rs300_set_contrast()` |
| DDE | `rs300_set_dde()` |
| Spatial NR | `rs300_set_spatial_nr()` |
| Temporal NR | `rs300_set_temporal_nr()` |
| YUV Format | `rs300_set_yuv_format()` |
| FPS | `rs300_set_fps()` |

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
sudo dmesg -C
echo "options rs300 mode=2 fps=60" | sudo tee /etc/modprobe.d/rs300.conf
sudo reboot
# after the reboot
sudo rs300-configure
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
