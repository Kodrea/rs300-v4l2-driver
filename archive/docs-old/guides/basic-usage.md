# Basic Usage Guide

Complete guide to streaming thermal video and using the RS300 camera for common tasks.

## Quick Reference

| Task | Command | Time |
|------|---------|------|
| Configure pipeline (Pi 5) | `./configure_media.sh` | 30s |
| Live view | `ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0` | - |
| Stream test | `v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10` | 5s |
| List controls | `v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls` (Pi 5) | 1s |
| FFC calibration | `v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=ffc_trigger=0` | 1s |

---

## Platform-Specific Setup

### Raspberry Pi 5 Setup

Pi 5 uses the RP1-CFE camera system with media controller pipeline.

#### Initial Configuration (After Each Reboot)

**IMPORTANT**: Media pipeline configuration does not persist across reboots.

**Automated configuration** (recommended):
```bash
cd ~/rs300-v4l2-driver
./configure_media.sh
```

**What it does**:
- Auto-detects RS300 camera
- Prompts for format selection (UYVY or YUYV)
- Configures media controller links
- Sets up complete pipeline
- Tests streaming

**Make it permanent**: See [BOOT_CONFIGURATION.md](../reference/BOOT_CONFIGURATION.md) for auto-start options.

#### Manual Configuration (Advanced)

For users who want full control:

```bash
# Link CSI receiver to video node
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set format on all pads (must match exactly)
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"

# Set video node format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

**Important**: Pi 5 RP1-CFE only supports 16-bit packed formats:
- ✅ `UYVY8_1X16` (recommended)
- ✅ `YUYV8_1X16`
- ❌ `*8_2X8` formats (will cause "Format mismatch!" error)

#### Verify Configuration

```bash
# Check media topology
media-ctl -p

# Look for:
# - rs300 10-003c entity
# - ENABLED links
# - Matching formats (UYVY8_1X16 or YUYV8_1X16)

# Check video device
v4l2-ctl -d /dev/video0 --all
```

### Raspberry Pi 4 Setup

Pi 4 uses legacy Unicam driver - no media controller configuration needed.

#### Set Video Format

**IMPORTANT**: Unicam defaults to 640×480. Set format to match your module.

**640×512 module**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

**384×288 module**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=384,height=288,pixelformat=YUYV
```

**256×192 module**:
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=256,height=192,pixelformat=YUYV
```

**Note**: Format must match the `mode` value set in `rs300.c` before building.

---

## Streaming Methods

### Method 1: v4l2-ctl (Quick Test)

**Basic stream test**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10
```

**What it shows**:
- Frame capture rate (should be ~60fps)
- Buffer management
- Any errors

**Capture to file**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 --stream-to=thermal.raw
```

**No display** - for testing only.

### Method 2: ffplay (Live Viewing)

**Best for**: Quick live viewing, simple playback

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

**Controls**:
- `Q` or `ESC`: Quit
- `F`: Toggle fullscreen
- `P` or `SPACE`: Pause

**Options**:
```bash
# Specify window size
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0 -window_size 1280x1024

# Reduce latency
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 -fflags nobuffer /dev/video0
```

### Method 3: GStreamer (Best Performance)

**Best for**: Low latency, recording, processing pipelines

**Installation**:
```bash
sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad
```

#### Live Viewing with FPS Counter

**640×512 @ 60fps**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! \
  videoconvert ! \
  fpsdisplaysink video-sink=autovideosink text-overlay=true
```

**384×288 @ 60fps**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=384,height=288,framerate=60/1 ! \
  videoconvert ! \
  fpsdisplaysink video-sink=autovideosink text-overlay=true
```

**256×192 @ 50fps**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=256,height=192,framerate=50/1 ! \
  videoconvert ! \
  fpsdisplaysink video-sink=autovideosink text-overlay=true
```

**Remove FPS counter**: Change `text-overlay=true` to `text-overlay=false`

#### Simple Live View (No FPS)

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512 ! \
  videoconvert ! \
  autovideosink
