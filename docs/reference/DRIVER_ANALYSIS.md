# RS300 V4L2 Driver - Technical Reference

## Executive Summary

V4L2 subdevice driver for RS300 thermal camera (640×512@60fps) on Raspberry Pi 5 BCM2712/RP1-CFE. 2,946 lines of C code with 11 V4L2 controls and I2C command interface.

**Stats**: 3 video modes, 4 YUV422 formats, 11 I2C commands, 2-lane CSI-2 @ 80MHz

---

## 1. Architecture

### Hardware Interface
- **I2C Control** (i2c-10, addr 0x3c): Command/status via register 0x1d00/0x0200
- **MIPI CSI-2**: 2-lane @ 80MHz, YUV422, 200-400MHz pixel rate

### Core Data Structures

**struct rs300**:
- v4l2_subdev, media_pad[2], v4l2_mbus_framefmt
- 11 v4l2_ctrl pointers, xvclk, reset_gpio, 3x regulators
- streaming flag, current mode, mutex

**struct rs300_mode**: width, height, max_fps (fraction), media_bus_code

**struct ioctl_data**: bRequestType, bRequest, wValue, wIndex (register), *data, wLength, timeout

### Initialization Flow
```
sensor_mod_init() → i2c_add_driver() → rs300_probe()
  ├─ v4l2_i2c_subdev_init()
  ├─ rs300_check_hwcfg() [verify 2-lane, 80MHz]
  ├─ rs300_get_regulators() + rs300_power_on()
  ├─ rs300_set_default_format() → YUYV 640×512
  ├─ rs300_init_controls() [11 controls]
  ├─ media_entity_pads_init()
  └─ v4l2_async_register_subdev_sensor()
```

---

## 2. I2C Protocol

### Register Map

| Addr | Name | Dir | Purpose |
|------|------|-----|---------|
| 0x1d00 | I2C_VD_BUFFER_RW | R/W | Command buffer |
| 0x0200 | I2C_VD_BUFFER_STATUS | R | Status (busy/failed/error) |

### 18-Byte Command Packet

```
[0] Class    [1] Module   [2] SubCmd   [3] Reserved
[4-15] Parameters (12 bytes)
[16-17] CRC-16-CCITT (LSB first)
```

### CRC-16-CCITT (poly 0x1021)
```c
crc = 0x0000
for each byte: crc ^= (byte << 8)
  for 8 bits: crc = (crc << 1) ^ 0x1021 if MSB set, else << 1
```

### Status Register (0x0200)
- Bit 0: Busy (1=executing)
- Bit 1: Failed (1=error)
- Bits 2-7: Error code (0x00=success, 0x01=length, 0x02=unknown, 0x03=hw_error, 0x05-0x07=crc_error)

### Command Execution Pattern
```
1. Build 18-byte buffer (class, module, subcmd, params)
2. Calculate CRC over first 16 bytes
3. Write to 0x1d00 via i2c
4. Poll 0x0200 (50ms delay, 5 retries max)
5. Check: not busy && !failed → success
```

### Transfer Functions
- **read_regs()**: 2-msg I2C (write 16-bit BE address, read data)
- **write_regs()**: 1-msg I2C (address + data in one transfer)

---

## 3. Camera Commands (11 Total)

| Command | Class | Module | SubCmd | Range | Func |
|---------|-------|--------|--------|-------|------|
| GET Brightness | 0x10 | 0x04 | 0x87 | - | rs300_get_brightness() |
| SET Brightness | 0x10 | 0x04 | 0x47 | 0-100 | rs300_brightness_correct() |
| GET Colormap | 0x10 | 0x03 | 0x85 | - | rs300_get_colormap() |
| SET Colormap | 0x10 | 0x03 | 0x45 | 0-11 | rs300_set_colormap() |
| FFC Trigger | 0x10 | 0x02 | 0x43 | - | rs300_shutter_cal() |
| SET Zoom | 0x01 | 0x31 | 0x42 | 1-8 | rs300_set_zoom() |
| SET Scene Mode | 0x10 | 0x04 | 0x42 | 0-9 | rs300_set_scene_mode() |
| SET DDE | 0x10 | 0x04 | 0x45 | 0-100 | rs300_set_dde() |
| SET Contrast | 0x10 | 0x04 | 0x4A | 0-100 | rs300_set_contrast() |
| SET Spatial NR | 0x10 | 0x04 | 0x4B | 0-100 | rs300_set_spatial_nr() |
| SET Temporal NR | 0x10 | 0x04 | 0x4C | 0-100 | rs300_set_temporal_nr() |
| SET YUV Format | 0x10 | 0x03 | 0x4D | 0-3 | rs300_set_yuv_format() |
| SET FPS | 0x10 | 0x10 | 0x46 | {25,30,50,60} | rs300_set_fps() |

**Colormap palette** (0-11): White Hot, Reserved, Sepia, Ironbow, Rainbow, Night, Aurora, Red Hot, Jungle, Medical, Black Hot, Golden Red

