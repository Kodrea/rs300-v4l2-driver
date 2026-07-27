# First Thermal Capture

Quick start guide to capture your first thermal image and explore basic camera controls.

## Prerequisites

**Before starting**, ensure you've completed installation:
- ✅ [Installation Guide](installation.md) - for Pi 4 or Pi 5

**Verification**:
```bash
# Driver should be loaded
lsmod | grep rs300

# Camera should be detected
v4l2-ctl --list-devices
```

## Quick Start (5 Minutes)

### Pi 5: Configure Media Pipeline

**IMPORTANT**: Pi 5 requires media pipeline configuration after every reboot.

```bash
cd ~/rs300-v4l2-driver
./configure_media.sh
```

**Follow prompts**:
1. Choose the module resolution (0=640x512, 1=256x192, 2=384x288)
2. Script will test streaming automatically

The pixel format is not a prompt. The script always uses YUYV8_1X16, the only
YUV bus format RP1-CFE accepts.

**Pi 4**: Skip this step (no media controller required)

### Your First Thermal Video Stream

**Live viewing with ffplay**:

**Pi 5 & Pi 4 (640×512)**:
```bash
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0
```

**Pi 4 (384×288)**:
```bash
ffplay -f v4l2 -video_size 384x288 -pixel_format yuyv422 /dev/video0
```

**Pi 4 (256×192)**:
```bash
ffplay -f v4l2 -video_size 256x192 -pixel_format yuyv422 /dev/video0
```

**Success!** You should see live thermal video.

**Controls**:
- Press `ESC` or `Q` to quit
- Window can be resized

### Capture Still Frame

**Capture raw frame**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=thermal_frame.raw
```

**Convert to viewable format** (requires ffmpeg):
```bash
# Install ffmpeg if needed
sudo apt install ffmpeg

# Convert raw YUYV to PNG
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x512 -i thermal_frame.raw thermal_image.png
```

**View the image**:
```bash
# On Raspberry Pi desktop
xdg-open thermal_image.png

# Or copy to your computer via SCP
```

## Exploring Camera Controls

The RS300 exposes 26 V4L2 controls. Most can be adjusted while streaming. A few,
including `pixel_rate` and `link_freq`, are read-only.

### Identify Your Subdevice

**Pi 5**:
```bash
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
```

**Pi 4**:
```bash
v4l2-ctl -d /dev/v4l-subdev0 --list-ctrls
```

**Output** shows all available controls:
```
brightness 0x00980900 (int)    : min=0 max=100 step=1 default=50 value=50
contrast 0x00980901 (int)      : min=0 max=100 step=1 default=50 value=50
zoom_absolute 0x009a090d (int) : min=1 max=8 step=1 default=1 value=1
...
```

### Essential Controls to Try

For convenience, set your subdevice:

**Pi 5**:
```bash
SUBDEV=/dev/v4l-subdev2
```

**Pi 4**:
```bash
SUBDEV=/dev/v4l-subdev0
```

#### 1. Flat Field Calibration (FFC)

**Purpose**: Corrects sensor non-uniformities, removes "ghosting" artifacts

**When to use**:
- After significant temperature changes
- If image shows persistent artifacts
- Every 5-10 minutes for best quality

**Trigger calibration**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
```

**What happens**:
- You'll hear an audible "click" (shutter closing/opening)
- Video freezes briefly (~1 second)
- Image quality improves

**Tip**: Start every session with FFC calibration.

#### 2. Change Colormap (Palette)

**Purpose**: Different color schemes help visualize temperature differences

**Try these popular colormaps**:

```bash
# White Hot (default) - hot objects are white
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=0

# Black Hot - hot objects are black (inverted)
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=1

# Iron Red - classic thermal camera look
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=2

# Hot Iron - enhanced thermal look
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=3

# Rainbow 1 - high color contrast
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=6
```

**All colormaps** (0-11):
| Value | Name | Description |
|-------|------|-------------|
| 0 | White Hot | Hot = white, cold = black (default) |
| 1 | Black Hot | Hot = black, cold = white (inverted) |
| 2 | Iron Red | Red/orange thermal look |
| 3 | Hot Iron | Enhanced thermal colors |
| 4 | Medical | Medical imaging palette |
| 5 | Arctic | Blue-tinted cold emphasis |
| 6 | Rainbow 1 | High contrast rainbow |
| 7 | Rainbow 2 | Alternative rainbow |
| 8 | Tint | Subtle color tinting |
| 9 | Black & White | Grayscale |

**Experiment**: Try different colormaps while viewing to find your preference.

#### 3. Adjust Brightness

**Purpose**: Adjust overall image brightness/gain

```bash
# Increase brightness (more sensitive, shows subtle differences)
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=75

# Decrease brightness (less sensitive, emphasize hot spots)
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=25

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=50
```

**Range**: 0-100, default 50

**Tip**: Adjust brightness based on your scene's temperature range.

#### 4. Adjust Contrast

**Purpose**: Enhance temperature differences

```bash
# Increase contrast (sharper temperature boundaries)
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=75

# Decrease contrast (softer gradients)
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=25

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=50
```

**Range**: 0-100, default 50

#### 5. Digital Zoom

**Purpose**: Crop and magnify center of image

