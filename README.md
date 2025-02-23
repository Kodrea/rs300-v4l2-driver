
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
i2cdump -f -y 10 0x3c
lsmod | grep rs300
dmesg | grep -i rs300
modinfo rs300
vcgencmd get_camera
v4l2-ctl --list-devices
v4l2-ctl -d /dev/video0 --all
v4l2-ctl -d /dev/video0 --list-ctrls
v4l2-ctl -d /dev/video0 --list-formats-ext
v4l2-ctl -d /dev/video0 --stream-mmap -o test.yuv
media-ctl -p
ls /dev/v4l-subdev*
v4l2-ctl -d /dev/v4l-subdev0 --all
sudo cat /dev/kmsg | grep rs300
```

sudo i2ctransfer -y 10 w20@0x3c 0x1d 0x00 0x10 0x02 0x81 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01 0x00 0x00 0x00 0x78 0x34
sudo i2ctransfer -y 10 r2@0x3c 0x02 0x00
sudo i2ctransfer -y 10 r4@0x3c 0x1d 0x00

# rs300-v4l2-driver