```

#### Record to File (H.264)

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! \
  videoconvert ! \
  x264enc speed-preset=ultrafast tune=zerolatency ! \
  h264parse ! \
  mp4mux ! \
  filesink location=thermal_recording.mp4
```

**Stop recording**: Press `Ctrl+C`

**Play recording**:
```bash
ffplay thermal_recording.mp4
```

#### Record + Live View (tee element)

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! \
  tee name=t ! \
  queue ! videoconvert ! autovideosink \
  t. ! queue ! videoconvert ! \
  x264enc speed-preset=ultrafast ! h264parse ! mp4mux ! \
  filesink location=thermal.mp4
```

---

## Video Recording

### Method 1: Raw Capture (Simple)

**Capture raw YUV frames**:
```bash
# Capture 300 frames (~5 seconds @ 60fps)
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=300 --stream-to=thermal.yuv
```

**Convert to MP4**:
```bash
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x512 -framerate 60 \
  -i thermal.yuv -c:v libx264 -preset fast thermal.mp4
```

### Method 2: Direct Encoding (ffmpeg)

**Record with ffmpeg**:
```bash
# 10 second recording
ffmpeg -f v4l2 -video_size 640x512 -pixel_format yuyv422 -framerate 60 -t 10 \
  -i /dev/video0 -c:v libx264 -preset fast thermal.mp4
```

**Options**:
- `-t 10`: Duration in seconds
- `-preset fast`: Encoding speed (ultrafast, fast, medium, slow)
- `-crf 18`: Quality (lower = better, 18 = visually lossless)

### Method 3: GStreamer (Recommended)

See GStreamer section above for recording pipelines.

---

## Image Capture

### Single Frame Capture

**Capture raw frame**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=frame.raw
```

**Convert to PNG**:
```bash
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x512 \
  -i frame.raw frame.png
```

**Convert to JPEG**:
```bash
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x512 \
  -i frame.raw -q:v 2 frame.jpg
```

### Timelapse Capture

**Script to capture frames every N seconds**:

```bash
#!/bin/bash
# timelapse.sh - Capture thermal timelapse

INTERVAL=5  # Seconds between captures
COUNT=100   # Number of frames

for i in $(seq 1 $COUNT); do
    echo "Capturing frame $i/$COUNT"
    v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1 --stream-to=frame_$(printf "%04d" $i).raw
    sleep $INTERVAL
done

echo "Converting to images..."
for raw in frame_*.raw; do
    png="${raw%.raw}.png"
    ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 640x512 -i "$raw" "$png" -y
    rm "$raw"
done

echo "Creating timelapse video..."
ffmpeg -framerate 30 -pattern_type glob -i 'frame_*.png' -c:v libx264 -preset fast timelapse.mp4
```

**Usage**:
```bash
chmod +x timelapse.sh
./timelapse.sh
```

---

## Streaming Over Network

### Method 1: VLC Network Stream

**On Raspberry Pi** (sender):
```bash
# Install VLC
sudo apt install vlc

# Stream via HTTP
cvlc v4l2:///dev/video0 :v4l2-width=640 :v4l2-height=512 \
  --sout '#transcode{vcodec=h264,vb=800,fps=30}:http{mux=ffmpeg{mux=flv},dst=:8080/stream}' \
  --no-sout-audio
```

**On viewing computer**:
- Open VLC
- Media → Open Network Stream
- Enter: `http://[raspberry-pi-ip]:8080/stream`

### Method 2: GStreamer UDP Stream

**On Raspberry Pi** (sender):
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=30/1 ! \
  videoconvert ! \
  x264enc tune=zerolatency bitrate=2000 ! \
  rtph264pay ! \
  udpsink host=[destination-ip] port=5000
```

**On viewing computer** (receiver):
```bash
gst-launch-1.0 udpsrc port=5000 ! \
  application/x-rtp,encoding-name=H264,payload=96 ! \
  rtph264depay ! \
  avdec_h264 ! \
  videoconvert ! \
  autovideosink
