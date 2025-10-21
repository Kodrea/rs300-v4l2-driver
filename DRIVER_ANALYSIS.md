# RS300 V4L2 Driver - Deep Technical Analysis

## Executive Summary

The `rs300.c` driver is a Linux V4L2 (Video for Linux 2) subdevice driver for the RS300 thermal camera module. It implements a complete I2C-based command protocol for controlling a 640×512 thermal imaging sensor connected via MIPI CSI-2 interface to Raspberry Pi 5 (BCM2712/RP1-CFE architecture).

**Driver Statistics:**
- **Total Lines**: 2,946 lines of C code
- **Supported Modes**: 3 video modes (640×512@60fps primary)
- **V4L2 Controls**: 11 controls (7 custom + 4 standard)
- **Camera Commands**: 11+ I2C command implementations
- **Media Bus Formats**: 4 YUV422 formats (16-bit and 8-bit variants)

---

## 1. Architecture Overview

### 1.1 Driver Purpose & Hardware Interface

The RS300 driver interfaces with a thermal imaging camera module through two primary channels:

1. **I2C Control Bus** (I2C address 0x3c on i2c-10):
   - Command/control interface
   - Register access (command buffer 0x1d00, status 0x0200)
   - Bi-directional parameter exchange

2. **MIPI CSI-2 Video Interface**:
   - 2-lane CSI-2 at 80MHz link frequency
   - YUV422 video data streaming
   - Supports 200-400MHz pixel rates

### 1.2 Core Data Structures

#### `struct rs300` (rs300.c:303-340)
```c
struct rs300 {
    struct v4l2_subdev sd;              // V4L2 subdevice
    struct media_pad pad[NUM_PADS];     // 2 pads: IMAGE + METADATA
    struct v4l2_mbus_framefmt fmt;      // Current format
    struct clk *xvclk;                  // External clock
    struct gpio_desc *reset_gpio;       // Reset GPIO
    struct regulator_bulk_data supplies[3]; // Power supplies
    struct v4l2_ctrl_handler ctrl_handler;  // Control handler

    // V4L2 Controls
    struct v4l2_ctrl *pixel_rate;
    struct v4l2_ctrl *link_frequency;
    struct v4l2_ctrl *brightness;
    struct v4l2_ctrl *shutter_cal;      // FFC trigger
    struct v4l2_ctrl *colormap;         // Palette selection
    struct v4l2_ctrl *zoom;
    struct v4l2_ctrl *scene_mode;
    struct v4l2_ctrl *dde;              // Digital Detail Enhancement
    struct v4l2_ctrl *contrast;
    struct v4l2_ctrl *spatial_nr;       // Spatial noise reduction
    struct v4l2_ctrl *temporal_nr;      // Temporal noise reduction

    const struct rs300_mode *mode;      // Current video mode
    struct mutex mutex;                 // Serialization lock
    bool streaming;                     // Streaming state
};
```

#### `struct rs300_mode` (rs300.c:275-280)
Defines supported video modes:
```c
struct rs300_mode {
    unsigned int width;
    unsigned int height;
    struct v4l2_fract max_fps;         // Frame rate as fraction
    u32 code;                          // Media bus format code
};
```

#### `struct ioctl_data` (rs300.c:116-124)
Custom IOCTL structure for direct register access (legacy USB-I2C compatibility):
```c
struct ioctl_data {
    unsigned char bRequestType;
    unsigned char bRequest;
    unsigned short wValue;
    unsigned short wIndex;             // Register address
    unsigned char* data;               // Data pointer
    unsigned short wLength;            // Data length
    unsigned int timeout;              // Timeout in ms
};
```

### 1.3 Module Initialization Flow

```
sensor_mod_init() [2930]
  └─> i2c_add_driver(&rs300_i2c_driver)
       └─> rs300_probe() [2759]
            ├─> Allocate rs300 structure
            ├─> v4l2_i2c_subdev_init() - Initialize V4L2 subdev
            ├─> rs300_check_hwcfg() - Verify device tree config
            │    └─> Check 2-lane CSI-2 @ 80MHz
            ├─> rs300_get_regulators() - Get power supplies
            ├─> rs300_power_on() - Enable regulators
            ├─> rs300_set_default_format() - Set 640×512 YUYV
            ├─> rs300_set_yuv_format(2) - Configure YUYV ordering
            ├─> rs300_init_controls() - Register 11 V4L2 controls
            ├─> media_entity_pads_init() - Initialize media pads
            └─> v4l2_async_register_subdev_sensor() - Register subdev
```

---

## 2. I2C Communication Protocol

### 2.1 Register Map

| Address | Name | Direction | Purpose |
|---------|------|-----------|---------|
| `0x1d00` | I2C_VD_BUFFER_RW | R/W | Command/data buffer (read/write mode) |
| `0x9d00` | I2C_VD_BUFFER_HLD | R/W | Command/data buffer (hold mode) |
| `0x0200` | I2C_VD_BUFFER_STATUS | R | Command execution status |
| `0x8000` | I2C_VD_CHECK_ACCESS | - | Access check flag |

### 2.2 Command Buffer Structure (18 bytes)

All camera commands follow this 18-byte packet format:

```
Offset | Size | Field         | Description
-------|------|---------------|----------------------------------
0      | 1    | Class         | Command class (0x01, 0x10, etc.)
1      | 1    | Module        | Module index (0x02-0x10)
2      | 1    | SubCmd        | Sub-command ID
3      | 1    | Reserved      | Usually 0x00
4-15   | 12   | Parameters    | Command-specific parameters
16-17  | 2    | CRC-16        | Checksum (LSB first)
```

**Example - Shutter Calibration (FFC)** (rs300.c:1226-1239):
```c
cmd_buffer[0] = 0x10;  // Class
cmd_buffer[1] = 0x02;  // Module (shutter)
cmd_buffer[2] = 0x43;  // SubCmd (FFC trigger)
cmd_buffer[3] = 0x00;  // Reserved
// Bytes 4-15: zeros
crc = do_crc(cmd_buffer, 16);
cmd_buffer[16] = crc & 0xFF;        // CRC low byte
cmd_buffer[17] = (crc >> 8) & 0xFF; // CRC high byte
```

