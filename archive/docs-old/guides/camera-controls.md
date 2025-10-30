# Camera Controls Reference

Complete reference for all 11 V4L2 controls available on the RS300 thermal camera.

## Quick Reference

| Control | Type | Range | Default | Description |
|---------|------|-------|---------|-------------|
| `brightness` | Integer | 0-100 | 50 | Thermal brightness/gain level |
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

---

## Accessing Controls

### Identify Subdevice

**Raspberry Pi 5**:
```bash
SUBDEV=/dev/v4l-subdev2
```

**Raspberry Pi 4**:
```bash
SUBDEV=/dev/v4l-subdev0
```

### List All Controls

```bash
v4l2-ctl -d $SUBDEV --list-ctrls
```

**Output example**:
```
                     brightness 0x00980900 (int)    : min=0 max=100 step=1 default=50 value=50
                       contrast 0x00980901 (int)    : min=0 max=100 step=1 default=50 value=50
                  zoom_absolute 0x009a090d (int)    : min=1 max=8 step=1 default=1 value=1
                       colormap 0x0098090b (menu)   : min=0 max=11 default=0 value=0
                    ffc_trigger 0x0098090c (button) : flags=write-only, execute-on-write
                     scene_mode 0x009a090a (menu)   : min=0 max=9 default=3 value=3
  digital_detail_enhancement 0x00980903 (int)    : min=0 max=100 step=1 default=50 value=50
   spatial_noise_reduction 0x00980904 (int)    : min=0 max=100 step=1 default=50 value=50
  temporal_noise_reduction 0x00980905 (int)    : min=0 max=100 step=1 default=50 value=50
                     pixel_rate 0x009f0902 (int64)  : min=0 max=0 step=0 default=200000000 value=200000000 flags=read-only
                      link_freq 0x009f0901 (intmenu): min=0 max=0 default=0 value=0 flags=read-only
```

### Get Current Value

```bash
v4l2-ctl -d $SUBDEV --get-ctrl=brightness
```

### Set Value

```bash
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=75
```

---

## Control Categories

### Adjustment Controls
Interactive controls that adjust image appearance:
- brightness
- contrast
- digital_detail_enhancement
- spatial_noise_reduction
- temporal_noise_reduction
- zoom_absolute

### Selection Controls
Choose from predefined options:
- colormap (12 options)
- scene_mode (10 options)

### Action Controls
Trigger specific camera functions:
- ffc_trigger

### Read-Only Controls
Information only, cannot be changed:
- pixel_rate
- link_freq

---

## Detailed Control Reference

### 1. Brightness

**Purpose**: Adjusts overall thermal sensitivity and gain

**Type**: Integer
**Range**: 0-100
**Default**: 50
**Units**: Percentage

**Description**:
Controls the brightness/gain level of the thermal image. Higher values increase sensitivity to subtle temperature differences, while lower values reduce sensitivity and emphasize hot spots.

**Usage**:
```bash
# Increase sensitivity (show subtle temp differences)
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=75

# Decrease sensitivity (emphasize hot spots)
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=25

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=50
```

**When to Adjust**:
- **Increase (60-80)**: Scenes with subtle temperature variations
- **Decrease (20-40)**: Scenes with large temperature differences
- **Default (50)**: General purpose imaging

**Examples**:
```bash
# Indoor inspection (subtle variations)
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=70

# Outdoor with sun exposure (large variations)
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=30
```

---

### 2. Contrast

**Purpose**: Enhances temperature boundaries and edges

**Type**: Integer
**Range**: 0-100
**Default**: 50
**Units**: Percentage

**Description**:
Adjusts the contrast of the thermal image. Higher contrast creates sharper temperature boundaries, while lower contrast produces smoother gradients.

**Usage**:
```bash
# Increase contrast (sharper boundaries)
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=75

# Decrease contrast (softer gradients)
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=25

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=50
```

**When to Adjust**:
- **Increase (60-80)**: Identify distinct temperature zones
- **Decrease (20-40)**: View smooth temperature transitions
- **Default (50)**: Balanced view