```

### Method 3: RTSP Server

**Install rtsp-simple-server**:
```bash
wget https://github.com/aler9/rtsp-simple-server/releases/download/v0.21.5/rtsp-simple-server_v0.21.5_linux_arm64v8.tar.gz
tar -xzf rtsp-simple-server_v0.21.5_linux_arm64v8.tar.gz
```

**Configure and run**:
```bash
# Start RTSP server
./rtsp-simple-server &

# Stream to RTSP server
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=30/1 ! \
  videoconvert ! \
  x264enc tune=zerolatency ! \
  rtspclientsink location=rtsp://localhost:8554/thermal
```

**View from any device**:
```
rtsp://[raspberry-pi-ip]:8554/thermal
```

---

## Processing and Analysis

### OpenCV Integration (Python)

**Install requirements**:
```bash
sudo apt install python3-opencv python3-numpy
```

**Basic capture script**:
```python
#!/usr/bin/env python3
import cv2
import numpy as np

# Open video device
cap = cv2.VideoCapture('/dev/video0')

# Set resolution
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 512)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Display frame
    cv2.imshow('Thermal Camera', frame)

    # Press 'q' to quit
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
```

**Save to file**:
```bash
chmod +x thermal_capture.py
./thermal_capture.py
```

---

## Performance Optimization

### Reduce Latency

**ffplay low-latency**:
```bash
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 \
  -fflags nobuffer -flags low_delay -framedrop /dev/video0
```

**GStreamer low-latency**:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512 ! \
  queue max-size-buffers=2 leaky=downstream ! \
  videoconvert ! \
  autovideosink sync=false
```

### Maximize Frame Rate

**Check current FPS**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
```

**Tips**:
1. Close unnecessary applications
2. Ensure adequate power supply
3. Check thermal throttling: `vcgencmd get_throttled`
4. Use GStreamer instead of ffplay
5. Disable FPS overlay if not needed

---

## Common Use Cases

### Security/Surveillance

```bash
# Continuous recording with rotation (1 hour files)
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=30/1 ! \
  videoconvert ! \
  x264enc ! h264parse ! \
  splitmuxsink location=thermal_%02d.mp4 max-size-time=3600000000000
```

### Equipment Inspection

```bash
# High quality recording for detailed analysis
ffmpeg -f v4l2 -video_size 640x512 -pixel_format yuyv422 -framerate 60 \
  -i /dev/video0 -c:v libx264 -preset slow -crf 15 inspection.mp4
```

### Real-time Monitoring

```bash
# Live view with controls overlay
gst-launch-1.0 v4l2src device=/dev/video0 ! \
  video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! \
  videoconvert ! \
  fpsdisplaysink video-sink=autovideosink text-overlay=true
```

---

## Troubleshooting

### No Video Stream

**Check device exists**:
```bash
ls -l /dev/video*
v4l2-ctl --list-devices
```

**Pi 5: Verify pipeline configured**:
```bash
./configure_media.sh
media-ctl -p | grep ENABLED
```

**Pi 4: Verify format set**:
```bash
v4l2-ctl -d /dev/video0 --get-fmt-video
```

### Low Frame Rate

**Measure actual FPS**:
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
```

**Solutions**:
1. Check power supply
2. Close other programs
3. Reduce resolution (if using 640×512)
4. Check thermal throttling

### "Format mismatch!" (Pi 5)

**Cause**: Using 8-bit dual lane format on RP1-CFE

**Solution**: Use 16-bit packed formats only
```bash
./configure_media.sh  # Choose UYVY or YUYV
```

---

## Next Steps

- **[Camera Controls →](camera-controls.md)** - Adjust image settings
- **[Advanced Topics →](../../)** - ISP integration, boot automation
- **[Troubleshooting →](../reference/TROUBLESHOOTING.md)** - Detailed debugging

---

**See Also**:
- [DEV_QUICK_REFERENCE.md](../reference/DEV_QUICK_REFERENCE.md) - Command cheat sheet
- [RASPBERRY_PI_ISP_GUIDE.md](../reference/RASPBERRY_PI_ISP_GUIDE.md) - Hardware ISP processing
- [RS300_Media_Pipeline_Guide.md](../reference/RS300_Media_Pipeline_Guide.md) - Media controller details