### 2.3 CRC-16 Calculation (rs300.c:152-170)

The driver uses **CRC-16-CCITT** polynomial 0x1021:

```c
unsigned short do_crc(unsigned char *ptr, int len)
{
    unsigned short crc = 0x0000;
    while(len--) {
        crc ^= (unsigned short)(*ptr++) << 8;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}
```

**Usage Pattern**: CRC is calculated over first 16 bytes, appended as bytes 16-17 in **little-endian** order.

### 2.4 Status Register (0x0200) Bit Layout

```
Bit 7-2: Error Code (6 bits)
  0x00 = Success
  0x01 = Length error
  0x02 = Unknown command
  0x03 = Hardware error
  0x04 = Command not enabled
  0x05-0x07 = CRC error

Bit 1: Failed Flag
  0 = Success
  1 = Command failed

Bit 0: Busy Flag
  0 = Idle/Complete
  1 = Command executing
```

### 2.5 Command Execution Pattern

All camera command functions follow this standardized pattern (see rs300.c:635-703 for example):

```c
// 1. Construct command buffer
cmd_buffer[0] = 0x10;  // Class
cmd_buffer[1] = module_index;
cmd_buffer[2] = subcmd;
cmd_buffer[3] = 0x00;
cmd_buffer[4] = value;
memset(&cmd_buffer[5], 0, 11);

// 2. Calculate CRC
crc = do_crc(cmd_buffer, 16);
cmd_buffer[16] = crc & 0xFF;
cmd_buffer[17] = (crc >> 8) & 0xFF;

// 3. Write to command buffer
write_regs(client, 0x1d00, cmd_buffer, 18);

// 4. Poll status register (with retry)
while (retry_count < max_retries) {
    msleep(50);  // Wait for processing
    read_regs(client, 0x0200, status_buffer, 1);

    bool is_busy = (status_buffer[0] & 0x01);
    bool has_failed = (status_buffer[0] & 0x02);
    u8 error_code = (status_buffer[0] >> 2) & 0x3F;

    if (!is_busy) {
        if (has_failed) return -EIO;
        return 0;  // Success
    }
    retry_count++;
}
return -ETIMEDOUT;
```

### 2.6 I2C Transfer Functions

#### `read_regs()` (rs300.c:205-231)
```c
int read_regs(struct i2c_client *client, u32 reg, u8 *val, int len)
```
- Uses 2-message I2C transaction (write address, read data)
- Register address sent as 16-bit big-endian
- Returns 0 on success, negative errno on failure

#### `write_regs()` (rs300.c:233-267)
```c
int write_regs(struct i2c_client *client, u32 reg, u8 *val, int len)
```
- Single I2C message with register address + data
- Dynamically allocates buffer for address+data
- Returns 0 on success, negative errno on failure

---

## 3. Camera Command System

### 3.1 Command Categories

| Class | Module | Purpose | Commands Implemented |
|-------|--------|---------|---------------------|
| 0x10 | 0x02 | Shutter | FFC calibration (0x43) |
| 0x10 | 0x03 | Display | Colormap (0x45, 0x85), YUV format (0x4D) |
| 0x10 | 0x04 | Image Processing | Brightness (0x47, 0x87), Scene mode (0x42), Contrast (0x4A), DDE (0x45), Spatial NR (0x4B), Temporal NR (0x4C) |
| 0x10 | 0x10 | MIPI | FPS setting (0x46) |
| 0x01 | 0x31 | Zoom | Zoom level (0x42) |
| 0x01 | 0x01 | Device Info | Device name (0x81) |

### 3.2 Detailed Command Implementations

#### 3.2.1 Brightness Control (GET: rs300.c:502-633, SET: rs300.c:1327-1456)

**GET Brightness (SubCmd 0x87)**:
```c
// Command structure
cmd[0] = 0x10; cmd[1] = 0x04; cmd[2] = 0x87;
cmd[4] = 0x01; cmd[12] = 0x01;

// Response parsing
brightness_value = result_buffer[4];  // Range: 0-100
```

**SET Brightness (SubCmd 0x47)**:
```c
// Command structure
cmd[0] = 0x10; cmd[1] = 0x04; cmd[2] = 0x47;
cmd[4] = brightness_param;  // 0-100

// Verification
rs300_get_brightness(rs300, &current_brightness);
// Warns if mismatch
```

**Special Features**:
- Uses 200ms poll interval (longer than other commands)
- Implements verification readback after setting
- Detailed error code interpretation

#### 3.2.2 Colormap Selection (GET: rs300.c:991-1078, SET: rs300.c:1081-1213)

**12 Color Palettes** (rs300.c:53-67):
```c
0  = White Hot
1  = Reserved
2  = Sepia
3  = Ironbow
4  = Rainbow
5  = Night
6  = Aurora
7  = Red Hot
8  = Jungle
9  = Medical
10 = Black Hot
11 = Golden Red Glory_Hot
```

**GET Colormap (SubCmd 0x85)**:
```c
cmd[0] = 0x10; cmd[1] = 0x03; cmd[2] = 0x85;
cmd[12] = 0x01;
```

**SET Colormap (SubCmd 0x45)**:
```c
cmd[0] = 0x10; cmd[1] = 0x03; cmd[2] = 0x45;
cmd[4] = 0x00;           // Fixed parameter
cmd[5] = colormap_value; // 0-11
```

**Special Features**:
- Validates range 0-11
- Includes verification readback with 100ms delay
- Warns on mismatch

#### 3.2.3 Shutter Calibration / FFC (rs300.c:1215-1325)

**Flat Field Correction** (SubCmd 0x43):
```c
cmd[0] = 0x10; cmd[1] = 0x02; cmd[2] = 0x43;
// All parameters zero
```

**Special Features**:
- Uses 1000ms poll interval (longest wait time)
- Critical for thermal camera calibration
- Triggered via V4L2_CID_CUSTOM_BASE+2 button control
- No parameters required

#### 3.2.4 Zoom Control (rs300.c:1458-1539)

**Zoom Levels: 1x - 8x** (SubCmd 0x42):
```c
cmd[0] = 0x01; cmd[1] = 0x31; cmd[2] = 0x42;
cmd[5] = zoom_level * 10;  // 10, 20, 30...80
cmd[16] = 0x06; cmd[17] = 0x0A; // Fixed CRC
```