**Combination with Brightness**:
```bash
# High contrast + low brightness = emphasize extremes
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=80
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=30

# Low contrast + high brightness = view subtle details
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=30
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=70
```

---

### 3. Zoom Absolute

**Purpose**: Digital zoom (crop and magnify center)

**Type**: Integer
**Range**: 1-8
**Default**: 1 (no zoom)
**Units**: Zoom factor (1x, 2x, 3x, ..., 8x)

**Description**:
Performs digital zoom by cropping the center of the image and scaling it up. This is **digital zoom** (not optical), so resolution decreases at higher zoom levels.

**Usage**:
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

**Zoom Levels**:
| Value | Zoom | Effective Resolution (640×512) | Use Case |
|-------|------|--------------------------------|----------|
| 1 | 1x | 640×512 (full) | General use |
| 2 | 2x | 320×256 | Close inspection |
| 3 | 3x | 213×171 | Detailed area |
| 4 | 4x | 160×128 | Spot check |
| 5-8 | 5-8x | Smaller | Extreme magnification |

**Limitations**:
- Image quality decreases with higher zoom
- Cannot pan (zoom is always centered)
- Digital zoom, not optical (pixelation at high levels)

---

### 4. Colormap

**Purpose**: Select color palette for thermal visualization

**Type**: Menu
**Range**: 0-11
**Default**: 0 (White Hot)
**Options**: 12 colormaps

**Description**:
Changes the color scheme used to display temperature data. Different colormaps help emphasize different temperature ranges or provide better visual interpretation.

**Usage**:
```bash
# Set colormap
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=3

# Query current colormap
v4l2-ctl -d $SUBDEV --get-ctrl=colormap
```

**Available Colormaps**:

| Value | Name | Description | Best For |
|-------|------|-------------|----------|
| **0** | White Hot | Hot = white, cold = black | General purpose, traditional thermal |
| **1** | Black Hot | Hot = black, cold = white | Outdoor daytime, printing |
| **2** | Iron Red | Red/orange thermal look | Hot spot detection |
| **3** | Hot Iron | Enhanced thermal colors | Temperature visualization |
| **4** | Medical | Medical imaging palette | Clinical applications |
| **5** | Arctic | Blue-tinted cold emphasis | Cold spot detection |
| **6** | Rainbow 1 | High contrast rainbow | Maximum visual distinction |
| **7** | Rainbow 2 | Alternative rainbow | Different color mapping |
| **8** | Tint | Subtle color tinting | Artistic/subtle visualization |
| **9** | Black & White | Pure grayscale | Documentation, analysis |
| **10** | (Reserved) | Reserved for future use | - |
| **11** | (Reserved) | Reserved for future use | - |

**Examples by Use Case**:

**Security/Surveillance**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=1  # Black Hot (outdoor)
```

**Equipment Inspection**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=3  # Hot Iron (hot spots)
```

**Maximum Detail**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=6  # Rainbow 1 (high contrast)
```

**Documentation**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=9  # Black & White (printable)
```

**Note**: CVBS analog output only supports White Hot (0) and Black Hot (1).

---

### 5. FFC Trigger (Flat Field Calibration)

**Purpose**: Trigger sensor calibration to remove artifacts

**Type**: Button (write-only, execute-on-write)
**Range**: N/A (any write triggers)
**Default**: N/A

**Description**:
Triggers Flat Field Calibration (FFC), also known as "shutter calibration" or "NUC" (Non-Uniformity Correction). This process corrects sensor non-uniformities and removes ghosting artifacts.

**Usage**:
```bash
# Trigger FFC (value doesn't matter, any write triggers)
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
```

**What Happens**:
1. Audible "click" (internal shutter closes/opens)
2. Video freezes briefly (~1 second)
3. Image quality improves
4. Artifacts removed

**When to Use**:
- ✅ **Required**:
  - After significant temperature changes
  - When image shows ghosting or artifacts
  - First use after power-on
  - Every 5-10 minutes for best quality

- ❌ **Not needed**:
  - If image quality is good
  - During stable temperature conditions

