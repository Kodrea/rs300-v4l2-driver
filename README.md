# RS300 Thermal Camera Driver for Raspberry Pi

This driver supports the RS300 thermal camera on Raspberry Pi platforms.

## Installation

### Prerequisites
- Raspberry Pi with Raspbian/Raspberry Pi OS
- Linux headers installed
- DKMS support

### Build and Install
Run the included setup script to build and install the driver:

```bash
chmod +x setup.sh
./setup.sh
```

## Usage

### Driver Parameters

The driver supports several parameters that can be set at load time:

| Parameter   | Values           | Description                                   |
|-------------|------------------|-----------------------------------------------|
| mode        | 0=256x192, 1=640x512 | Camera resolution mode                       |
| fps         | integer (e.g. 25)     | Frames per second                            |
| width       | integer (e.g. 256)    | Image width                                  |
| height      | integer (e.g. 192)    | Image height                                 |
| type        | 16=YUYV, 18=UYVY     | Image format type                            |
| bus_format  | 0=YUYV8_2X8, 1=YUYV8_1X16 | Media bus format                       |
| debug       | 0=off, 1=on          | Enable debugging output                      |

### Setting Parameters at Module Load

You can set these parameters when loading the module:

```bash
sudo modprobe rs300 mode=1 fps=30 bus_format=1 debug=1
```

Or permanently in `/etc/modprobe.d/rs300.conf`:

```
options rs300 mode=1 fps=30 bus_format=1 debug=1
```

### Runtime Configuration

Some parameters can be changed at runtime via sysfs:

```bash
# Change bus format
echo 1 > /sys/devices/platform/soc/fe201000.serial/tty/ttyAMA0/device/subsystem/devices/i2c-1/1-003c/bus_format
```

## Debugging

### Enabling Debug Output

To enable debug output:

1. Load the module with the debug parameter:
   ```bash
   sudo modprobe rs300 debug=1
   ```

2. View kernel logs:
   ```bash
   dmesg -w
   ```

### Checking Camera Status

When debug mode is enabled, the driver will print detailed information about camera operations:

- Driver initialization
- Format negotiation
- Streaming start/stop
- I2C communication details

### Viewing Device Information

Get basic information about the device:

```bash
v4l2-ctl --list-devices
v4l2-ctl --all -d /dev/video0
```

### Testing Video Capture

Test the camera with v4l2-ctl:

```bash
v4l2-ctl -d /dev/video0 --set-fmt-video=width=256,height=192,pixelformat=YUYV --stream-mmap --stream-count=10
```

### Common Issues and Solutions

1. **No video streaming**: Try different bus_format values (0 or 1)
2. **Wrong resolution**: Check mode parameter matches your camera
3. **I2C errors**: Verify camera connections and power
4. **Format errors**: Try different format types (16 or 18)

## Troubleshooting

If experiencing issues:

1. Enable debug mode: `sudo modprobe -r rs300 && sudo modprobe rs300 debug=1`
2. Check kernel messages: `dmesg | grep rs300`
3. Verify I2C connection: `i2c-detect -y 1`
4. Check for device: `ls -la /dev/video*`

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

sudo i2ctransfer -y 10 w20@0x3c 0x1d 0x00 0x10 0x02 0x81 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x78 0x34
sudo i2ctransfer -y 10 r2@0x3c 0x02 0x00
sudo i2ctransfer -y 10 r4@0x3c 0x1d 0x00

# rs300-v4l2-driver
./setup.sh
sudo sh /usr/src/rs300-0.0.1/dkms.postinst