**Note**: This is the only command with **fixed CRC values** instead of calculated CRC (see TODO at rs300.c:1459).

#### 3.2.5 Scene Mode (rs300.c:1541-1624)

**10 Scene Modes** (rs300.c:70-82):
```c
0 = Low
1 = Linear Stretch
2 = Low Contrast
3 = General Mode (default)
4 = High Contrast
5 = Highlight
6-8 = Reserved
9 = Outline Mode
```

**SET Scene Mode (SubCmd 0x42)**:
```c
cmd[0] = 0x10; cmd[1] = 0x04; cmd[2] = 0x42;
cmd[4] = scene_mode_value;  // 0-9
```

#### 3.2.6 Image Enhancement Controls

All following commands share similar structure (Class 0x10, Module 0x04):

**DDE - Digital Detail Enhancement** (rs300.c:635-704)
- SubCmd: 0x45
- Range: 0-100
- Default: 50

**Contrast** (rs300.c:778-847)
- SubCmd: 0x4A
- Range: 0-100
- Default: 50

**Spatial Noise Reduction** (rs300.c:849-918)
- SubCmd: 0x4B
- Range: 0-100
- Default: 50

**Temporal Noise Reduction** (rs300.c:920-989)
- SubCmd: 0x4C
- Range: 0-100
- Default: 50

#### 3.2.7 YUV Format Configuration (rs300.c:706-776)

**SET YUV Format** (SubCmd 0x4D):
```c
cmd[0] = 0x10; cmd[1] = 0x03; cmd[2] = 0x4D;
cmd[4] = format;  // 0=UYVY, 1=VYUY, 2=YUYV, 3=YVYU
```

**Used in Probe**: Sets format to YUYV (value 2) at rs300.c:2834.

#### 3.2.8 FPS Configuration (rs300.c:1997-2080)

**SET FPS** (SubCmd 0x46):
```c
cmd[0] = 0x10; cmd[1] = 0x10; cmd[2] = 0x46;
cmd[4] = 0x01;  // Enable
cmd[5] = 0x03;  // MIPI Progressive
cmd[6] = fps;   // 25, 30, 50, or 60
```

**Special Features**:
- Validates fps ∈ {25, 30, 50, 60}
- Uses 300ms poll interval (15 max retries)
- Called during stream start (rs300.c:2139)
- Returns 0 even on failure (non-critical)

### 3.3 Command Pattern Analysis

**Repetitive Code**: ~500 lines of nearly identical command execution code across 11 functions.

**Common Pattern**:
1. Validate parameter range
2. Construct 18-byte command buffer
3. Calculate CRC (except zoom)
4. Write to 0x1d00
5. Poll 0x0200 with 50ms delay, 5 retries
6. Parse status (busy/failed/error_code)
7. Return 0/-EIO/-ETIMEDOUT

**Refactoring Opportunity**: Could be consolidated into single `rs300_send_command()` helper function (see Section 7.2).

---

## 4. V4L2 Control Interface

### 4.1 Control Overview (rs300.c:2512-2594)

**11 V4L2 Controls Registered**:

| Control ID | Name | Type | Range | Default | Read-Only |
|------------|------|------|-------|---------|-----------|
| V4L2_CID_LINK_FREQ | Link Frequency | INT_MENU | 80MHz | 80MHz | ✓ |
| V4L2_CID_PIXEL_RATE | Pixel Rate | INTEGER | 200-400MHz | 400MHz | ✓ |
| V4L2_CID_BRIGHTNESS | Brightness | INTEGER | 0-100 | 50 | ✗ |
| V4L2_CID_CONTRAST | Contrast | INTEGER | 0-100 | 50 | ✗ |
| V4L2_CID_ZOOM_ABSOLUTE | Zoom | INTEGER | 1-8 | 1 | ✗ |
| CUSTOM+1 | Colormap | MENU | 0-11 | 0 | ✗ |
| CUSTOM+2 | FFC Trigger | BUTTON | - | - | ✗ |
| CUSTOM+3 | Scene Mode | MENU | 0-9 | 3 | ✗ |
| CUSTOM+4 | Digital Detail Enhancement | INTEGER | 0-100 | 50 | ✗ |
| CUSTOM+5 | Spatial Noise Reduction | INTEGER | 0-100 | 50 | ✗ |
| CUSTOM+6 | Temporal Noise Reduction | INTEGER | 0-100 | 50 | ✗ |

### 4.2 Control Handler Registration

```c
// Initialize handler for 11 controls
v4l2_ctrl_handler_init(ctrl_hdlr, 11);
ctrl_hdlr->lock = &rs300->mutex;  // Serialize with mutex

// Link frequency (read-only)
rs300->link_frequency = v4l2_ctrl_new_int_menu(ctrl_hdlr, NULL,
    V4L2_CID_LINK_FREQ, 0, 0, link_freq_menu);
rs300->link_frequency->flags |= V4L2_CTRL_FLAG_READ_ONLY;

// Pixel rate (read-only, dynamic based on format)
u64 pixel_rate = rs300_get_pixel_rate(MEDIA_BUS_FMT_YUYV8_1X16);
rs300->pixel_rate = v4l2_ctrl_new_std(ctrl_hdlr, NULL,
    V4L2_CID_PIXEL_RATE, pixel_rate, pixel_rate, 1, pixel_rate);
rs300->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;

// Custom controls
rs300->colormap = v4l2_ctrl_new_custom(ctrl_hdlr, &colormap_ctrl, NULL);
rs300->shutter_cal = v4l2_ctrl_new_custom(ctrl_hdlr, &ffc_ctrl, NULL);
// ... etc

// Attach to subdevice
rs300->sd.ctrl_handler = ctrl_hdlr;
```

### 4.3 Control Handler (rs300.c:1626-1680)

