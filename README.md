# RS300 Thermal Camera Driver for Raspberry Pi

This driver is for the "mini2" thermal camera module refered to as the rs300 in the original driver.
Tested with a 640x512 module on Raspberry Pi 4b running Raspberry OS - Bookworm

## TODO
- Raspberry Pi 5 testing coming soon
- Get the 256 module working


## Where I got the module
I've bought from two stores on Alibaba who sell the same module
1. Purple River Technology: https://purpleriver.en.alibaba.com/index.html?spm=a2700.details.0.0.1a245460PSIafr&from=detail&productId=1601081970203
   - I bought a 256 module with 15mm lens from them and they helped me with the custom PCB board for the module that allows it to connect to the Pi. Excellent Customer support.

2. Shenzhen Chengen Thermovisiontechnology: https://cersnv.en.alibaba.com/index.html?spm=a2700.details.0.0.7d0f2dfbCpGJwE&from=detail&productId=1601308525861
   - I bought the 640 module with 25mm lens from this seller and put the PCB board from purple river on it to connect it to the pi.


## Installation

### Prerequisites
- Raspberry Pi with Raspbian/Raspberry Pi OS
- Linux headers installed
- DKMS support

### Build and Install
Run the included setup script to build and install the driver:

```bash
sudo apt install linux-headers dkms git
```

```bash
chmod +x setup.sh
./setup.sh
```

## Usage

### Viewing Device Information

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

1. **No video streaming**: Try different bus_format values (0 or 1)
2. **Wrong resolution**: Check mode parameter matches your camera
3. **I2C errors**: Verify camera connections and power
4. **Format errors**: Try different format types (16 or 18)

## Troubleshooting

If experiencing issues:

1. Check kernel messages: `dmesg | grep rs300`
2. Verify I2C connection: `i2c-detect -y 1`
3. Check for device: `ls -la /dev/video*`

# RS300 V4L2 Driver (Mini2 Thermal Camera)


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
I started getting a message that postinst wasn't running, but it's only needed for the first time or if changes to the device tree overlay are made

```bash
./setup.sh
sudo sh /usr/src/rs300-0.0.1/dkms.postinst
```