**Best Practices**:
```bash
# Perform FFC at start of every session
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0

# Wait 2 seconds for completion
sleep 2

# Continue with imaging
```

**Automated FFC Script**:
```bash
#!/bin/bash
# auto_ffc.sh - Automatic periodic FFC

while true; do
    echo "Triggering FFC..."
    v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl=ffc_trigger=0
    sleep 600  # Every 10 minutes
done
```

---

### 6. Scene Mode

**Purpose**: Pre-configured settings optimized for different scenarios

**Type**: Menu
**Range**: 0-9
**Default**: 3 (Indoor)
**Options**: 10 scene modes

**Description**:
Selects pre-configured combinations of image processing parameters optimized for specific imaging scenarios.

**Usage**:
```bash
# Set scene mode
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=5

# Query current scene mode
v4l2-ctl -d $SUBDEV --get-ctrl=scene_mode
```

**Available Scene Modes**:

| Value | Name | Description | Best For |
|-------|------|-------------|----------|
| **0** | Manual | No automatic adjustments | Custom manual settings |
| **1** | Default | Factory default settings | General purpose |
| **2** | Outdoor | Optimized for outdoor use | Outdoor surveillance, inspection |
| **3** | Indoor | Optimized for indoor use | Indoor monitoring (default) |
| **4** | AGC | Automatic Gain Control | Dynamic temperature ranges |
| **5** | High Contrast | Enhanced contrast mode | Large temperature differences |
| **6** | Low Contrast | Reduced contrast mode | Subtle temperature variations |
| **7** | High Brightness | Increased brightness/gain | Low-light, cold scenes |
| **8** | Low Brightness | Reduced brightness/gain | Bright, hot scenes |
| **9** | Custom | User-defined preset | Save custom configuration |

**Examples by Application**:

**Outdoor Security**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=2  # Outdoor
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=1    # Black Hot
```

**Indoor Monitoring**:
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=3  # Indoor (default)
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=0    # White Hot
```

**Equipment Inspection** (large temp variations):
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=5  # High Contrast
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=3    # Hot Iron
```

**Subtle Detection** (small temp differences):
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=6  # Low Contrast
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=70
```

---

### 7. Digital Detail Enhancement (DDE)

**Purpose**: Edge enhancement and sharpening

**Type**: Integer
**Range**: 0-100
**Default**: 50
**Units**: Percentage

**Description**:
Enhances edges and details in the thermal image. Higher values increase sharpness but may introduce artifacts or noise amplification.

**Usage**:
```bash
# Increase DDE (sharper edges)
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=75

# Decrease DDE (softer image)
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=25

# Disable DDE
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=0

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=50
```

**When to Adjust**:
- **Increase (60-80)**: Inspection requiring sharp edges
- **Decrease (20-40)**: Noisy environments
- **Disable (0)**: Analysis requiring unprocessed data
- **Default (50)**: General use

**Balance with Noise Reduction**:
```bash
# Sharp image with noise control
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=70
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=60

# Smooth image
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=30
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=30
```

---

### 8. Spatial Noise Reduction

**Purpose**: Reduce random noise within each frame

**Type**: Integer
**Range**: 0-100
**Default**: 50
**Units**: Percentage

**Description**:
Applies spatial filtering to reduce random noise within individual frames. Higher values reduce noise but may decrease fine detail.

**Usage**:
```bash
# Increase noise reduction (smoother)
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=75

# Decrease noise reduction (preserve detail)
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=25

# Disable
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=0

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=50
```

**When to Adjust**:
- **Increase (60-80)**: Noisy environments, low-quality sensors
- **Decrease (20-40)**: Preserve maximum detail
- **Disable (0)**: Scientific analysis (unprocessed data)
- **Default (50)**: General use

---

### 9. Temporal Noise Reduction

**Purpose**: Reduce noise across multiple frames (motion-based)

**Type**: Integer
**Range**: 0-100
**Default**: 50
**Units**: Percentage