```c
static int rs300_set_ctrl(struct v4l2_ctrl *ctrl)
{
    struct rs300 *rs300 = container_of(ctrl->handler, struct rs300, ctrl_handler);

    switch (ctrl->id) {
    case V4L2_CID_BRIGHTNESS:
        return rs300_brightness_correct(rs300, ctrl->val);
    case V4L2_CID_CUSTOM_BASE + 1:  // Colormap
        return rs300_set_colormap(rs300, ctrl->val);
    case V4L2_CID_CUSTOM_BASE + 2:  // FFC button
        if (ctrl->val == 0)
            return rs300_shutter_cal(rs300);
        break;
    case V4L2_CID_ZOOM_ABSOLUTE:
        return rs300_set_zoom(rs300, ctrl->val);
    // ... 7 more cases
    }
}
```

### 4.4 Pixel Rate Calculation (rs300.c:452-467)

**Dynamic pixel rate based on format**:

```c
static u64 rs300_get_pixel_rate(u32 format_code)
{
    switch (format_code) {
    case MEDIA_BUS_FMT_YUYV8_1X16:
    case MEDIA_BUS_FMT_UYVY8_1X16:
        return RS300_PIXEL_RATE_16BIT;  // 400 MHz
    case MEDIA_BUS_FMT_YUYV8_2X8:
    case MEDIA_BUS_FMT_UYVY8_2X8:
        return RS300_PIXEL_RATE;        // 200 MHz
    default:
        return RS300_PIXEL_RATE_16BIT;
    }
}
```

**Updated on format change** (rs300.c:1920-1926):
```c
if (rs300->pixel_rate) {
    u64 new_pixel_rate = rs300_get_pixel_rate(rs300->fmt.code);
    v4l2_ctrl_s_ctrl_int64(rs300->pixel_rate, new_pixel_rate);
}
```

---

## 5. Video Format & Streaming Management

### 5.1 Supported Video Modes (rs300.c:342-371)

```c
static struct rs300_mode supported_modes[] = {
    { // Mode 0: 640x512 @ 60fps (PRIMARY)
        .width  = 640,
        .height = 512,
        .max_fps = { .numerator = 60, .denominator = 1 },
        .code = MEDIA_BUS_FMT_YUYV8_1X16,
    },
    { // Mode 1: 256x192 @ 25fps
        .width  = 256,
        .height = 192,
        .max_fps = { .numerator = 25, .denominator = 1 },
        .code = MEDIA_BUS_FMT_YUYV8_1X16,
    },
    { // Mode 2: 384x288 @ 30fps
        .width  = 384,
        .height = 288,
        .max_fps = { .numerator = 30, .denominator = 1 },
        .code = MEDIA_BUS_FMT_YUYV8_1X16,
    }
};
```

**Selected via module parameter**: `mode` (0-640, 1-256, 2-384) at rs300.c:88.

### 5.2 Media Bus Format Codes (rs300.c:295-301)

```c
static const u32 codes[] = {
    MEDIA_BUS_FMT_YUYV8_1X16,  // 0x200f - PRIMARY for RP1-CFE
    MEDIA_BUS_FMT_UYVY8_1X16,  // 0x200e - Alternative
    MEDIA_BUS_FMT_YUYV8_2X8,   // 0x2007 - Legacy
    MEDIA_BUS_FMT_UYVY8_2X8,   // 0x2006 - Legacy
};
```

**RP1-CFE Compatibility Note** (rs300.c:296):
> "Pi 5 RP1-CFE only supports 16-bit packed formats (*8_1X16)"

### 5.3 Colorspace Configuration (rs300.c:441-449)

**Fixed for thermal imaging**:
```c
static void rs300_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
{
    fmt->colorspace = V4L2_COLORSPACE_SMPTE170M;     // SDTV YUV
    fmt->ycbcr_enc = V4L2_YCBCR_ENC_601;            // BT.601
    fmt->quantization = V4L2_QUANTIZATION_LIM_RANGE; // Limited range
    fmt->xfer_func = V4L2_XFER_FUNC_709;            // Standard video
}
```

**Rationale** (rs300.c:443-444):
> "Thermal data disguised as standard video YUV for ISP processing"

### 5.4 Pad Operations

#### Two Pads Defined (rs300.c:269-273):
```c
enum pad_types {
    IMAGE_PAD,      // Main video stream
    METADATA_PAD,   // Embedded metadata
    NUM_PADS
};
```

#### Format Enumeration (rs300.c:1686-1725)

**`rs300_enum_mbus_code()`**:
```c
if (code->pad == IMAGE_PAD) {
    if (code->index >= ARRAY_SIZE(supported_modes))
        return -EINVAL;
    code->code = supported_modes[code->index].code;
} else {
    if (code->index > 0)
        return -EINVAL;
    code->code = MEDIA_BUS_FMT_SENSOR_DATA;  // Metadata
}
```

#### Get Format (rs300.c:1774-1832)

**Active format** (rs300.c:1802-1815):
```c
if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
    if (fmt->pad == IMAGE_PAD) {
        fmt->format = rs300->fmt;  // Return current format
    } else if (fmt->pad == METADATA_PAD) {
        fmt->format.code = rs300->fmt.code;
        fmt->format.width = 16384;   // Standard CSI-2 metadata width
        fmt->format.height = 1;      // Single line
    }
}
```

#### Set Format (rs300.c:1850-1951)

**Format negotiation flow**:
```c
// 1. Validate format code
for (i = 0; i < ARRAY_SIZE(codes); i++)
    if (codes[i] == fmt->format.code) break;
if (i >= ARRAY_SIZE(codes))
    i = 0;  // Default to YUYV8_1X16

// 2. Find nearest supported mode
mode = v4l2_find_nearest_size(supported_modes,
                               ARRAY_SIZE(supported_modes),
                               width, height,
                               fmt->format.width, fmt->format.height);

// 3. Update format
rs300_update_image_pad_format(rs300, mode, fmt);

// 4. Update active format and pixel rate
if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
    rs300->fmt = fmt->format;
    rs300->mode = mode;

    u64 new_pixel_rate = rs300_get_pixel_rate(rs300->fmt.code);
    v4l2_ctrl_s_ctrl_int64(rs300->pixel_rate, new_pixel_rate);
}
```

### 5.5 Stream Start Sequence (rs300.c:2097-2256)

**`rs300_set_stream(enable=1)` execution flow**:

```
1. Check streaming state (avoid duplicate start)
   └─> Return 0 if already streaming

2. Set FPS (rs300.c:2139)
   └─> rs300_set_fps(rs300, fps)
       └─> Send Class 0x10, Module 0x10, SubCmd 0x46
       └─> Parameters: Enable=1, Progressive=3, FPS=fps

3. Update start registers (rs300.c:2147-2165)
   ├─> start_regs[19] = type (8 or 16-bit)
   ├─> start_regs[21] = fps
   ├─> start_regs[22-23] = width (little-endian)
   ├─> start_regs[24-25] = height (little-endian)
   └─> Calculate dual CRCs:
       ├─> CRC1 over bytes[18:28] → bytes[14:15]
       └─> CRC2 over bytes[0:16] → bytes[16:17]

4. Write start command (rs300.c:2168)
   └─> write_regs(client, 0x1d00, start_regs, 28)

5. Verify write (rs300.c:2174-2180)
   └─> read_regs() and memcmp() for verification

6. Set frame format (rs300.c:2185)
   └─> rs300_set_framefmt(rs300)
       └─> Log format selection (no hardware write)

7. Poll device ready status (rs300.c:2193-2227)
   ├─> Retry up to 10 times with 100ms delay
   ├─> Read 0x0200 status register
   ├─> Check VCMD_BUSY_STS_BIT (bit 0)
   ├─> Check VCMD_RST_STS_BIT (bit 1)
   └─> Check VCMD_ERR_STS_BIT (bits 2-7)

8. Final status check (rs300.c:2229-2239)
   └─> msleep(2000) then final status read

9. Update streaming flag
   └─> rs300->streaming = true
```

**Start Register Buffer** (rs300.c:172-186):
```c
static u8 start_regs[] = {
    0x01, 0x30, 0xc1, 0x00,    // Command header
    0x00, 0x00, 0x00, 0x00,    // Reserved
    0x00, 0x00, 0x00, 0x00,    // Reserved
    0x0a, 0x00,                // Length
    0x00, 0x00,                // CRC1 [14:15]
    0x2F, 0x0D,                // CRC2 [16:17] (gets recalculated)
    0x00,                      // Path
    0x16,                      // Source
    0x03,                      // Destination
    0x3c,                      // FPS (60 decimal)
    0x80, 0x02,                // Width 640 (LE)
    0x00, 0x02,                // Height 512 (LE)
    0x00, 0x00
};
```

### 5.6 Stream Stop Sequence (rs300.c:1983-1995)

**Simple stop**:
```c
static void rs300_stop_streaming(struct rs300 *rs300)
{
    write_regs(client, I2C_VD_BUFFER_RW, stop_regs, sizeof(stop_regs));
}
```

**Stop Register Buffer** (rs300.c:188-203):
```c
static u8 stop_regs[] = {
    0x01, 0x30, 0xc2, 0x00,    // Note: 0xc2 vs 0xc1 for stop
    // ... similar structure to start_regs
    0x01,                      // Path = 1 (different from start)
    0x16,                      // Source
    0x00,                      // Destination = 0 (vs 0x03 for start)
    // ...
};
```

---

## 6. Hardware Integration & Platform Specifics

### 6.1 Raspberry Pi 5 BCM2712 Architecture

**Device Tree Requirements** (rs300.c:2719-2757):

```c
static int rs300_check_hwcfg(struct device *dev)
{
    // Parse device tree endpoint
    endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
    v4l2_fwnode_endpoint_alloc_parse(endpoint, &ep_cfg);

    // Verify configuration
    if (ep_cfg.bus.mipi_csi2.num_data_lanes != 2)
        return -EINVAL;  // MUST be 2 lanes

    if (ep_cfg.link_frequencies[0] != RS300_LINK_RATE)
        return -EINVAL;  // MUST be 80MHz
}
```

**Required in `/boot/firmware/config.txt`**:
```
camera_auto_detect=0
dtoverlay=rs300
```

### 6.2 MIPI CSI-2 Configuration

**Link Frequency**: 80 MHz (rs300.c:43)
```c
#define RS300_LINK_RATE (80 * 1000 * 1000)
```

**Pixel Rates**:
- 8-bit formats: 200 MHz (rs300.c:44)
- 16-bit formats: 400 MHz (rs300.c:45)

**Physical Interface**:
- **Bus**: i2c-10 (I2C CSI DSI controller)
- **I2C Address**: 0x3c
- **CSI Port**: csi0 on BCM2712
- **Cable**: 22-pin to 15-pin adapter required

### 6.3 Power Management

**Three Power Supplies** (rs300.c:287-293):
```c
static const char * const rs300_supply_names[] = {
    "VANA",   // Digital I/O power
    "VDIG",   // Analog power
    "VDDL",   // Digital core power
};
```

**Power On Sequence** (rs300.c:2336-2359):
```c
static int rs300_power_on(struct device *dev)
{
    // Enable all regulators
    ret = regulator_bulk_enable(rs300_NUM_SUPPLIES, rs300->supplies);

    // Reset sequence (currently commented out)
    // gpiod_set_value_cansleep(rs300->reset_gpio, 1);  // Assert
    // msleep(100);
    // gpiod_set_value_cansleep(rs300->reset_gpio, 0);  // Release
    // msleep(500);
}
```

**Power Off** (rs300.c:2361-2372):
```c
static int rs300_power_off(struct device *dev)
{
    gpiod_set_value_cansleep(rs300->reset_gpio, 1);  // Active low reset
    regulator_bulk_disable(rs300_NUM_SUPPLIES, rs300->supplies);
}
```

### 6.4 Media Controller Pipeline Setup

**Required pipeline configuration** (from CLAUDE.md):
```bash
# Enable CSI-2 to CFE link
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set format on rs300 output pad
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 ...]"

# Set format on CSI-2 input
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 ...]"

# Set format on CSI-2 output
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 ...]"

# Set video device format
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY
```

**Critical**: RP1-CFE requires 16-bit packed formats, not 8-bit dual lane.

### 6.5 Debug Infrastructure

**Extensive dev_info/dev_err logging** throughout:
- Format negotiation (rs300.c:385-399, 1865-1929)
- Stream start/stop (rs300.c:2105-2247)
- Command execution (all command functions)
- Control operations (rs300.c:1633-1636)

