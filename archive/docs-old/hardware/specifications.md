# RS300 Technical Specifications

Detailed technical specifications for the RS300 "Mini2" thermal imaging camera module.

## Overview

The **RS300 Mini2** is a thermal imaging camera module designed for imaging applications only. For temperature measurement capabilities, a different module variant (referred to as "mini" - note the confusing naming scheme) is required.

**Key Specifications**:
- **NETD**: 40mK (thermal sensitivity)
- **Output**: Dual simultaneous video streams (digital + analog)
- **Interface**: MIPI CSI-2, USB 2.0, CVBS, DVP, BT656
- **Module Type**: Imaging only (no radiometric temperature measurement)

## Available Resolutions

The RS300 Mini2 is available in three resolution variants:

| Resolution | Frame Rates | USB Support | MIPI Support | Notes |
|------------|-------------|-------------|--------------|-------|
| **256×192** | 25/50 Hz | ✅ 25/50 Hz | ⚠️ Testing | 25/50Hz available over USB, MIPI troubleshooting in progress |
| **384×288** | 30/60 Hz | ✅ 30/60 Hz | ✅ 60 Hz | Full support on Pi 4B, not yet tested on Pi 5 |
| **640×512** | 30/60 Hz | ⚠️ 30 Hz only | ✅ 60 Hz | **60 Hz NOT supported over USB** - MIPI required for 60fps |

### Resolution Details

#### 640×512 (High Resolution)
- **Recommended for**: Applications requiring maximum detail
- **Frame Rates**: 30 Hz or 60 Hz
- **USB Limitation**: Maximum 30fps over USB 2.0 (bandwidth limited)
- **MIPI Advantage**: Full 60fps via MIPI CSI-2 interface
- **Testing Status**: ✅ Fully working on Pi 5 and Pi 4B

#### 384×288 (Medium Resolution)
- **Recommended for**: Balance of detail and performance
- **Frame Rates**: 30 Hz or 60 Hz
- **USB Support**: Full 60fps over USB 2.0
- **Testing Status**: ✅ Working on Pi 4B, not yet tested on Pi 5

#### 256×192 (Standard Resolution)
- **Recommended for**: Lower bandwidth applications
- **Frame Rates**: 25 Hz or 50 Hz (PAL standard)
- **USB Support**: Full 25/50fps, switchable in camera apps
- **MIPI Status**: ⚠️ Troubleshooting in progress on Pi 4B
- **Workaround**: Use USB mode for reliable 50Hz streaming

## Video Output Methods

The RS300 module supports multiple simultaneous video outputs. One digital stream and one analog stream can be active at the same time.

### MIPI CSI-2 (Digital)
**Primary interface for Raspberry Pi integration**

- **Data Lanes**: 2 lanes
- **Link Frequency**: 80 MHz
- **Pixel Rate**:
  - 8-bit mode: 200 MHz
  - 16-bit mode: 400 MHz
- **Supported Formats** (Pi 5): UYVY8_1X16, YUYV8_1X16 (16-bit packed only)
- **Supported Formats** (Pi 4): YUYV, UYVY (various media bus codes)
- **Connector**: 15-pin FPC connector (compatible with Raspberry Pi cameras)
- **Advantages**:
  - Full 60fps with 640×512 module
  - Direct integration with V4L2 subsystem
  - Low latency
  - Hardware pipeline acceleration (Pi 5)

### USB 2.0 (Digital)
**Plug-and-play UVC compliant**

- **Standard**: USB Video Class (UVC) compliant
- **Pixel Format**: YUYV422 (can be changed to UYVU with SDK)
- **Compatibility**: Works as standard webcam
- **Limitations**:
  - 640×512 module limited to 30fps (bandwidth)
  - Camera controls not exposed via UVC
  - Requires Windows/Linux SDK for control commands
- **Advantages**:
  - No driver installation required
  - Works on any platform
  - Good for testing and development
- **Control Methods**:
  - Basic streaming: Standard camera apps
  - Advanced control: Requires manufacturer SDK (Windows/Linux)

### CVBS (Analog)
**Composite video output**

- **Standards**: NTSC or PAL (selectable)
- **Simultaneous**: Can output simultaneously with USB or MIPI
- **Colormap Limitation**: Only White Hot and Black Hot available
  - All other colormaps will display but may not render correctly
- **Shared Parameters**: All digital settings affect analog output:
  - ✅ Brightness
  - ✅ Contrast
  - ✅ Noise reduction (spatial & temporal)
  - ✅ Digital detail enhancement (DDE)
  - ✅ Digital zoom
- **Use Cases**:
  - Monitoring on external display
  - Recording with analog capture devices
  - Integration with analog video systems

### DVP & BT656 (Digital - Not Available)
- **Status**: Supported by module but not broken out on RPi adapter board
- **Note**: Not researched for Raspberry Pi integration

## Pixel Formats

### MIPI CSI-2 Formats