**Description**:
Applies temporal filtering using data from consecutive frames to reduce noise. Higher values produce smoother video but may introduce motion blur or ghosting on moving objects.

**Usage**:
```bash
# Increase temporal NR (smoother, may blur motion)
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=75

# Decrease temporal NR (preserve motion)
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=25

# Disable
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=0

# Reset to default
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=50
```

**When to Adjust**:
- **Increase (60-80)**: Static scenes, monitoring
- **Decrease (20-40)**: Fast-moving objects
- **Disable (0)**: High-motion scenarios
- **Default (50)**: General use

**Pi 5 ISP Integration**:
On Pi 5, hardware-accelerated temporal noise reduction is available via the PiSP Backend. See [RASPBERRY_PI_ISP_GUIDE.md](../reference/RASPBERRY_PI_ISP_GUIDE.md) for details.

---

### 10. Pixel Rate (Read-Only)

**Purpose**: Display pixel clock rate

**Type**: Integer64 (read-only)
**Range**: N/A
**Default**: 200,000,000 (8-bit) or 400,000,000 (16-bit)
**Units**: Hz (pixels per second)

**Description**:
Reports the current pixel rate of the camera. This is determined by the bit depth configuration.

**Usage**:
```bash
# Query pixel rate
v4l2-ctl -d $SUBDEV --get-ctrl=pixel_rate
```

**Values**:
- **200 MHz**: 8-bit mode
- **400 MHz**: 16-bit mode (default on Pi 5)

**Note**: Cannot be changed via V4L2 controls (configured at driver compile time).

---

### 11. Link Frequency (Read-Only)

**Purpose**: Display MIPI CSI-2 link frequency

**Type**: Integer Menu (read-only)
**Range**: N/A
**Default**: 80,000,000
**Units**: Hz (80 MHz)

**Description**:
Reports the MIPI CSI-2 link frequency used for data transmission between camera and SoC.

**Usage**:
```bash
# Query link frequency
v4l2-ctl -d $SUBDEV --get-ctrl=link_freq
```

**Value**: 80 MHz (fixed for RS300)

**Note**: Cannot be changed (hardware-determined).

---

## Preset Configurations

### General Purpose (Default)
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=50
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=50
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=0
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=3
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=50
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=50
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=50
v4l2-ctl -d $SUBDEV --set-ctrl=zoom_absolute=1
```

### Hot Spot Detection
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=30
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=80
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=3
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=5
v4l2-ctl -d $SUBDEV --set-ctrl=digital_detail_enhancement=60
```

### Subtle Detail Analysis
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=75
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=30
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=6
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=6
v4l2-ctl -d $SUBDEV --set-ctrl=spatial_noise_reduction=40
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=70
```

### Outdoor Surveillance
```bash
v4l2-ctl -d $SUBDEV --set-ctrl=ffc_trigger=0
v4l2-ctl -d $SUBDEV --set-ctrl=brightness=40
v4l2-ctl -d $SUBDEV --set-ctrl=contrast=60
v4l2-ctl -d $SUBDEV --set-ctrl=colormap=1
v4l2-ctl -d $SUBDEV --set-ctrl=scene_mode=2
v4l2-ctl -d $SUBDEV --set-ctrl=temporal_noise_reduction=70
```

---

## Automated Testing

Test all controls with the included script:

```bash
cd ~/rs300-v4l2-driver

# Quick test (2 minutes)
./test_controls.sh --quick

# Full test (5 minutes)
./test_controls.sh

# Test specific control
./test_controls.sh --control brightness
```

---

## Next Steps

- **[Basic Usage →](basic-usage.md)** - Streaming and recording
- **[Troubleshooting →](../reference/TROUBLESHOOTING.md)** - Debug guide
- **[Quick Reference →](../reference/DEV_QUICK_REFERENCE.md)** - Command cheat sheet

---

**See Also**:
- [I2C Protocol Documentation](../reference/I2C_PROTOCOL.md) - Low-level command details
- [Driver Analysis](../reference/DRIVER_ANALYSIS.md) - Control implementation (rs300.c:1626-1680)