**Debug function** (rs300.c:2082-2095):
```c
static void rs300_debug_pipeline_state(struct rs300 *rs300, const char *context)
{
    dev_err(&client->dev, "=== PIPELINE STATE [%s] ===", context);
    dev_err(&client->dev, "Streaming: %d", rs300->streaming);
    dev_err(&client->dev, "Active format: 0x%x (%dx%d)", ...);
    dev_err(&client->dev, "Current mode: %dx%d @ %d/%d fps", ...);
    dev_err(&client->dev, "Pixel rate for format: %llu", ...);
}
```

**Access via**:
```bash
dmesg | grep rs300
dmesg -wH  # Watch in real-time
```

---

## 7. Code Quality Assessment

### 7.1 TODO Items & Known Limitations

| Line | TODO | Status |
|------|------|--------|
| 8 | Remove unused headers | Pending |
| 87 | Make mode adjustable during runtime | Not implemented (requires driver reload) |
| 499 | Reduce repetitive code for camera command functions | Critical improvement needed |
| 1459 | Fix zoom function (link to V4L2 zoom control) | Issues with hardcoded CRC |

**Mode switching limitation** (rs300.c:86-87):
```c
// Mode must be set before running setup.sh
// TODO: Make mode adjustable during runtime
static int mode = 0; // 0-640; 1-256; 2-384
```

Currently requires:
1. Modify module parameter
2. Rebuild with `./setup.sh`
3. Reboot system


### 7.2 Code Duplication Analysis

**~500 lines of repetitive command execution code** across these functions:
- `rs300_get_brightness()` - 132 lines (rs300.c:502-633)
- `rs300_set_dde()` - 70 lines (rs300.c:635-704)
- `rs300_set_yuv_format()` - 71 lines (rs300.c:706-776)
- `rs300_set_contrast()` - 70 lines (rs300.c:778-847)
- `rs300_set_spatial_nr()` - 70 lines (rs300.c:849-918)
- `rs300_set_temporal_nr()` - 70 lines (rs300.c:920-989)
- `rs300_get_colormap()` - 88 lines (rs300.c:991-1078)
- `rs300_set_colormap()` - 133 lines (rs300.c:1081-1213)
- `rs300_shutter_cal()` - 110 lines (rs300.c:1215-1325)
- `rs300_brightness_correct()` - 129 lines (rs300.c:1327-1456)
- `rs300_set_zoom()` - 82 lines (rs300.c:1458-1539)
- `rs300_set_scene_mode()` - 84 lines (rs300.c:1541-1624)

**Common Pattern** (93% identical code):
```c
// 1. Parameter validation (5 lines)
if (value < min || value > max) {
    dev_err(...);
    return -EINVAL;
}

// 2. Command construction (13 lines)
cmd_buffer[0] = class;
cmd_buffer[1] = module;
cmd_buffer[2] = subcmd;
cmd_buffer[3] = 0x00;
cmd_buffer[4] = value;
memset(&cmd_buffer[5], 0, 11);

// 3. CRC calculation (3 lines)
crc = do_crc(cmd_buffer, 16);
cmd_buffer[16] = crc & 0xFF;
cmd_buffer[17] = (crc >> 8) & 0xFF;

// 4. Write command (4 lines)
ret = write_regs(client, 0x1d00, cmd_buffer, sizeof(cmd_buffer));
if (ret) {
    dev_err(...);
    return ret;
}

// 5. Status polling loop (30+ lines)
while (retry_count < max_retries) {
    msleep(delay);
    read_regs(client, 0x0200, status_buffer, 1);

    is_busy = status_buffer[0] & 0x01;
    has_failed = status_buffer[0] & 0x02;
    error_code = (status_buffer[0] >> 2) & 0x3F;

    if (!is_busy) {
        if (has_failed) return -EIO;
        return 0;
    }
    retry_count++;
}
return -ETIMEDOUT;
```

**Proposed Refactoring**:
```c
struct rs300_cmd {
    u8 class;
    u8 module;
    u8 subcmd;
    u8 params[12];
    int param_count;
    bool skip_crc;      // For zoom command
    u16 fixed_crc;      // For zoom command
};

static int rs300_send_command(struct rs300 *rs300,
                               const struct rs300_cmd *cmd,
                               int retry_delay_ms,
                               int max_retries)
{
    u8 cmd_buffer[18];
    u8 status_buffer[1];
    int ret, retry = 0;

    // Construct buffer
    cmd_buffer[0] = cmd->class;
    cmd_buffer[1] = cmd->module;
    cmd_buffer[2] = cmd->subcmd;
    cmd_buffer[3] = 0x00;
    memcpy(&cmd_buffer[4], cmd->params, cmd->param_count);
    memset(&cmd_buffer[4 + cmd->param_count], 0, 12 - cmd->param_count);

    // CRC
    if (cmd->skip_crc) {
        cmd_buffer[16] = cmd->fixed_crc & 0xFF;
        cmd_buffer[17] = (cmd->fixed_crc >> 8) & 0xFF;
    } else {
        u16 crc = do_crc(cmd_buffer, 16);
        cmd_buffer[16] = crc & 0xFF;
        cmd_buffer[17] = (crc >> 8) & 0xFF;
    }

    // Write + poll (common logic)
    // ... [~40 lines instead of 500]
}
```

**Estimated savings**: 500 lines → ~150 lines (70% reduction)

### 7.3 Error Handling Consistency

**Inconsistent return behavior**:

Most commands return proper error codes:
```c
if (ret) return -EIO;
if (timeout) return -ETIMEDOUT;
```

But `rs300_set_fps()` returns 0 even on failure (rs300.c:2046, 2056, 2068, 2079):
```c
// Changed from return ret
return 0;  // Non-critical, don't fail stream start
```

**Status checking varies**:
- Brightness: 200ms delay, detailed error interpretation (rs300.c:550-613)
- FFC: 1000ms delay (rs300.c:1253)
- Most others: 50ms delay
- FPS: 300ms delay, 15 retries (vs 5 for others) (rs300.c:2004, 2051)

### 7.4 Strengths

1. **Comprehensive Logging**: Extensive dev_info/dev_err for debugging
2. **Format Validation**: Robust format code checking (rs300.c:378-400)
3. **Verification**: Colormap/brightness verify GET after SET
4. **Status Interpretation**: Detailed error code mapping (rs300.c:580-604)
5. **V4L2 Compliance**: Proper subdev ops, control handler, media entity
6. **Documentation**: Inline comments explaining thermal-specific choices