```bash
# 2x zoom
v4l2-ctl -d $SUBDEV --set-ctrl=zoom_absolute=2

# 4x zoom
v4l2-ctl -d $SUBDEV --set-ctrl=zoom_absolute=4

# Maximum 8x zoom
v4l2-ctl -d $SUBDEV --set-ctrl=zoom_absolute=8

# Reset (no zoom)
v4l2-ctl -d $SUBDEV --set-ctrl=zoom_absolute=1
```

**Range**: 1-8x, default 1 (no zoom)

**Note**: This is digital zoom (cropping), not optical zoom.

#### 6. Scene Modes

**Purpose**: Pre-configured settings optimized for different scenarios

```bash
# General purpose (default)
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=3

# High contrast scenes
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=5

# Outdoor
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=2

# Indoor
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=3
```

**All scene modes** (0-9):
| Value | Name | Best For |
|-------|------|----------|
| 0 | Manual | Custom adjustments |
| 1 | Default | General purpose |
| 2 | Outdoor | Outdoor imaging |
| 3 | Indoor | Indoor imaging (default) |
| 4 | AGC | Automatic gain control |
| 5 | High Contrast | Scenes with large temp differences |
| 6 | Low Contrast | Subtle temperature variations |
| 7 | High Brightness | Bright environments |
| 8 | Low Brightness | Dark environments |
| 9 | Custom | User-defined preset |

## Practical Examples

### Example 1: Find Hot Spots in a Room

```bash
# Use high contrast mode and hot iron colormap
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=3
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=5
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=60

# View live
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0
```

### Example 2: Detailed Inspection (with zoom)

```bash
# FFC calibration
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0

# 4x zoom for detail
v4l2-ctl -d $SUBDEV --set-ctrl=zoom_absolute=4

# High contrast
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=80

# View live
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0
```

### Example 3: Outdoor Surveillance

```bash
# FFC calibration
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0

# Outdoor scene mode
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=2

# Black hot (better for outdoor)
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=1

# View live
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0
```

## Recording Thermal Video

### Record with GStreamer

**640×512 @ 60fps** (H.264 encoding):
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! \
  videoconvert ! \
  x264enc speed-preset=ultrafast tune=zerolatency ! \
  h264parse ! \
  mp4mux ! \
  filesink location=thermal_video.mp4
```

**Stop recording**: Press `Ctrl+C`

**Play recorded video**:
```bash
ffplay thermal_video.mp4
```

### Record Raw Stream

**Capture 300 frames** (~5 seconds @ 60fps):
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=300 --stream-to=thermal_raw.yuv
```

**Convert to video**:
```bash
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x512 -framerate 60 \
  -i thermal_raw.yuv -c:v libx264 -preset fast thermal_converted.mp4
```

## Automated Testing

**Test all controls** with the included test script:

```bash
# List every control the driver exposes
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls

# Read one control
v4l2-ctl -d /dev/v4l-subdev2 --get-ctrl brightness

# Write one control
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl brightness=60
```

**What it tests**:
- All 26 V4L2 controls
- Valid ranges
- Error handling
- Success/failure reporting

## Troubleshooting First Capture

### "No such file or directory" (/dev/video0)

**Solution**:
```bash
# Check if driver loaded
lsmod | grep rs300

# If not loaded, reload driver
cd ~/rs300-v4l2-driver
sudo ./install.sh
sudo reboot
```

### Pi 5: "Format mismatch" Error

**Solution**: Run media pipeline configuration:
```bash
cd ~/rs300-v4l2-driver
./configure_media.sh
```

### No Video / Black Screen

**Likely causes**:
1. FPC cable not fully seated
2. Wrong resolution specified
3. Format not set (Pi 4)

**Solution (Pi 4)**: Set format explicitly:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

### Low Frame Rate

**Check actual FPS**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
```

**If low**:
1. Check power supply (adequate wattage)
2. Check thermal throttling: `vcgencmd get_throttled`
3. Close other programs

### Control Commands Not Working

**Verify subdevice path**:
- **Pi 5**: Use `/dev/v4l-subdev2`
- **Pi 4**: Use `/dev/v4l-subdev0`

**List all subdevices**:
```bash
v4l2-ctl --list-devices
```

## Next Steps

Now that you've captured your first thermal image:

1. **[Advanced Usage →](../../README.md#advanced-usage)** - ISP integration, boot automation
2. **[Troubleshooting →](../reference/SETUP_AND_TROUBLESHOOTING.md)** - Detailed debugging guide
3. **[Pipeline Guide →](../reference/RS300_Media_Pipeline_Guide.md)** - Media controller topology

## Learning Resources

- 📺 **[Video Tutorials](https://linktr.ee/kodrea)** - Visual walkthroughs
- 📖 **[DEV_QUICK_REFERENCE.md](../reference/DEV_QUICK_REFERENCE.md)** - Command cheat sheet
- 📖 **[DRIVER_ANALYSIS.md](../reference/DRIVER_ANALYSIS.md)** - Deep technical dive
- 🐛 **[GitHub Issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)** - Community support

---

**Congratulations!** You've successfully captured your first thermal image. Experiment with different controls and scenes to get the most out of your RS300 camera.