**Raspberry Pi 5 (RP1-CFE)**:
- `UYVY8_1X16` - 16-bit packed UYVY (recommended)
- `YUYV8_1X16` - 16-bit packed YUYV
- **Important**: Pi 5 RP1-CFE **only supports 16-bit packed formats**
- 8-bit dual lane formats (`*8_2X8`) will cause "Format mismatch!" errors

**Raspberry Pi 4 (Unicam)**:
- `YUYV` / `UYVY` - Standard YUV 4:2:2 formats
- Media bus codes: Various 8-bit and 16-bit modes supported

### USB Format
- `YUYV422` - Standard YUV 4:2:2 (default)
- Can be changed to `UYVU` with manufacturer SDK

## I2C Interface

**Communication Specifications**:
- **I2C Address**: 0x3c (7-bit address)
- **Bus Speed**: Standard 100 kHz or Fast 400 kHz
- **Raspberry Pi 5**: i2c-10 (i2c_csi_dsi0 controller)
- **Raspberry Pi 4**: i2c-1
- **Protocol**: Custom 18-byte packet structure with CRC-16-CCITT
- **Commands**: 14 documented camera commands
- **Status Register**: Available for polling operation status

**See**: [I2C_PROTOCOL.md](../../I2C_PROTOCOL.md) for complete protocol documentation

## Camera Controls (V4L2)

**11 V4L2 Controls Available** via `v4l2-ctl`:

| Control | Type | Range | Default | Description |
|---------|------|-------|---------|-------------|
| `brightness` | Integer | 0-100 | 50 | Thermal brightness level |
| `contrast` | Integer | 0-100 | 50 | Image contrast |
| `zoom_absolute` | Integer | 1-8 | 1 | Digital zoom (1x-8x) |
| `colormap` | Menu | 0-11 | 0 | Color palette selection |
| `ffc_trigger` | Button | - | - | Flat field calibration trigger |
| `scene_mode` | Menu | 0-9 | 3 | Scene optimization mode |
| `digital_detail_enhancement` | Integer | 0-100 | 50 | Edge enhancement (DDE) |
| `spatial_noise_reduction` | Integer | 0-100 | 50 | Spatial noise filter |
| `temporal_noise_reduction` | Integer | 0-100 | 50 | Temporal noise filter |
| `pixel_rate` | Integer (RO) | - | 200-400 MHz | Pixel clock rate (read-only) |
| `link_freq` | Integer (RO) | - | 80 MHz | MIPI link frequency (read-only) |

### Colormap Options (0-11)
0. White Hot
1. Black Hot
2. Iron Red
3. Hot Iron
4. Medical
5. Arctic
6. Rainbow 1
7. Rainbow 2
8. Tint
9. Black & White
10. (Reserved)
11. (Reserved)

### Scene Mode Options (0-9)
0. Manual
1. Default
2. Outdoor
3. Indoor
4. Automatic Gain Control (AGC)
5. High Contrast
6. Low Contrast
7. High Brightness
8. Low Brightness
9. Custom

**See**: [DEV_QUICK_REFERENCE.md](../../DEV_QUICK_REFERENCE.md) for complete control reference and examples

## Thermal Sensitivity

**NETD: 40mK**
- NETD (Noise Equivalent Temperature Difference): 40 millikelvin
- This represents the smallest temperature difference the sensor can distinguish
- Lower NETD = better thermal sensitivity
- 40mK is suitable for most thermal imaging applications

## Module Dimensions & Optics

**Note**: Specific dimensions depend on lens configuration

**Available Lens Options**:
- 9mm lens (standard)
- 15mm lens
- 25mm lens
- Custom lens options (contact manufacturer)

**Field of View (FOV)**: Varies by lens selection

## Power Specifications

**Power Requirements**:
- **Input Voltage**: 5V (via ribbon cable or dedicated pins)
- **Current Draw**:
  - Startup: High current spike during initialization
  - Operation: Moderate continuous draw
  - FFC Calibration: Increased current during shutter operation
- **Thermal**: Module generates heat during operation (normal)

**See**: [Compatibility Matrix](compatibility.md#power-considerations) for platform-specific power issues

## Environmental Specifications

*Note: Consult manufacturer datasheets for detailed environmental specifications*

**Typical Operating Conditions**:
- Operating temperature range
- Storage temperature range
- Humidity tolerance
- Shock and vibration resistance

**Contact**: Purple River Technology or Chengen for detailed environmental specifications

## Comparison: RS300 Mini2 vs Mini

| Feature | Mini2 (RS300) | Mini |
|---------|---------------|------|
| **Purpose** | Imaging only | Radiometric (temperature measurement) |
| **Output** | Video streams | Video + temperature data |
| **Use Cases** | Thermal imaging, detection | Temperature measurement, analysis |
| **This Driver** | ✅ Supported | ❌ Not supported (different module) |

**Important**: This driver is specifically for the **Mini2 (RS300)** imaging module. The "Mini" variant with radiometric capabilities requires different software support.

---

**Related Documentation**:
- [Hardware Compatibility →](compatibility.md)
- [Purchasing Guide →](purchasing-guide.md)
- [Installation Guide →](../getting-started/)
- [Complete I2C Protocol Specification →](../../I2C_PROTOCOL.md)