### 7.5 Potential Improvements

**Priority 1 - Critical**:
- [ ] Consolidate command execution into helper function (saves 350+ lines)
- [ ] Implement runtime mode switching

**Priority 2 - High**:
- [ ] Standardize error handling (consistent return codes)
- [ ] Audit unused headers (TODO line 8)
- [ ] Fix zoom command CRC calculation (currently hardcoded)

**Priority 3 - Medium**:
- [ ] Add sysfs attributes for command statistics
- [ ] Implement regmap for register access
- [ ] Add power management (runtime PM)

**Priority 4 - Low**:
- [ ] Reduce debug verbosity (too many dev_info calls)
- [ ] Add module parameter for log level
- [ ] Implement metadata pad streaming

---

## 8. Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        User Space                            │
│  ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐│
│  │v4l2-ctl │  │media-ctl │  │  libcamera│  │  Application ││
│  └────┬────┘  └────┬─────┘  └────┬─────┘  └──────┬───────┘│
└───────┼───────────┼──────────────┼────────────────┼────────┘
        │           │              │                │
        ▼           ▼              ▼                ▼
┌────────────────────────────────────────────────────────────┐
│                    V4L2 Subsystem                          │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           /dev/video0 (rp1-cfe-csi2_ch0)             │  │
│  └─────────────────────┬────────────────────────────────┘  │
│                        │                                    │
│  ┌─────────────────────▼────────────────────────────────┐  │
│  │        /dev/v4l-subdev2 (rs300 10-003c)              │  │
│  │  ┌───────────────┐  ┌────────────────────────────┐   │  │
│  │  │ Control Ops   │  │     Pad Ops                │   │  │
│  │  │ - set_ctrl    │  │ - enum_mbus_code           │   │  │
│  │  │   (11 ctrls)  │  │ - get_fmt / set_fmt        │   │  │
│  │  └───────┬───────┘  └─────────┬──────────────────┘   │  │
│  │          │                     │                       │  │
│  │          ▼                     ▼                       │  │
│  │  ┌────────────────────────────────────────────────┐   │  │
│  │  │         rs300_set_stream()                     │   │  │
│  │  └──────────────┬─────────────────────────────────┘   │  │
│  └─────────────────┼──────────────────────────────────────┘  │
└────────────────────┼─────────────────────────────────────────┘
                     │
┌────────────────────▼─────────────────────────────────────────┐
│                    I2C Subsystem                              │
│  ┌───────────────────────────────────────────────────────┐   │
│  │         i2c-10 (i2c_csi_dsi controller)               │   │
│  │  ┌─────────────────┐  ┌──────────────────────────┐   │   │
│  │  │  read_regs()    │  │    write_regs()          │   │   │
│  │  │  - 2 msg xfer   │  │    - Single msg + data   │   │   │
│  │  └─────┬───────────┘  └───────┬──────────────────┘   │   │
│  └────────┼────────────────────────┼──────────────────────┘   │
└───────────┼────────────────────────┼──────────────────────────┘
            │                        │
┌───────────▼────────────────────────▼──────────────────────────┐
│                    RS300 Hardware                              │
│                   (I2C Address 0x3c)                           │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  Register Map                                            │ │
│  │  ┌────────────┬────────────────────────────────────────┐│ │
│  │  │ 0x1d00     │ Command/Data Buffer (R/W 256 bytes)    ││ │
│  │  ├────────────┼────────────────────────────────────────┤│ │
│  │  │ 0x0200     │ Status Register (R 1 byte)             ││ │
│  │  │            │   Bit 0: Busy                          ││ │
│  │  │            │   Bit 1: Failed                        ││ │
│  │  │            │   Bits 2-7: Error Code                 ││ │
│  │  └────────────┴────────────────────────────────────────┘│ │
│  └──────────────────────────────────────────────────────────┘ │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  Command Processing Engine                               │ │
│  │  - Class/Module/SubCmd router                            │ │
│  │  - CRC-16 validation                                     │ │
│  │  - Parameter processing                                  │ │
│  └──────────────────────────────────────────────────────────┘ │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  Thermal Sensor + ISP                                    │ │
│  │  - 640×512 microbolometer array                          │ │
│  │  - Scene modes, colormap LUTs                            │ │
│  │  - FFC calibration                                       │ │
│  │  - YUV output generation                                 │ │
│  └─────────────────────────┬────────────────────────────────┘ │
└────────────────────────────┼──────────────────────────────────┘
                             │
┌────────────────────────────▼──────────────────────────────────┐
│                   MIPI CSI-2 Interface                         │
│  - 2 data lanes                                                │
│  - 80 MHz link frequency                                       │
│  - YUV422 8-bit/16-bit                                         │
│  - 200-400 MHz pixel rate                                      │
└────────────────────────────┬──────────────────────────────────┘
                             │