**Scene modes** (0-9): Low, Linear Stretch, Low Contrast, General, High Contrast, Highlight, Reserved×3, Outline

**Special Notes**:
- Zoom uses **hardcoded CRC** (0x0a, 0x06) instead of calculation
- Brightness & colormap include verification readback
- FFC uses 1000ms delay (longest)
- FPS returns 0 even on failure (non-critical)

---

## 4. V4L2 Controls (11 Total)

| Control | Type | Range | Read-Only | Default |
|---------|------|-------|-----------|---------|
| LINK_FREQ | INT_MENU | 80MHz | ✓ | 80MHz |
| PIXEL_RATE | INTEGER | 200-400MHz | ✓ | 400MHz |
| BRIGHTNESS | INTEGER | 0-100 | ✗ | 50 |
| CONTRAST | INTEGER | 0-100 | ✗ | 50 |
| ZOOM_ABSOLUTE | INTEGER | 1-8 | ✗ | 1 |
| COLORMAP (CUSTOM+1) | MENU | 0-11 | ✗ | 0 |
| FFC_TRIGGER (CUSTOM+2) | BUTTON | - | ✗ | - |
| SCENE_MODE (CUSTOM+3) | MENU | 0-9 | ✗ | 3 |
| DDE (CUSTOM+4) | INTEGER | 0-100 | ✗ | 50 |
| SPATIAL_NR (CUSTOM+5) | INTEGER | 0-100 | ✗ | 50 |
| TEMPORAL_NR (CUSTOM+6) | INTEGER | 0-100 | ✗ | 50 |

**Pixel rate calc**: 16-bit formats = 400MHz, 8-bit = 200MHz (updated on format change)

---

## 5. Video Formats & Streaming

### Supported Modes
```c
Mode 0: 640×512 @ 60fps (PRIMARY)  → YUYV8_1X16
Mode 1: 256×192 @ 25fps            → YUYV8_1X16
Mode 2: 384×288 @ 30fps            → YUYV8_1X16
```

### Media Bus Formats (4 codes)
- YUYV8_1X16 (0x200f) - PRIMARY for RP1-CFE
- UYVY8_1X16 (0x200e)
- YUYV8_2X8 (0x2007) - Legacy
- UYVY8_2X8 (0x2006) - Legacy

**CRITICAL**: RP1-CFE **only supports 16-bit packed** (*8_1X16), not 8-bit dual lane

### Colorspace (Fixed)
- V4L2_COLORSPACE_SMPTE170M (SDTV YUV)
- Encoding: BT.601
- Quantization: Limited range
- Transfer: 709

### Pads
- Pad 0 (IMAGE_PAD): Main video stream
- Pad 1 (METADATA_PAD): CSI-2 metadata (code=SENSOR_DATA, 16384×1)

### Stream Start (rs300_set_stream enable=1)
```
1. Check streaming state (avoid duplicate)
2. Set FPS via command (0x10/0x10/0x46)
3. Build 28-byte start_regs buffer
4. Calculate dual CRCs
5. Write to 0x1d00
6. Poll 0x0200 (10 retries, 100ms delay)
7. Final check after 2000ms
```

**Start buffer** (28 bytes): Header, reserved, length (0x0a), CRC1@[14:15], CRC2@[16:17], path(0x00), source(0x16), dest(0x03), fps, width_LE, height_LE

### Stream Stop
Write 27-byte stop_regs (similar, cmd 0xc2 vs 0xc1, path=1, dest=0)

---

## 6. Hardware Platform (Pi 5)

**Device Tree Requirements**: Verify 2-lane CSI-2 @ 80MHz via endpoint config

**Power Supplies** (3): VANA (I/O), VDIG (analog), VDDL (core)

**Reset GPIO**: Active-low (currently commented out in power_on)

**MIPI CSI-2**: 2-lane @ 80MHz, 200-400MHz pixel rate, YUV422

---

## 7. Code Quality Notes

### Known Issues
- ~500 lines duplicate command execution code (consolidation abandoned - kernel crash risk)
- Mode switching requires driver rebuild/reboot
- Zoom command uses hardcoded CRC
- Error handling inconsistent (FPS returns 0 on failure)

### File Structure
```
rs300.c (2946 lines)
├─ Headers, metadata, menus [1-96]
├─ Registers, CRC, helpers [105-267]
├─ Data structures, modes [269-371]
├─ Format handling [378-489]
├─ Camera commands [494-1624]
├─ V4L2 control ops [1626-1684]
├─ Pad ops (enum, get, set) [1686-1951]
├─ Streaming ops [1953-2256]
├─ Init/probe/remove [2258-2893]
└─ Driver registration [2906-2946]
```

### Strengths
- Comprehensive logging (dev_info/dev_err)
- Robust format validation
- Verification readback (colormap, brightness)
- Detailed error code interpretation
- V4L2 compliance

---

**Version**: 1.0 | **Last Updated**: 2025-10-30
