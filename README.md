# RS300 Thermal Camera Driver for Raspberry Pi
- Credit to [will127534](https://github.com/will127534) for the setup script in his imx294 driver
- Used the [IMX219.c](https://github.com/raspberrypi/linux/blob/rpi-6.6.y/drivers/media/i2c/imx219.c) as a template for the driver on RPi.

This driver is for the "mini2" thermal camera module refered to as the rs300 in the original driver.

Tested with a 640x512 module on Raspberry Pi 4b running Raspberry OS - Bookworm

**WORK IN PROGRESS**

## TODO
- Raspberry Pi 5 testing coming soon
- Get the 256 module working with raspberry pi


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
1. 256x192 - 25/50hz   (50hz is available over USB)
2. 384x288 - 30/60hz   (Untested - Don't have this module)
3. 640x512 - 30/60hz   (60hz is NOT supported by USB)

NETD: 50mk or 40mk I've seen it listed as both
------
### Video Output Methods
This module can output two simultaneous video streams. One Digital, One Analog.

For the custom PCB I have there is USB 2.0, MIPI, and CVBS. The camera also support DVP and BT656 as digital formats but they are not broken out on this board and I haven't researched them further.

#### USB 2.0
- As mentioned above the 640 module is limited to 30fps over USB. I'm unsure about the 384 module.
- The camera is UVC compliant and will work the same as a webcam. For the 256 module you can even switch the framerate from 25fps to 50fps in standard camera apps.
- The pixel format is YUYV422 but you can change it to a different order like UYVU.
- For sending commands to the camera over USB, such as brightness changes, shutter calibration, changing colormaps, you will need to use the provided SDK. The SDK is for windows and linux but I have only used the linux portion so far

#### MIPI
- Allows digital 60fps with the 640 module
- The custom PCB has the 15 pin connector compatible with RPi 4B MIPI CSI-2 camera port. However, the design uses a same sided head ribbon cable and not a reverse head like most other RPi cameras. I will try to have this changed before a batch is made for easier compatibility. This will be critical for easy connection to newer RPis with the 22pin camera ports.
- Uses the rs300 driver I am currently working on to connect to the Pi. I'm implementing camera commands over I2C without the need for any SDK or precompiled binary files. As many commands as I can I am making them V4L2 compliant for easy integration.

#### CVBS
- Analog Output can be set to NTSC or PAL
- Can be output simultaneously with USB or MIPI
- Only White Hot and Black Hot colormaps are displayed. So far all other parameters seem to equally affect the analog video stream like; brighntess, contrast, noise reduction, digital detail enhancement (DDE), and digital zoom.


## Installation

### Prerequisites
- Raspberry Pi with Raspbian/Raspberry Pi OS
- Linux headers installed
- DKMS support

### Build and Install

Install the needed headers:
```bash
sudo apt install linux-headers dkms git
```

Run the setup script:
```bash
cd /rs300-v4l2-driver
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


Then reboot
```bash
sudo reboot
```

## Usage

### Viewing Device Information
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

Test the camera with v4l2-ctl:

```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV --stream-mmap --stream-count=10
```

### Common Issues and Solutions

1. **Wrong resolution**: Make sure to set the stream to your modules resolution. By default the Unicam driver will have it as 640x480


## Troubleshooting

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

# Various Other Commands I used for troubleshooting
TODO: Cleanup this section


```bash
cd rs300-v4l2-driver
```

- make them executable

```bash
chmod +x setup.sh dkms.postinst
```

# Install DKMS if not already installed

```bash
sudo apt install linux-headers dkms git
```

# Run the setup script

```bash
./setup.sh
```

# Check DKMS status and if module is loaded

```bash
dkms status
lsmod | grep rs300
modinfo rs300
```

# check i2c devices

```bash
ls /dev/i2c*
i2cdetect -l
i2cdetect -y 10 # right now it's bus 10 for me
i2cdump -f -y 10 0x3c  # i2c 10, 0x3c is your device address
```

# check v4l2 devices

```bash
ls /dev/video*
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --all
# List supported formats
v4l2-ctl -d /dev/video0 --list-formats-ext
# List supported controls
v4l2-ctl -d /dev/video0 --list-ctrls
# Show current input/output
v4l2-ctl -d /dev/video0 --info
```

## set v4l2 parameters manually; resolution, fps, etc   

```bash
v4l2-ctl -d /dev/video0 --set-fmt-video width=256,height=192,pixelformat=YUYV
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=YUYV
v4l2-ctl --stream-mmap -d /dev/video0 -o test.yuv

```

ffplay -f v4l2 -input_format yuyv422 -video_size 256x192 -i /dev/video0

ffplay -f v4l2 -input_format yuyv422 -video_size 640x512 -i /dev/video0


# Rebuild the module

```bash
cd rs300-v4l2-driver
./setup.sh
```

# Check after rebuild and reboot
- check if the module is on i2c
- check if the video node is created

```bash
i2cdetect -l 
i2cdetect -y 10
lsmod | grep rs300
dmesg | grep -i rs300
modinfo rs300

media-ctl -p
ls /dev/v4l-subdev*

v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --all

v4l2-ctl -d /dev/video0 --list-ctrls
v4l2-ctl -d /dev/v4l-subdev0 --list-ctrls

v4l2-ctl -d /dev/video0 --list-formats-ext
vcgencmd get_camera


v4l2-ctl -d /dev/v4l-subdev0 --all
sudo cat /dev/kmsg | grep rs300
v4l2-ctl -d /dev/video0 --stream-mmap -o test.yuv
sudo dmesg -wH
ffplay -f video4linux2 -input_format yuyv422 -video_size 256x192 -i /dev/video0
ffplay -f video4linux2 -input_format yuyv422 -video_size 640x512 -i /dev/video0
```

## rs300-v4l2-driver