┌────────────────────────────▼──────────────────────────────────┐
│              BCM2712 RP1 CSI-2 Receiver                        │
│  - csi2 subdevice                                              │
│  - rp1-cfe (Camera Front End)                                  │
│  - /dev/video0 output                                          │
└────────────────────────────────────────────────────────────────┘
```

---

## 9. Command Reference Table

| Command | Class | Module | SubCmd | Parameters | Range | Default | Function |
|---------|-------|--------|--------|------------|-------|---------|----------|
| **GET Brightness** | 0x10 | 0x04 | 0x87 | P1=0x01, P9=0x01 | - | - | rs300_get_brightness() |
| **SET Brightness** | 0x10 | 0x04 | 0x47 | P1=value | 0-100 | 50 | rs300_brightness_correct() |
| **GET Colormap** | 0x10 | 0x03 | 0x85 | P9=0x01 | - | - | rs300_get_colormap() |
| **SET Colormap** | 0x10 | 0x03 | 0x45 | P1=0x00, P2=value | 0-11 | 0 | rs300_set_colormap() |
| **FFC Trigger** | 0x10 | 0x02 | 0x43 | All zeros | - | - | rs300_shutter_cal() |
| **SET Zoom** | 0x01 | 0x31 | 0x42 | P2=value×10 | 1-8 | 1 | rs300_set_zoom() |
| **SET Scene Mode** | 0x10 | 0x04 | 0x42 | P1=value | 0-9 | 3 | rs300_set_scene_mode() |
| **SET DDE** | 0x10 | 0x04 | 0x45 | P1=value | 0-100 | 50 | rs300_set_dde() |
| **SET Contrast** | 0x10 | 0x04 | 0x4A | P1=value | 0-100 | 50 | rs300_set_contrast() |
| **SET Spatial NR** | 0x10 | 0x04 | 0x4B | P1=value | 0-100 | 50 | rs300_set_spatial_nr() |
| **SET Temporal NR** | 0x10 | 0x04 | 0x4C | P1=value | 0-100 | 50 | rs300_set_temporal_nr() |
| **SET YUV Format** | 0x10 | 0x03 | 0x4D | P1=format | 0-3 | 2 | rs300_set_yuv_format() |
| **SET FPS** | 0x10 | 0x10 | 0x46 | P1=0x01, P2=0x03, P3=fps | 25,30,50,60 | 60 | rs300_set_fps() |
| **GET Device Name** | 0x01 | 0x01 | 0x81 | P1=0x01, P9=0x20 | - | - | rs300_get_device_name() |

---

## 10. File Structure Map

```
rs300.c (2946 lines)
├─ Headers & Includes (1-37)
├─ Driver Metadata (39-50)
│  ├─ DRIVER_VERSION 0.01.01
│  ├─ RS300_LINK_RATE 80MHz
│  ├─ RS300_PIXEL_RATE 200MHz / 400MHz
│  └─ V4L2_CID_CUSTOM_BASE definition
│
├─ Menu Definitions (53-82)
│  ├─ colormap_menu[12]
│  └─ scene_mode_menu[10]
│
├─ Module Parameters (88-96)
│  ├─ mode (0=640, 1=256, 2=384)
│  ├─ fps (default 30)
│  ├─ type (8 or 16-bit)
│  └─ debug level
│
├─ Register Definitions (105-149)
│  ├─ IOCTL commands (CMD_GET, CMD_SET)
│  ├─ I2C buffer addresses
│  └─ Status bit definitions
│
├─ Helper Functions (152-267)
│  ├─ do_crc() - CRC-16 calculation
│  ├─ read_regs() - I2C read
│  ├─ write_regs() - I2C write
│  └─ Start/stop register arrays
│
├─ Data Structures (269-340)
│  ├─ enum pad_types
│  ├─ struct rs300_mode
│  ├─ struct rs300 (main driver state)
│  └─ supported_modes[3]
│
├─ Format Handling (378-489)
│  ├─ rs300_get_format_code()
│  ├─ rs300_reset_colorspace()
│  ├─ rs300_get_pixel_rate()
│  └─ rs300_set_default_format()
│
├─ Camera Command Functions (494-1624)
│  ├─ rs300_get_brightness() [502-633]
│  ├─ rs300_set_dde() [635-704]
│  ├─ rs300_set_yuv_format() [706-776]
│  ├─ rs300_set_contrast() [778-847]
│  ├─ rs300_set_spatial_nr() [849-918]
│  ├─ rs300_set_temporal_nr() [920-989]
│  ├─ rs300_get_colormap() [991-1078]
│  ├─ rs300_set_colormap() [1081-1213]
│  ├─ rs300_shutter_cal() [1215-1325]
│  ├─ rs300_brightness_correct() [1327-1456]
│  ├─ rs300_set_zoom() [1458-1539]
│  └─ rs300_set_scene_mode() [1541-1624]
│
├─ V4L2 Control Handler (1626-1684)
│  ├─ rs300_set_ctrl() - Dispatch to command functions
│  └─ rs300_ctrl_ops
│
├─ V4L2 Pad Operations (1686-1951)
│  ├─ rs300_enum_mbus_code()
│  ├─ rs300_enum_frame_sizes()
│  ├─ rs300_get_pad_fmt()
│  ├─ rs300_set_pad_fmt()
│  └─ Helper functions
│
├─ Streaming Operations (1953-2256)
│  ├─ rs300_set_framefmt()
│  ├─ rs300_stop_streaming()
│  ├─ rs300_set_fps()
│  ├─ rs300_debug_pipeline_state()
│  └─ rs300_set_stream() - Main stream control
│
├─ Initialization & Probe (2258-2893)
│  ├─ Link frequency menu
│  ├─ rs300_init_cfg()
│  ├─ rs300_open()
│  ├─ rs300_power_on/off()
│  ├─ rs300_get_regulators()
│  ├─ V4L2 ops structures
│  ├─ Control configurations (colormap, ffc, scene, etc.)
│  ├─ rs300_init_controls()
│  ├─ rs300_free_controls()
│  ├─ rs300_get_device_name()
│  ├─ rs300_check_hwcfg()
│  ├─ rs300_probe() - Main probe function
│  └─ rs300_remove()
│
├─ Driver Registration (2906-2941)
│  ├─ I2C device ID table
│  ├─ Device tree match table
│  ├─ I2C driver structure
│  └─ Module init/exit
│
└─ Module Metadata (2943-2946)
   ├─ MODULE_AUTHOR
   ├─ MODULE_DESCRIPTION
   └─ MODULE_LICENSE
```

---

## Conclusion

The RS300 V4L2 driver is a **functionally complete** implementation for thermal camera control with comprehensive I2C command support. It successfully interfaces with Raspberry Pi 5's RP1-CFE architecture using MIPI CSI-2.

**Key Strengths**:
- Complete V4L2 subdevice integration
- 11 fully implemented camera controls
- Robust I2C command protocol with CRC validation
- Extensive debug logging

**Primary Improvement Opportunities**:
1. **Code consolidation** (~500 lines of duplicated command logic)
2. **Runtime mode switching** (currently requires rebuild)
3. **Zoom command CRC** (hardcoded values)

The driver represents a solid foundation for thermal imaging applications on Raspberry Pi 5, with clear paths for optimization and feature enhancement.

---

**Document Version**: 1.0
**Generated**: 2025-10-21
**Driver Version**: 0.01.01
**Total Analysis Time**: Deep code analysis of 2,946 lines
