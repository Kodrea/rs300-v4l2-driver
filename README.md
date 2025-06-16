# RS300 Thermal Camera Driver for Raspberry Pi
- Credit to [will127534](https://github.com/will127534) for the setup script in his imx294 driver
- Used the [IMX219.c](https://github.com/raspberrypi/linux/blob/rpi-6.6.y/drivers/media/i2c/imx219.c) as a template for the driver on RPi.

This driver is for the "mini2" thermal camera module refered to as the rs300 in the original driver.

Tested with a 640x512 module on Raspberry Pi 4b running Raspberry OS - Bookworm

---
**WORK IN PROGRESS**

---

## UPDATE 5/2/2025
After a delay with the boards they are now available for purchase with the module! Currently the only listing for the Mini2 is the 640 module with 9mm lens but if you need a different lens or resolutions feel free to reach out to their customer support. 

Below is the link to buy, make sure to use the coupon code which will ensure your order has the RPi adapter board and takes 30USD off.
</br>⚠️IMPORTANT: use the coupon code to get the custom RPi board and not their standard board!
- Link to Buy: https://www.thermal-image.com/product/mini2-640x512-9mm-thermal-imaging-camera-module-for-drones/
- Coupon Code: HXZUK8WG

I'm working on cleaning up the driver as well as creating tutorial videos in the coming days
- https://linktr.ee/kodrea

## UPDATE!! 3/27/25
Approximately two weeks until the MIPI CSI-2 boards for raspberry pi will be available from Purple River Tech.

---

## Raspberry Pi MIPI CSI-2 Testing

| Raspberry Pi Model | Module Resolution | Connection Type | OS & Kernel | Status         | Notes                                                          |
| ------------------ | ----------------- | --------------- | ----------- | -------------- | -------------------------------------------------------------- |
| Pi 4B              | 640x512           | MIPI CSI-2      | Bookworm    | Working        | 60Hz video. Low-voltage warning; high current draw on 3.3V CSI port. Rarely causes issues |
| Pi 4B              | 384x288           | MIPI CSI-2      | Bookworm    | Working        | 60Hz video                           |
| Pi 4B              | 256x192           | MIPI CSI-2      | Bookworm    | *Working       | *Purple River tested the 256 with my driver and it worked. They believe my module's firmware is the issue and are sending me instructions to update it|
| Pi 5               | 640x512           | MIPI CSI-2      | Bookworm    | ✅ **WORKING** | 60fps thermal streaming with media controller pipeline |
| Pi Zero 2 W        | 640x512           | MIPI CSI-2      | Bookworm    | ⚠️ Brownouts   | Camera startup draws too much current, maybe possible in a later board revision|

- For the 256 module I'm pretty stumped, I've spent endless hours troubleshooting the mipi video. I2C commands work and the camera appears to be operating normally (shutter click startup sequence is audible and CVBS video stream works) but no matter what I do I get no video when opening the camera and no data if i try --streammap. I will continue troubleshooting but this will be on the back burner since the 50hz is available via USB.
- For the Pi Zero 2W I tried powering the 5V rail directly with a DC PSU but still faced brownouts. The camera causes a voltage drop large enough that the raspberry pi reboots every time. Unfortunately attempts to power the module externally have not worked either. it's not possible to power the module by the 5V and GND connectors exposed and connect the mipi CSI-2 cable. In a later board revision I would like to have 5V pins dedicated for powering even when the module is connected by the 15pin ribbon cable.

## TODO
- Test 384 and 256 modules on Pi 5
- Continue Troubleshooting 256 mipi data on Pi 4
- ISP integration for hardware denoising (Pi 5)


## Where I got the module
I've bought from two stores on Alibaba who sell the same module
1. Purple River Technology:
   - **Discount code that also orders the custom PCB for Raspberry Pi connection coming soon. Will be used on their website https://www.thermal-image.com/product-category/thermalmodule/**
   - I bought a 256 module with 15mm lens from them and they helped me with the custom PCB board for the module that allows it to connect to the Pi. Excellent Customer support.
   - [Purple River Alibaba Store](https://purpleriver.en.alibaba.com/index.html?spm=a2700.details.0.0.1a245460PSIafr&from=detail&productId=1601081970203)
3. Shenzhen Chengen Thermovisiontechnology: [Chengen Alibaba Store](https://cersnv.en.alibaba.com/index.html?spm=a2700.details.0.0.7d0f2dfbCpGJwE&from=detail&productId=1601308525861)
   - I bought the 640 module with 25mm lens from this seller and put the PCB board from purple river on it to connect it to the pi.

## Some Basic Specs
The "mini2" is a thermal imaging camera that comes in multiple resolutions and to my understanding is imaging only. For temperature measurements a different module or variant is needed, like the "mini" (I wish they used a better naming scheme).

The available resolutions are:
1. 256x192 - 25/50hz   (25/50hz is available over USB)
2. 384x288 - 30/60hz   (30/60Hz is available over USB)
3. 640x512 - 30/60hz   (60hz is NOT supported by USB)

NETD: 40mk
------
### Video Output Methods
This module can output two simultaneous video streams. One Digital, One Analog.

For the custom PCB I have there is USB 2.0, MIPI, and CVBS. The camera also support DVP and BT656 as digital formats but they are not broken out on this board and I haven't researched them further.

#### USB 2.0
- As mentioned above the 640 module is limited to 30fps over USB. I'm unsure about the 384 module.
- The camera is UVC compliant and will work the same as a webcam but camera controls are not exposed. For the 256 module you can even switch the framerate from 25fps to 50fps in standard camera apps.
- The pixel format is YUYV422 but you can change it to a different order like UYVU with the SDK.
- For sending commands to the camera over USB, such as brightness changes, shutter calibration, changing colormaps, you will need to use the provided SDK. The SDK is for windows and linux but I have only used the linux portion so far

#### MIPI
- Allows digital 60fps with the 640 module
- The custom PCB has the 15 pin connector compatible with RPi 4B MIPI CSI-2 camera port. The original board needed a same-sided ribbon cable so I had them update the design to be compatible with the reverse head cable that raspberry pi cameras generally use. This is critical to be able to use the 15-22pin adapter cable for the RPi 5, zero, and CMs
- Uses the rs300 driver I am currently working on to connect to the Pi. I'm implementing camera commands over I2C without the need for any SDK or precompiled binary files. As many commands as I can I am making them V4L2 compliant for easy integration.

#### CVBS
- Analog Output can be set to NTSC or PAL
- Can be output simultaneously with USB or MIPI
- Only White Hot and Black Hot colormaps are displayed. So far all other parameters seem to equally affect the analog video stream like; brighntess, contrast, noise reduction, digital detail enhancement (DDE), and digital zoom.


## Installation

### Prerequisites
- Raspberry Pi with Raspbian/Raspberry Pi OS Bookworm (Bullseye not tested)
- Linux headers installed
- DKMS support

### Raspberry Pi 5 - Quick Setup (Recommended)

**Pi 5 uses the new RP1-CFE camera system with media controller pipeline configuration.**

Install dependencies:
```bash
sudo apt install linux-headers dkms git v4l-utils
```

Clone and build:
```bash
git clone https://github.com/your-repo/rs300-v4l2-driver.git
cd rs300-v4l2-driver
chmod +x setup.sh
./setup.sh
```

Add to config.txt:
```bash
sudo nano /boot/firmware/config.txt
```
Add these lines:
```bash
camera_auto_detect=0
dtoverlay=rs300
```

Reboot and configure:
```bash
sudo reboot
./configure_media.sh
```

The configuration script will:
- Auto-detect your RS300 camera
- Let you choose UYVY or YUYV format
- Configure the media controller pipeline
- Test streaming automatically

**That's it! Your thermal camera should now be streaming at 60fps.**

### Raspberry Pi 4 - Manual Setup

**Pi 4 uses the legacy Unicam driver system.**

Install the needed headers:
```bash
sudo apt install linux-headers dkms git
```

MODIFY DRIVER BEFORE BUILDING.
This will be improved soon so it can be set without modifying driver code 
```c
static int mode = 2; //0-640; 1-256; 2-384
static int fps = 60; //256: 25/50fps, 384/640: 30/60fps  
```

Run the setup script:
```bash
cd /rs300-v4l2-driver
# modify driver to select module
sudo nano rs300.c
chmod +x setup.sh
./setup.sh
```

In your config.txt file add the overlay
- For bookworm
```bash
sudo nano /boot/firmware/config.txt
```

Then Add
```bash
camera_auto_detect=0
dtoverlay=rs300
```

Then reboot
```bash
sudo reboot
```

## Usage

### Raspberry Pi 5 Usage

**For Pi 5, use the automated configuration script:**
```bash
./configure_media.sh
```

**Manual streaming commands for Pi 5:**
```bash
# Basic stream test
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=10

# Live viewing with ffplay
ffplay -f v4l2 -video_size 640x512 -pixel_format yuyv422 /dev/video0

# GStreamer pipeline
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=512 ! videoconvert ! autovideosink
```

**Camera controls (Pi 5):**
```bash
# List available controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls

# Trigger FFC calibration
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=ffc_trigger=0

# Change colormap
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=colormap=3

# Adjust brightness
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=brightness=50
```

### Raspberry Pi 4 Usage

**Viewing Device Information**
The camera should now be linked to the Raspberry Pi's Unicam driver and available on /dev/video0
Get basic information about the Unicam bridge driver:
```bash
v4l2-ctl --list-devices
v4l2-ctl --all -d /dev/video0
```

The actual camera is registered as a sub device in the driver

```bash
v4l2-ctl -d /dev/v4l-subdev0 --list-ctrls
```

### Testing Video Capture

Set the format resolution for the appropriate module

Mini2-256
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=256,height=192,pixelformat=YUYV
```
Mini2-384
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=384,height=288,pixelformat=YUYV
```
Mini2-640
```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

### gstreamer
gstreamer and has been faster than ffmpeg for me
first install this
```bash
sudo apt install gstreamer1.0-tools
```

set test-overlay=false to remove fps counter

Mini2-256
```bash  
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=256,height=192,framerate=50/1 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```
Mini2-384
```bash  
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=384,height=288,framerate=60/1 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```
Mini2-640
```bash  
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=512,framerate=60/1 ! videoconvert ! fpsdisplaysink video-sink=autovideosink text-overlay=true
```
### Camera Controls (Pi 4)
use v4l2 controls

this triggers the ffc shutter calibration
```bash
v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=ffc_trigger=0
```

Change colormaps
```bash
v4l2-ctl -d /dev/v4l-subdev0 --set-ctrl=colormap=3
```

## Troubleshooting

### Raspberry Pi 5 Troubleshooting

**1. Media Pipeline Issues**
```bash
# Check if driver loaded
dmesg | grep rs300
lsmod | grep rs300

# Verify camera detection
./configure_media.sh

# Check media controller topology
media-ctl -p

# Generate pipeline visualization
python3 media-topology-visualizer.py --check-formats
```

**2. Format Issues**
- Pi 5 only supports 16-bit packed formats (`YUYV8_1X16`, `UYVY8_1X16`)
- 8-bit dual lane formats (`*8_2X8`) will cause "Format mismatch!" errors
- Use `./configure_media.sh` to ensure proper format configuration

**3. No Video Stream**
```bash
# Check I2C communication (should be bus 10 on Pi 5)
i2cdetect -y 10

# Verify media links are enabled
media-ctl --print-topology | grep ENABLED

# Test basic streaming
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1
```

### Raspberry Pi 4 Troubleshooting

**Common Issues and Solutions:**

1. **Wrong resolution**: Make sure to set the stream to your modules resolution. By default the Unicam driver will have it as 640x480

If experiencing issues:

1. Check kernel messages: `dmesg | grep rs300`
2. In a seperate terminal: `dmesg wH`
3. Verify I2C connection: `i2c-detect -y 1`
4. Check for device: `ls -la /dev/video*`
5. I started getting a message that postinst wasn't running, but it's only needed for the first time or if changes to the device tree overlay are made

```bash
./setup.sh
sudo sh /usr/src/rs300-0.0.1/dkms.postinst
```

## Current Status

### ✅ **Raspberry Pi 5 - FULLY WORKING**
- **Driver**: RS300 module loads successfully 
- **I2C**: Communication working on i2c-10 bus at 0x3c
- **Media Pipeline**: Automatic configuration with `./configure_media.sh`
- **Streaming**: 60fps thermal video confirmed working
- **Formats**: YUYV8_1X16 and UYVY8_1X16 both supported
- **Controls**: FFC calibration, colormap, brightness accessible
- **Tools**: Visualization and diagnostic scripts included

### ✅ **Raspberry Pi 4 - WORKING**  
- **Driver**: Tested and working with Unicam
- **Resolutions**: 640x512 and 384x288 confirmed
- **Performance**: 60Hz video with minimal issues
- **Note**: Requires manual driver configuration for resolution

### 🚧 **Development Status**
- **256x192 Module**: I2C working, MIPI video troubleshooting in progress
- **Pi Zero 2W**: Hardware power limitations prevent reliable operation
- **ISP Integration**: Future Pi 5 enhancement for hardware denoising

---

## Advanced Usage

### Media Controller Visualization (Pi 5)
```bash
# Generate pipeline diagram
python3 media-topology-visualizer.py --check-formats --show-links

# Configure with visualization
./configure_media.sh --visualize
```

### Manual Pipeline Configuration (Pi 5)
```bash
# For advanced users - manual media controller setup
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -V "'csi2':0 [fmt:YUYV8_1X16/640x512]"
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
```

### Diagnostic Commands
```bash
# Check driver status
dkms status | grep rs300
lsmod | grep rs300

# I2C verification
i2cdetect -y 10  # Pi 5
i2cdetect -y 1   # Pi 4

# Media controller status (Pi 5)
media-ctl --print-topology
```


