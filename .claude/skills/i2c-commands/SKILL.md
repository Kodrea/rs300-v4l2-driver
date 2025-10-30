# RS300 I2C Commands Skill

**Complete reference for formatting and validating RS300 thermal camera I2C commands**

---


## 1. When to Use This Skill

Invoke this skill when you need to:
- Format RS300 I2C command packets (18-byte structure)
- Validate command hex codes against CSV source
- Calculate or lookup CRC-16-CCITT checksums
- Construct camera control sequences
- Set up thermal camera parameters (brightness, colormap, zoom, etc.)
- Debug I2C command failures or packet structure issues

**This skill provides**:
- Complete command reference for 20+ I2C commands
- Packet structure validation rules
- CRC calculation and hardcoded lookup tables
- Multi-byte parameter encoding examples
- Working end-to-end command examples
- Common mistakes with prevention tips
- Pre-execution verification checklist

---

## 2. Command Classes

RS300 I2C commands use **two command classes**:

| Class | Purpose | Module Range | Examples |
|-------|---------|--------------|----------|
| `0x10` | Camera control | 0x02-0x10 | Brightness, Colormap, FFC, Output Mode, FPS |
| `0x01` | Device info & zoom | 0x01, 0x31 | Device Name, VID/PID, Zoom control |

---

## 3. Command Reference (Quick Summary)

**All commands organized by category - 45+ total I2C commands**

### 3.1 Shutter & Calibration Commands

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| FFC Trigger (Shutter Correction) | 0x10/0x02/0x43 | SET | Dyn | 2 |
| Background Correction | 0x01/0x10/0x52 | SET | Dyn | 3 |
| Close Shutter | 0x01/0x0F/0x45 | SET | Hard | 4 |
| Open Shutter | 0x01/0x0F/0x45 | SET | Hard | 5 |
| Autoshutter SET | 0x10/0x02/0x41 | SET | Dyn | 6-7 |
| Autoshutter GET | 0x10/0x02/0x81 | GET | Dyn | 8 |
| Autoshutter Temp SET | 0x10/0x02/0x42 | SET | Dyn | 9-10 |
| Autoshutter Interval SET | 0x10/0x02/0x42 | SET | Dyn | 11-13 |
| Autoshutter Params GET | 0x10/0x02/0x82 | GET | Dyn | 15-16 |

### 3.2 K-Value & Blind Element Calibration

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| K-Value Collect Low | 0x10/0x11/0x41 | SET | Dyn | 17 |
| K-Value Collect High | 0x10/0x11/0x41 | SET | Dyn | 18 |
| K-Value Collection & Calc | 0x10/0x11/0x41 | SET | Dyn | 19 |
| K-Value Save | 0x10/0x11/0x43 | SET | Dyn | 20 |
| K-Value Cancel | 0x10/0x11/0x42 | SET | Dyn | 21 |
| K-Value Clear | 0x10/0x11/0x44 | SET | Dyn | 22 |
| K-Value Restore Factory | 0x10/0x11/0x45 | SET | Dyn | 23 |
| Blind Element Auto Calibration | 0x10/0x11/0x51 | SET | Dyn | 24 |
| Blind Element Cursor Enable | 0x10/0x11/0x57 | SET | Dyn | 25 |
| Blind Element Cursor Disable | 0x10/0x11/0x57 | SET | Dyn | 26 |
| Blind Element Cursor GET | 0x10/0x11/0x81 | GET | Dyn | 27 |
| Blind Element Set Position | 0x10/0x11/0x58 | SET | Dyn | 28 |
| Blind Element Get Position | 0x10/0x11/0x82 | GET | Dyn | 29 |
| Blind Element Mark Bad | 0x10/0x11/0x52 | SET | Dyn | 30 |
| Blind Element Mark Good | 0x10/0x11/0x52 | SET | Dyn | 31 |
| Blind Element Cancel | 0x10/0x11/0x53 | SET | Dyn | 32 |
| Blind Element Save | 0x10/0x11/0x54 | SET | Dyn | 33 |
| Blind Element Clear | 0x10/0x11/0x55 | SET | Dyn | 34 |
| Blind Element Restore Factory | 0x10/0x11/0x56 | SET | Dyn | 35 |

### 3.3 Pot Lid Calibration

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Pot Lid Calibration | 0x10/0x11/0x61 | SET | Dyn | 36 |
| Pot Lid Save | 0x10/0x11/0x63 | SET | Dyn | 37 |
| Pot Lid Cancel | 0x10/0x11/0x62 | SET | Dyn | 38 |
| Pot Lid Clear | 0x10/0x11/0x64 | SET | Dyn | 39 |
| Pot Lid Restore Factory | 0x10/0x11/0x65 | SET | Dyn | 40 |

### 3.4 System Features

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Firmware Update | 0x01/0x01/0x42 | SET | Dyn | 41 |
| Anti-burn Protection SET | 0x10/0x03/0x4B | SET | Hard | 42-43 |
| Anti-burn Protection GET | 0x10/0x03/0x8B | GET | Hard | 44 |
| Sleep SET | 0x10/0x10/0x48 | SET | Hard | 45-46 |
| Sleep GET | 0x10/0x10/0x88 | GET | Hard | 47 |
| Boot Logo SET | 0x10/0x10/0x41 | SET | Hard | 48-49 |
| Boot Logo GET | 0x10/0x10/0x81 | GET | Hard | 50 |
| DVP/I2C Voltage SET | 0x10/0x10/0x47 | SET | Hard | 51-52 |
| DVP/I2C Voltage GET | 0x10/0x10/0x87 | GET | Hard | 53 |
| Parameter Preservation | 0x10/0x10/0x51 | SET | Dyn | 54 |
| Parameter Recovery | 0x10/0x10/0x52 | SET | Dyn | 55 |

### 3.5 Device Information

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Device Name GET | 0x01/0x01/0x81 | GET | Hard | 56 |
| FW Version GET | 0x01/0x01/0x81 | GET | Hard | 57 |
| VID GET | 0x01/0x01/0x81 | GET | Hard | 58 |
| PID GET | 0x01/0x01/0x81 | GET | Hard | 59 |
| PN GET | 0x01/0x01/0x81 | GET | Hard | 60 |
| SN GET | 0x01/0x01/0x81 | GET | Hard | 61 |
| Module Temperature GET | 0x10/0x10/0x91 | GET | Dyn | 62 |
| Hottest Spot Coordinates GET | 0x10/0x10/0x92 | GET | Dyn | 63-64 |
| Power-on Time GET | 0x10/0x10/0x93 | GET | Dyn | 65 |

### 3.6 Video Output - Digital Formats

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Digital Video Format SET | 0x10/0x10/0x46 | SET | Dyn | 66-83 |
| Digital Video Format GET | 0x10/0x10/0x86 | GET | Dyn | 84 |

### 3.7 Video Output - Analog & YUV Formats

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Analog Video Format SET | 0x10/0x10/0x4A | SET | Dyn | 85-87 |
| Analog Video Format GET | 0x10/0x10/0x8A | GET | Dyn | 88 |
| Digital-Analog Output Format | 0x10/0x10/0x49 | SET | Hard | 89 |
| Detector Frame Rate SET | 0x10/0x10/0x44 | SET | Dyn | 90-93 |
| Detector Frame Rate GET | 0x10/0x10/0x84 | GET | Dyn | 94 |
| Output Mode (IR/KBC/TNR/SNR/DDE/YUV) SET | 0x10/0x10/0x45 | SET | Hard | 95-100 |
| Output Mode GET | 0x10/0x10/0x85 | GET | Hard | 101 |
| YUV Format SET | 0x10/0x03/0x4D | SET | Dyn | 102-105 |
| YUV Format GET | 0x10/0x03/0x8C | GET | Dyn | 106 |

### 3.8 Video Output - Display Control

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Screen Freeze SET | 0x10/0x10/0x42 | SET | Dyn | 107-108 |
| Screen Freeze GET | 0x10/0x10/0x82 | GET | Dyn | 109 |
| Mirror/Flip SET | 0x10/0x10/0x43 | SET | Dyn | 110-113 |
| Mirror/Flip GET | 0x10/0x10/0x83 | GET | Dyn | 114 |
| External Sync SET | 0x10/0x10/0x4B | SET | Dyn | 115-116 |
| External Sync GET | 0x10/0x10/0x8B | GET | Dyn | 117 |

### 3.9 Zoom Control

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Electronic Zoom Center SET | 0x01/0x31/0x42 | SET | Dyn | 118-122 |
| Electronic Zoom Center GET | 0x01/0x31/0x82 | GET | Dyn | 123 |
| Electronic Zoom AOI SET | 0x01/0x31/0x51 | SET | Dyn | 124 |
| Electronic Zoom AOI GET | 0x01/0x31/0x91 | GET | Dyn | 125 |

### 3.10 Image Processing - Scene & Color

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Scene Mode SET (10 modes) | 0x10/0x04/0x42 | SET | Dyn | 126-135 |
| Scene Mode GET | 0x10/0x04/0x89 | GET | Dyn | 136 |
| False Color (Colormap) SET (12 palettes) | 0x10/0x03/0x45 | SET | Dyn | 137-148 |
| False Color GET | 0x10/0x03/0x85 | GET | Dyn | 149 |

### 3.11 Image Processing - Enhancement & Filters

| Command | Hex Code | Type | CRC | CSV Rows |
|---------|----------|------|-----|----------|
| Detail Enhancement (DDE) SET (0-100) | 0x10/0x04/0x45 | SET | Dyn | 150-160 |
| Detail Enhancement GET | 0x10/0x04/0x85 | GET | Dyn | 161 |
| Brightness SET (0-100) | 0x10/0x04/0x47 | SET | Dyn | 162-172 |
| Brightness GET | 0x10/0x04/0x87 | GET | Dyn | 173 |
| Contrast SET (0-100) | 0x10/0x04/0x4A | SET | Dyn | 174-184 |
| Contrast GET | 0x10/0x04/0x8A | GET | Dyn | 185 |
| Spatial Noise Reduction (SNR) SET (0-100) | 0x10/0x04/0x4B | SET | Dyn | 186-196 |
| Spatial Noise Reduction GET | 0x10/0x04/0x8B | GET | Dyn | 197 |
| Temporal Noise Reduction (TNR) SET (0-100) | 0x10/0x04/0x4C | SET | Dyn | 198-208 |
| Temporal Noise Reduction GET | 0x10/0x04/0x8C | GET | Dyn | 209 |
| Gamma Intensity SET (0-100) | 0x10/0x04/0x4D | SET | Dyn | 210-220 |
| Gamma Intensity GET | 0x10/0x04/0x8D | GET | Dyn | 221 |
| Hook Edge Position SET (0-2) | 0x10/0x04/0x4E | SET | Dyn | 222-224 |
| Hook Edge Position GET | 0x10/0x04/0x8E | GET | Dyn | 225 |

**Legend**:
- **CRC**: `Dyn` = Calculate using CRC-16-CCITT, `Hard` = Use hardcoded lookup table
- **Type**: `GET` = Read response, `SET` = Write command
- **CSV Rows**: References to specific rows in Mini2_I2C_full_commands.csv

---

## 4. Packet Structure (Standard 18-Byte)

All RS300 I2C commands use **exactly 18 bytes**:

```
Byte Position  Field      Description                     Example
[0]            Class      0x10 (camera) or 0x01 (device)  0x10
[1]            Module     Command module (varies)         0x04
[2]            SubCmd     Sub-command (varies)            0x47
[3]            Reserved   Always 0x00                     0x00
[4-15]         P1-P12     Parameters (varies)             [val] + 0x00...
[16]           CRC_Low    LSB of CRC-16-CCITT             0xXX
[17]           CRC_High   MSB of CRC-16-CCITT             0xXX
```

**Critical Rules**:
- ✅ Always 18 bytes total
- ✅ Byte [3] (reserved) must be 0x00
- ✅ Unused parameter bytes (P1-P12) must be 0x00
- ✅ CRC is over bytes [0-15], appended at [16-17]

---

## 5. Detailed Command Specifications

### 5.1 Output Mode SET (CRITICAL - I2C Protocol)

**Command**: Set camera output processing mode
**Hex Code**: `0x10/0x10/0x45` (Class: Camera, Module: MIPI, SubCmd: Output source)
**CSV Rows**: 95-101
**CRC Type**: Hardcoded lookup table

**Modes**:
| Mode | Value | Description | CRC (LSB, MSB) |
|------|-------|-------------|----------------|
| IR   | 0     | Raw infrared sensor output | 0xFB, 0xC0 |
| KBC  | 1     | K-based contrast enhancement | 0x8E, 0xC3 |
| TNR  | 2     | Temporal noise reduction | 0x11, 0xC6 |
| SNR  | 3     | Spatial noise reduction | 0x64, 0xC5 |
| DDE  | 4     | Digital detail enhancement | 0x2F, 0xCD |
| YUV  | 5     | YUV color output | 0x5A, 0xCE |

**Packet Structure** (18 bytes - Standard I2C):
```
[0]  = 0x10 (Class: Camera control)
[1]  = 0x10 (Module: MIPI interface)
[2]  = 0x45 (SubCmd: Output source selection)
[3]  = 0x00 (Reserved)
[4]  = mode (P1: 0=IR, 1=KBC, 2=TNR, 3=SNR, 4=DDE, 5=YUV)
[5-15] = 0x00 (Padding)
[16-17] = CRC (Hardcoded from lookup table)
```

**Example**: Set output mode to TNR (mode 2)
```
Packet: 10 10 45 00 02 00 00 00 00 00 00 00 00 00 00 00 11 C6
                    ^^                                   ^^^^^
                    P1=2 (TNR)                          CRC (hardcoded)
CSV Row: 97
```

**⚠️ IMPORTANT**: DO NOT calculate CRC for output mode - always use hardcoded values from the table above.

### 5.2 Brightness SET

**Command**: Set thermal brightness level
**Hex Code**: `0x10/0x04/0x47`
**CSV Rows**: 162-172 (11 sample values: 0, 10, 20, ..., 100)
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): Brightness value (0-100)

**Packet Structure**:
```
[0]  = 0x10
[1]  = 0x04
[2]  = 0x47
[3]  = 0x00
[4]  = brightness (0-100)
[5-15] = 0x00
[16-17] = CRC (calculated)
```

**Example**: Set brightness to 50
```
Packet: 10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]
                    ^^
                    P1=50 (0x32)
CSV Row: 167
```

### 5.3 Colormap SET

**Command**: Set color palette
**Hex Code**: `0x10/0x03/0x45`
**CSV Rows**: 137-148 (12 palettes)
**CRC Type**: Dynamic (CRC-16-CCITT)

**Note**: Unlike Brightness and Contrast (which use P1), Colormap uses **P2** (byte[5]) for the palette value. This is the camera's firmware design.

**Palettes**:
| Palette | Value | P1 | P2 |
|---------|-------|----|----|
| White Hot | 0 | 0x00 | 0x00 |
| Black Hot | 1 | 0x00 | 0x01 |
| Rainbow | 4 | 0x00 | 0x04 |
| Ironbow | 3 | 0x00 | 0x03 |
| Lava | 2 | 0x00 | 0x02 |
| Arctic | 5 | 0x00 | 0x05 |
| Wheel1 | 6 | 0x00 | 0x06 |
| Wheel2 | 7 | 0x00 | 0x07 |
| Wheel3 | 8 | 0x00 | 0x08 |
| Tyrian | 9 | 0x00 | 0x09 |
| Glory | 10 | 0x00 | 0x0A |
| EnvyGreen | 11 | 0x00 | 0x0B |

**Packet Structure**:
```
[0]  = 0x10
[1]  = 0x03
[2]  = 0x45
[3]  = 0x00
[4]  = 0x00 (P1: Always 0x00 per CSV)
[5]  = colormap (P2: 0-11)
[6-15] = 0x00
[16-17] = CRC (calculated)
```

### 5.4 Zoom SET

**Command**: Set digital zoom level
**Hex Code**: `0x01/0x31/0x42`
**CSV Rows**: 118-122 (5 levels: 1x, 2x, 3x, 4x, 8x)
**CRC Type**: Dynamic (CRC-16-CCITT)

**⚠️ Special Encoding**: Zoom level must be **multiplied by 10** before encoding.

**Zoom Levels**:
| Level | Multiply | P2 Value | Hex |
|-------|----------|----------|-----|
| 1x | 1 × 10 = 10 | 10 | 0x0A |
| 2x | 2 × 10 = 20 | 20 | 0x14 |
| 3x | 3 × 10 = 30 | 30 | 0x1E |
| 4x | 4 × 10 = 40 | 40 | 0x28 |
| 8x | 8 × 10 = 80 | 80 | 0x50 |

**Packet Structure**:
```
[0]  = 0x01
[1]  = 0x31
[2]  = 0x42
[3]  = 0x00
[4]  = 0x00 (P1: Always 0x00)
[5]  = zoom_level × 10 (P2)
[6-15] = 0x00
[16-17] = CRC (calculated)
```

**Example**: Set zoom to 2x
```
Packet: 01 31 42 00 00 14 00 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]
                       ^^
                       P2=20 (2×10=0x14)
CSV Row: 119
```

**Common Mistake**:
```
❌ WRONG: P2 = 2 (for 2x zoom) - Camera interprets as 0.2x zoom
✅ RIGHT: P2 = 20 (for 2x zoom) - Multiply by 10 first
```

### 5.5 Autoshutter Interval Parameters (Multi-Byte Example)

**Command**: Set autoshutter interval parameters
**Hex Code**: `0x10/0x02/0x42`
**CSV Rows**: 9-16
**CRC Type**: Dynamic (CRC-16-CCITT)

**Multi-Byte Parameters** (16-bit little-endian):
- P1 (byte[4]): Parameter type
  - 0x00 = Temperature threshold
  - 0x01 = Minimum interval (seconds)
  - 0x02 = Maximum interval (seconds)
- P2-P3 (bytes[5-6]): 16-bit value (little-endian)

**Example**: Set maximum interval to 360 seconds
```
Step 1: Convert to hex
  Decimal: 360
  Hex: 0x0168

Step 2: Encode as little-endian
  P2 = 0x68 (LSB)
  P3 = 0x01 (MSB)

Step 3: Construct packet
  Packet: 10 02 42 00 02 68 01 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]
                      ^^  ^^  ^^
                      P1  P2  P3
                      type LSB MSB

CSV Row: 12
```

### 5.6 FFC Trigger

**Command**: Trigger flat field calibration
**Hex Code**: `0x10/0x02/0x43`
**CSV Row**: 2
**CRC Type**: Dynamic (CRC-16-CCITT)
**Timeout**: 5000ms (5 seconds - longest command)

**Parameters**: All zeros (no parameters)

**Packet Structure**:
```
[0]  = 0x10
[1]  = 0x02
[2]  = 0x43
[3-15] = 0x00 (All zeros)
[16-17] = CRC (calculated)
```

### 5.7 Device Name GET (Hardcoded CRC Example)

**Command**: Get camera device name (40-byte ASCII string)
**Hex Code**: `0x01/0x01/0x81`
**CSV Row**: 56
**CRC Type**: Hardcoded (0xFC, 0x1E)
**Response**: 40 bytes read from register 0x1d00

**Parameters**:
- P1 (byte[4]): 0x01 (Device name selector)
- P12 (byte[15]): 0x20 (32 decimal = expected response length)

**Packet Structure**:
```
[0]  = 0x01
[1]  = 0x01
[2]  = 0x81
[3]  = 0x00
[4]  = 0x01 (P1: Device name)
[5-14] = 0x00
[15] = 0x20 (P12: Response length)
[16] = 0xFC (CRC LSB - hardcoded)
[17] = 0x1E (CRC MSB - hardcoded)
```

**Complete Packet**:
```
01 01 81 00 01 00 00 00 00 00 00 00 20 00 00 00 FC 1E
```

### 5.8 Open/Close Shutter

**Command**: Control thermal camera shutter (open = accept thermal input, close = block thermal input)
**Hex Code**: `0x01/0x0F/0x45` (Class: Device, Module: Shutter, SubCmd: Shutter control)
**CSV Rows**: 4-5

**Operations**:
| Operation | P1 Value | CRC (LSB, MSB) | Purpose |
|-----------|----------|----------------|---------|
| Close | 0x00 | 0x8D, 0x5A | Block thermal input (shutter closed) |
| Open | 0x01 | 0xF8, 0x59 | Accept thermal input (shutter open) |

**Note**: Both operations use hardcoded CRC values - do NOT calculate dynamically.

**Packet Structure** (18 bytes):
```
[0]  = 0x01 (Class: Device)
[1]  = 0x0F (Module: Shutter)
[2]  = 0x45 (SubCmd: Shutter control)
[3]  = 0x00 (Reserved)
[4]  = operation (0=close, 1=open)
[5-15] = 0x00 (Padding)
[16-17] = CRC (Hardcoded lookup)
```

**Example**: Open shutter
```
Packet: 01 0F 45 00 01 00 00 00 00 00 00 00 00 00 00 00 F8 59
                    ^^                                   ^^^^^
                    P1=1 (Open)                         CRC (hardcoded)
CSV Row: 5
```

### 5.9 Anti-burn Protection

**Command**: Enable/disable camera anti-burn protection (protects sensor from bright light damage)
**Hex Code**: `0x10/0x03/0x4B` (SET), `0x10/0x03/0x8B` (GET)
**CSV Rows**: 42-44

**Settings**:
| Setting | Value | CRC (LSB, MSB) | Purpose |
|---------|-------|----------------|---------|
| OFF | 0x00 | 0x58, 0x8D | Disable anti-burn protection |
| ON | 0x01 | 0x2D, 0x8E | Enable anti-burn protection |

**GET Parameters**:
- P1: 0x00 (selector)
- P12: 0x01 (response length = 1 byte)
- CRC: 0x2D, 0x87 (hardcoded)

**Packet Structure** (SET):
```
[0]  = 0x10 (Class: Camera)
[1]  = 0x03 (Module: Display/Protection)
[2]  = 0x4B (SubCmd: Anti-burn SET)
[3]  = 0x00 (Reserved)
[4]  = setting (0=OFF, 1=ON)
[5-15] = 0x00 (Padding)
[16-17] = CRC (Hardcoded lookup)
```

**Example**: Enable anti-burn protection
```
Packet: 10 03 4B 00 01 00 00 00 00 00 00 00 00 00 00 00 2D 8E
                    ^^                                   ^^^^^
                    P1=1 (ON)                           CRC (hardcoded)
CSV Row: 43
```

### 5.10 Hook Edge Position

**Command**: Set edge enhancement hook position (controls where edge enhancement is applied)
**Hex Code**: `0x10/0x04/0x4E` (SET), `0x10/0x04/0x8E` (GET)
**CSV Rows**: 222-225
**CRC Type**: Dynamic (CRC-16-CCITT)

**Positions**:
| Position | Value | Description |
|----------|-------|-------------|
| 0 | 0x00 | No edge hook |
| 1st Gear | 0x01 | First edge level |
| 2 Levels | 0x02 | Two edge levels |

**GET Parameters**:
- P1: 0x01 (selector)
- P12: 0x01 (response length = 1 byte)

**Packet Structure** (SET):
```
[0]  = 0x10 (Class: Camera)
[1]  = 0x04 (Module: Image Processing)
[2]  = 0x4E (SubCmd: Hook edge SET)
[3]  = 0x00 (Reserved)
[4]  = position (0, 1, or 2)
[5-15] = 0x00 (Padding)
[16-17] = CRC (Calculate using CRC-16-CCITT)
```

**Example**: Set hook edge position to 1st gear
```
Step 1: Construct packet
[0-2]   = 10 04 4E
[3]     = 00 (Reserved)
[4]     = 01 (P1: 1st Gear)
[5-15]  = 00 00 00 00 00 00 00 00 00 00 00

Step 2: Calculate CRC
uint16_t crc = calculate_crc(cmd, 16);

Step 3: Final packet (18 bytes)
10 04 4E 00 01 00 00 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]

CSV Row: 223
```

### 5.11 Detector Frame Rate

**Command**: Set thermal detector frame rate (frames per second)
**Hex Code**: `0x10/0x10/0x44` (SET), `0x10/0x10/0x84` (GET)
**CSV Rows**: 90-94
**CRC Type**: Dynamic (CRC-16-CCITT)

**Frame Rate Options**:
| Rate | P1 Value | Hex | CSV Row |
|------|----------|-----|---------|
| 30 Hz | 0x1E | 30 | 90 |
| 60 Hz | 0x3C | 60 | 91 |
| 25 Hz | 0x19 | 25 | 92 |
| 50 Hz | 0x32 | 50 | 93 |

**GET Parameters**:
- P1: 0x00 (selector)
- P12: 0x01 (response length = 1 byte)

**Packet Structure** (SET):
```
[0]  = 0x10 (Class: Camera)
[1]  = 0x10 (Module: MIPI/Timing)
[2]  = 0x44 (SubCmd: Frame rate SET)
[3]  = 0x00 (Reserved)
[4]  = frame_rate (0x1E/0x3C/0x19/0x32)
[5-15] = 0x00 (Padding)
[16-17] = CRC (Calculate using CRC-16-CCITT)
```

**Example**: Set frame rate to 60 Hz
```
Step 1: Construct packet
[0-2]   = 10 10 44
[3]     = 00 (Reserved)
[4]     = 3C (P1: 60 Hz)
[5-15]  = 00 00 00 00 00 00 00 00 00 00 00

Step 2: Calculate CRC
uint16_t crc = calculate_crc(cmd, 16);

Step 3: Final packet (18 bytes)
10 10 44 00 3C 00 00 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]

CSV Row: 91
```

### 5.12 Digital-Analog Output Format

**Command**: Save/apply digital-analog output format configuration
**Hex Code**: `0x10/0x10/0x49`
**CSV Row**: 89
**CRC Type**: Hardcoded (0x35, 0xD6)

**Description**: Configuration command to save or apply the current digital-analog output format settings. Parameters are all zero (no configuration parameters).

**Packet Structure**:
```
[0]  = 0x10 (Class: Camera)
[1]  = 0x10 (Module: MIPI/Output)
[2]  = 0x49 (SubCmd: Digital-analog format)
[3-15] = 0x00 (All parameters zero)
[16] = 0x35 (CRC LSB - hardcoded)
[17] = 0xD6 (CRC MSB - hardcoded)
```

**Complete Packet**:
```
10 10 49 00 00 00 00 00 00 00 00 00 00 00 00 00 35 D6
```

### 5.13 Shutter Correction

**Command**: Shutter calibration (FFC - Flat Field Correction without auto-retry)
**Hex Code**: `0x10/0x02/0x43`
**CSV Row**: 2
**CRC Type**: Dynamic (CRC-16-CCITT)

**Description**: Performs thermal shutter calibration for sensor correction. This is the base FFC command without automatic retry logic.

**Packet Structure**:
```
[0]  = 0x10 (Class: Camera)
[1]  = 0x02 (Module: Shutter)
[2]  = 0x43 (SubCmd: Shutter correction)
[3-15] = 0x00 (All zeros)
[16-17] = CRC (calculated)
```

### 5.14 Background Correction

**Command**: Background/offset correction for thermal reference
**Hex Code**: `0x01/0x10/0x52`
**CSV Row**: 3
**CRC Type**: Dynamic (CRC-16-CCITT)

**Description**: Applies background offset correction to thermal measurements.

**Packet Structure**:
```
[0]  = 0x01 (Class: Device)
[1]  = 0x10 (Module: Background)
[2]  = 0x52 (SubCmd: Background correction)
[3-15] = 0x00 (All zeros)
[16-17] = CRC (calculated)
```

### 5.15 Autoshutter Temperature Parameter

**Command**: Set autoshutter temperature threshold
**Hex Code**: `0x10/0x02/0x42`
**CSV Rows**: 9-10
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x00 (temperature threshold type)
- P2 (byte[5]): Temperature value (0-255)

**Options**:
| Setting | Temperature | P2 Value | CSV Row |
|---------|-------------|----------|---------|
| 10°C | 10 | 0x0A | 9 |
| 50°C | 50 | 0x32 | 10 |

### 5.16 Autoshutter Minimum Interval Time

**Command**: Set minimum autoshutter interval (seconds)
**Hex Code**: `0x10/0x02/0x42`
**CSV Rows**: 11-12
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x01 (minimum interval type)
- P2-P3 (bytes[5-6]): 16-bit value (little-endian, seconds)

**Options**:
| Setting | Value | P2 | P3 | CSV Row |
|---------|-------|----|----|---------|
| 1 second | 1 | 0x01 | 0x00 | 11 |
| 120 seconds | 120 | 0x78 | 0x00 | 12 |

### 5.17 Autoshutter Maximum Interval Time

**Command**: Set maximum autoshutter interval (seconds)
**Hex Code**: `0x10/0x02/0x42`
**CSV Rows**: 13-14
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x02 (maximum interval type)
- P2-P3 (bytes[5-6]): 16-bit value (little-endian, seconds)

**Options**:
| Setting | Value | P2 | P3 | CSV Row |
|---------|-------|----|----|---------|
| 120 seconds | 120 | 0x78 | 0x00 | 13 |
| 360 seconds | 360 | 0x68 | 0x01 | 14 |

### 5.18 Autoshutter Get Temperature

**Command**: Get current autoshutter temperature threshold
**Hex Code**: `0x10/0x02/0x82`
**CSV Row**: 15
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x00 (temperature selector)
- P12 (byte[15]): 0x02 (response length = 2 bytes)

### 5.19 Autoshutter Get Minimum Interval

**Command**: Get current autoshutter minimum interval
**Hex Code**: `0x10/0x02/0x82`
**CSV Row**: 16
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x01 (minimum interval selector)
- P12 (byte[15]): 0x02 (response length = 2 bytes)

### 5.20 Calibration K Value Commands

**Command Series**: K-value calibration sequence
**Hex Code**: `0x10/0x11/0x41` (collect), `0x10/0x11/0x42` (cancel), `0x10/0x11/0x43` (save), `0x10/0x11/0x44` (clear), `0x10/0x11/0x45` (restore)
**CSV Rows**: 17-23
**CRC Type**: Dynamic (CRC-16-CCITT)

**Operations**:
| Operation | Hex Code | P1 | Purpose | CSV Row |
|-----------|----------|----|---------|---------|
| Collect Low Temperature | 0x10/0x11/0x41 | 0x00 | Capture low reference point | 17 |
| Collect High Temperature | 0x10/0x11/0x41 | 0x01 | Capture high reference point | 18 |
| Collection & Calculation | 0x10/0x11/0x41 | 0x02 | Process calibration data | 19 |
| Save K Value | 0x10/0x11/0x43 | 0x00 | Persist calibration | 20 |
| Cancel Calibration | 0x10/0x11/0x42 | 0x00 | Discard calibration | 21 |
| Clear K Value | 0x10/0x11/0x44 | 0x00 | Clear stored calibration | 22 |
| Restore Factory K Value | 0x10/0x11/0x45 | 0x00 | Restore default calibration | 23 |

### 5.21 Blind Element (Defective Pixel) Calibration

**Command Series**: Blind element (dead pixel) calibration
**Hex Code**: `0x10/0x11/0x51` through `0x10/0x11/0x56`
**CSV Rows**: 24-35
**CRC Type**: Dynamic (CRC-16-CCITT)

**Key Operations**:
| Operation | Hex Code | Purpose | CSV Rows |
|-----------|----------|---------|----------|
| Automatic blind calibration | 0x10/0x11/0x51 | Auto-detect bad pixels | 24 |
| Cursor switch enable | 0x10/0x11/0x57 P1=0x01 | Enable cursor marking | 25 |
| Cursor switch disable (blanking) | 0x10/0x11/0x57 P1=0x00 | Disable cursor | 26 |
| Cursor switch acquisition | 0x10/0x11/0x81 | Query cursor state | 27 |
| Cursor position setting | 0x10/0x11/0x58 | Set cursor XY coordinates | 28 |
| Get cursor position | 0x10/0x11/0x82 | Read cursor location | 29 |
| Set blind pixel (manual) | 0x10/0x11/0x52 P1=0x00 | Mark pixel as blind | 30 |
| Set non-blind pixel (manual) | 0x10/0x11/0x52 P1=0x01 | Mark pixel as good | 31 |
| Cancel calibration | 0x10/0x11/0x53 | Discard blind calibration | 32 |
| Save blind data | 0x10/0x11/0x54 | Persist calibration | 33 |
| Clear blind data | 0x10/0x11/0x55 | Clear stored data | 34 |
| Restore factory blind data | 0x10/0x11/0x56 | Reset to defaults | 35 |

**Note**: Cursor position uses multi-byte XY encoding:
- P2-P3: X coordinate (16-bit little-endian)
- P4-P5: Y coordinate (16-bit little-endian)

### 5.22 Pot Lid (Shutter) Calibration

**Command Series**: Thermal shutter lid calibration
**Hex Code**: `0x10/0x11/0x61` through `0x10/0x11/0x65`
**CSV Rows**: 36-40
**CRC Type**: Dynamic (CRC-16-CCITT)

**Operations**:
| Operation | Hex Code | Purpose | CSV Row |
|-----------|----------|---------|---------|
| Calibration lid | 0x10/0x11/0x61 | Start lid calibration | 36 |
| Save lid | 0x10/0x11/0x63 | Store calibration data | 37 |
| Cancel results | 0x10/0x11/0x62 | Discard calibration | 38 |
| Empty the lid | 0x10/0x11/0x64 | Clear lid data | 39 |
| Restore factory data | 0x10/0x11/0x65 | Reset to defaults | 40 |

### 5.23 System Features - Firmware Update

**Command**: Initiate firmware update mode
**Hex Code**: `0x01/0x01/0x42`
**CSV Row**: 41
**CRC Type**: Dynamic (CRC-16-CCITT)

**Description**: Prepares camera for firmware update via I2C.

### 5.24 System Features - Sleep Control

**Command**: Control camera module sleep state
**Hex Code**: `0x10/0x10/0x48` (SET), `0x10/0x10/0x88` (GET)
**CSV Rows**: 45-47
**CRC Type**: Dynamic (CRC-16-CCITT)

**Operations**:
| Operation | P1 Value | CSV Row |
|-----------|----------|---------|
| Wake up (wake) | 0x00 | 45 |
| Sleep (sleep) | 0x01 | 46 |
| Get state | 0x00 (P12=0x01) | 47 |

### 5.25 System Features - Boot Logo

**Command**: Control boot logo display
**Hex Code**: `0x10/0x10/0x41` (SET), `0x10/0x10/0x81` (GET)
**CSV Rows**: 48-50
**CRC Type**: Dynamic (CRC-16-CCITT)

**Operations**:
| Operation | P1 Value | CSV Row |
|-----------|----------|---------|
| Disable logo | 0x00 | 48 |
| Enable logo | 0x01 | 49 |
| Get state | 0x00 (P12=0x01) | 50 |

### 5.26 System Features - DVP/I2C Voltage Switching

**Command**: Switch between DVP and I2C communication voltage levels
**Hex Code**: `0x10/0x10/0x47` (SET), `0x10/0x10/0x87` (GET)
**CSV Rows**: 51-53
**CRC Type**: Dynamic (CRC-16-CCITT)

**Voltage Options**:
| Voltage | P1 Value | CSV Row |
|---------|----------|---------|
| 1.8V | 0x00 | 51 |
| 3.3V | 0x01 | 52 |
| Get voltage | 0x00 (P12=0x01) | 53 |

### 5.27 System Features - Parameter Save/Restore

**Command**: Manage parameter persistence
**Hex Code**: `0x10/0x10/0x51` (preserve), `0x10/0x10/0x52` (recover)
**CSV Rows**: 54-55
**CRC Type**: Dynamic (CRC-16-CCITT)

**Operations**:
| Operation | Hex Code | Purpose | CSV Row |
|-----------|----------|---------|---------|
| Parameter preservation | 0x10/0x10/0x51 | Save current settings | 54 |
| Parameter recovery | 0x10/0x10/0x52 | Load saved settings | 55 |

### 5.28 Device Information - Module Temperature & Environment

**Command**: Get module/environmental data
**Hex Code**: `0x10/0x10/0x91` (temperature), `0x10/0x10/0x92` (hot spot coords), `0x10/0x10/0x93` (power-on time)
**CSV Rows**: 62-65
**CRC Type**: Dynamic (CRC-16-CCITT)

**Commands**:
| Command | Hex Code | Response | CSV Row |
|---------|----------|----------|---------|
| Get module temperature | 0x10/0x10/0x91 | 2 bytes (temp) | 62 |
| Get hottest spot coordinates | 0x10/0x10/0x92 | 4 bytes (X,Y) | 63-64 |
| Get power-on time | 0x10/0x10/0x93 | 4 bytes (seconds) | 65 |

**Note**: Hottest spot commands include area of interest parameters (AOI bounds).

### 5.29 Video Output - Digital Format Commands

**Command**: Configure digital video output format (USB, DVP, BT656, MIPI)
**Hex Code**: `0x10/0x10/0x46`
**CSV Rows**: 66-83
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x01 (enable) or 0x00 (disable output)
- P2 (byte[5]): Interface type (0=USB, 1=DVP, 2=BT656, 3=MIPI)
- P3 (byte[6]): Frame rate (0x1E=30Hz, 0x3C=60Hz, 0x19=25Hz, 0x32=50Hz)

**Format Combinations**:
| Format | P1 | P2 | P3 | Frame Rate | CSV Rows |
|--------|----|----|----|-----------|---------:|
| USB-Progressive | 0x01 | 0x00 | 0x1E | 30Hz | 66 |
| USB-Progressive | 0x01 | 0x00 | 0x3C | 60Hz | 67 |
| DVP-Progressive | 0x01 | 0x01 | 0x1E | 30Hz | 68 |
| DVP-Progressive | 0x01 | 0x01 | 0x3C | 60Hz | 69 |
| BT656-Progressive | 0x01 | 0x02 | 0x1E | 30Hz | 70 |
| BT656-Progressive | 0x01 | 0x02 | 0x3C | 60Hz | 71 |
| BT656-Interlaced | 0x01 | 0x02 | 0x12 | 25Hz | 72 |
| MIPI-Progressive | 0x01 | 0x03 | 0x1E | 30Hz | 73 |
| MIPI-Progressive | 0x01 | 0x03 | 0x3C | 60Hz | 74 |
| (Mini2 256 variants) | ... | ... | ... | 25/50Hz | 75-82 |
| Output disable | 0x00 | 0x00 | 0x00 | N/A | 83 |

### 5.30 Video Output - Get Format Configuration

**Command**: Query current digital output format
**Hex Code**: `0x10/0x10/0x86`
**CSV Row**: 84
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x00 (selector)
- P12 (byte[15]): 0x04 (response = 4 bytes)

**Response**: Interface, frame rate, and status bytes

### 5.31 Video Output - Analog Format Commands

**Command**: Control analog video output (NTSC/PAL)
**Hex Code**: `0x10/0x10/0x4A` (SET), `0x10/0x10/0x8A` (GET)
**CSV Rows**: 85-88
**CRC Type**: Dynamic (CRC-16-CCITT)

**Options**:
| Option | P1 Value | P2 Value | CSV Row |
|--------|----------|----------|---------|
| Enable NTSC | 0x01 | 0x00 | 85 |
| Enable PAL | 0x01 | 0x01 | 86 |
| Disable analog | 0x00 | 0x00 | 87 |
| Get status | 0x00 (P12=0x02) | N/A | 88 |

### 5.32 Video Output - YUV Format Selection

**Command**: Select YUV color component ordering
**Hex Code**: `0x10/0x03/0x4D` (SET), `0x10/0x03/0x8C` (GET)
**CSV Rows**: 102-106
**CRC Type**: Dynamic (CRC-16-CCITT)

**YUV Formats**:
| Format | P1 Value | Description | CSV Row |
|--------|----------|-------------|---------|
| UYVY | 0x00 | U-Y-V-Y ordering | 102 |
| VYUY | 0x01 | V-Y-U-Y ordering | 103 |
| YUYV | 0x02 | Y-U-Y-V ordering | 104 |
| YVYU | 0x03 | Y-V-Y-U ordering | 105 |
| Get status | 0x00 (P12=0x01) | N/A | 106 |

### 5.33 Video Output - Screen Freeze Control

**Command**: Freeze/unfreeze video output display
**Hex Code**: `0x10/0x10/0x42` (SET), `0x10/0x10/0x82` (GET)
**CSV Rows**: 107-109
**CRC Type**: Dynamic (CRC-16-CCITT)

**Options**:
| Option | P1 Value | CSV Row |
|--------|----------|---------|
| Disable freeze | 0x00 | 107 |
| Enable freeze | 0x01 | 108 |
| Get state | 0x00 (P12=0x01) | 109 |

### 5.34 Video Output - Mirror/Flip Control

**Command**: Control image orientation (horizontal/vertical flip)
**Hex Code**: `0x10/0x10/0x43` (SET), `0x10/0x10/0x83` (GET)
**CSV Rows**: 110-114
**CRC Type**: Dynamic (CRC-16-CCITT)

**Flip Modes**:
| Mode | P1 Value | Description | CSV Row |
|------|----------|-------------|---------|
| No flip | 0x00 | Original orientation | 110 |
| Flip left-right | 0x01 | Mirror horizontally | 111 |
| Flip upside down | 0x02 | Mirror vertically | 112 |
| Both (L-R + U-D) | 0x03 | 180° rotation | 113 |
| Get state | 0x00 (P12=0x01) | N/A | 114 |

### 5.35 Video Output - External Synchronization Mode

**Command**: Enable external sync for multi-camera setups
**Hex Code**: `0x10/0x10/0x4B` (SET), `0x10/0x10/0x8B` (GET)
**CSV Rows**: 115-117
**CRC Type**: Dynamic (CRC-16-CCITT)

**Options**:
| Option | P1 Value | CSV Row |
|--------|----------|---------|
| Enable external sync | 0x01 | 115 |
| Disable external sync | 0x00 | 116 |
| Get state | 0x00 (P12=0x01) | 117 |

### 5.36 Electronic Zoom - Center Position

**Command**: Center-point digital zoom (1x, 2x, 3x, 4x, 8x)
**Hex Code**: `0x01/0x31/0x42` (SET), `0x01/0x31/0x82` (GET)
**CSV Rows**: 118-123
**CRC Type**: Dynamic (CRC-16-CCITT)

**Zoom Levels** (note: P2 = zoom × 10):
| Zoom Level | P2 Value | Description | CSV Row |
|------------|----------|-------------|---------|
| 1x (no zoom) | 0x0A | 10 decimal | 118 |
| 2x | 0x14 | 20 decimal | 119 |
| 3x | 0x1E | 30 decimal | 120 |
| 4x | 0x28 | 40 decimal | 121 |
| 8x | 0x50 | 80 decimal | 122 |
| Get zoom | 0x00 (P12=0x01) | N/A | 123 |

### 5.37 Electronic Zoom - Area of Interest (Coordinates)

**Command**: Zoom to specific image coordinates with custom zoom level
**Hex Code**: `0x01/0x31/0x51` (SET), `0x01/0x31/0x91` (GET)
**CSV Rows**: 124-125
**CRC Type**: Dynamic (CRC-16-CCITT)

**Parameters**:
- P1 (byte[4]): 0x00 (reserved)
- P2-P3 (bytes[5-6]): X coordinate (16-bit little-endian)
- P4-P5 (bytes[7-8]): Y coordinate (16-bit little-endian)
- P6-P7 (bytes[9-10]): Zoom factor × 100 (16-bit little-endian)

**Example** (CSV Row 124): Position (300, 280), 2.1× zoom
- P2-P3: 0x2C 0x01 (300)
- P4-P5: 0x18 0x01 (280)
- P6-P7: 0x15 0x00 (21 = 2.1 × 10)

**GET Response**: 5 bytes (X LSB, X MSB, Y LSB, Y MSB, zoom)

### 5.38 Scene Mode - All Modes

**Command**: Apply scene optimization preset
**Hex Code**: `0x10/0x04/0x42` (SET), `0x10/0x04/0x89` (GET)
**CSV Rows**: 126-136
**CRC Type**: Dynamic (CRC-16-CCITT)

**Scene Modes**:
| Mode | P1 Value | Description | CSV Row |
|------|----------|-------------|---------|
| Low Temperature Protrusion | 0x00 | Enhanced low temp | 126 |
| Linear Stretch | 0x01 | Linear histogram | 127 |
| Low Contrast | 0x02 | Dark scenes | 128 |
| General Mode (Default) | 0x03 | Balanced | 129 |
| High Contrast | 0x04 | Bright scenes | 130 |
| Highlight | 0x05 | Extreme highlights | 131 |
| Reserve 1 | 0x06 | Reserved | 132 |
| Reserve 2 | 0x07 | Reserved | 133 |
| Reserve 3 | 0x08 | Reserved | 134 |
| Outline Mode | 0x09 | Edge detection | 135 |
| Get mode | 0x00 (P12=0x01) | N/A | 136 |

### 5.39 False Color (Colormap) - Extended Reference

**Command**: Select false color palette (12 options total)
**Hex Code**: `0x10/0x03/0x45` (SET), `0x10/0x03/0x85` (GET)
**CSV Rows**: 137-149
**CRC Type**: Dynamic (CRC-16-CCITT)

**All Palettes** (P2 parameter):
| Palette | P1 | P2 | Description | CSV Row |
|---------|----|----|-------------|---------|
| White Hot | 0x00 | 0x00 | White to black gradient | 137 |
| Reserved | 0x00 | 0x01 | Reserved palette | 138 |
| Gold Sepia | 0x00 | 0x02 | Gold to brown | 139 |
| Ironbow | 0x00 | 0x03 | Iron to rainbow | 140 |
| Rainbow | 0x00 | 0x04 | Full spectrum | 141 |
| Night | 0x00 | 0x05 | Night vision green | 142 |
| Aurora | 0x00 | 0x06 | Aurora (purple-green) | 143 |
| Red_Hot | 0x00 | 0x07 | Red hot metal | 144 |
| Jungle | 0x00 | 0x08 | Green jungle | 145 |
| Medical | 0x00 | 0x09 | Medical blue-red | 146 |
| Black_Hot | 0x00 | 0x0A | Black to white | 147 |
| Glory Hot | 0x00 | 0x0B | Golden glory | 148 |
| Get palette | 0x00 | N/A (P12=0x01) | N/A | 149 |

### 5.40 Detail Enhancement (DDE) - All Levels

**Command**: Digital detail enhancement (0-100 in 10% steps)
**Hex Code**: `0x10/0x04/0x45` (SET), `0x10/0x04/0x85` (GET)
**CSV Rows**: 150-161
**CRC Type**: Dynamic (CRC-16-CCITT)

**Enhancement Levels** (P1 = 0-100, step 10):
| Level | P1 Value | CSV Row |
|-------|----------|---------|
| 0 | 0x00 | 150 |
| 10 | 0x0A | 151 |
| 20 | 0x14 | 152 |
| 30 | 0x1E | 153 |
| 40 | 0x28 | 154 |
| 50 | 0x32 | 155 |
| 60 | 0x3C | 156 |
| 70 | 0x46 | 157 |
| 80 | 0x50 | 158 |
| 90 | 0x5A | 159 |
| 100 | 0x64 | 160 |
| Get level | 0x00 (P12=0x01) | 161 |

### 5.41 Contrast - All Levels

**Command**: Image contrast adjustment (0-100 in 10% steps)
**Hex Code**: `0x10/0x04/0x4A` (SET), `0x10/0x04/0x8A` (GET)
**CSV Rows**: 174-185
**CRC Type**: Dynamic (CRC-16-CCITT)

**Contrast Levels** (P1 = 0-100, step 10):
| Level | P1 Value | CSV Row |
|-------|----------|---------|
| 0 | 0x00 | 174 |
| 10 | 0x0A | 175 |
| 20 | 0x14 | 176 |
| 30 | 0x1E | 177 |
| 40 | 0x28 | 178 |
| 50 | 0x32 | 179 |
| 60 | 0x3C | 180 |
| 70 | 0x46 | 181 |
| 80 | 0x50 | 182 |
| 90 | 0x5A | 183 |
| 100 | 0x64 | 184 |
| Get level | 0x00 (P12=0x01) | 185 |

### 5.42 Spatial Noise Reduction (SNR) - All Levels

**Command**: Spatial/airspace noise reduction (0-100 in 10% steps)
**Hex Code**: `0x10/0x04/0x4B` (SET), `0x10/0x04/0x8B` (GET)
**CSV Rows**: 186-197
**CRC Type**: Dynamic (CRC-16-CCITT)

**Noise Reduction Levels** (P1 = 0-100, step 10):
| Level | P1 Value | CSV Row |
|-------|----------|---------|
| 0 | 0x00 | 186 |
| 10 | 0x0A | 187 |
| 20 | 0x14 | 188 |
| 30 | 0x1E | 189 |
| 40 | 0x28 | 190 |
| 50 | 0x32 | 191 |
| 60 | 0x3C | 192 |
| 70 | 0x46 | 193 |
| 80 | 0x50 | 194 |
| 90 | 0x5A | 195 |
| 100 | 0x64 | 196 |
| Get level | 0x00 (P12=0x01) | 197 |

### 5.43 Temporal Noise Reduction (TNR) - All Levels

**Command**: Temporal/frame-to-frame noise reduction (0-100 in 10% steps)
**Hex Code**: `0x10/0x04/0x4C` (SET), `0x10/0x04/0x8C` (GET)
**CSV Rows**: 198-209
**CRC Type**: Dynamic (CRC-16-CCITT)

**Noise Reduction Levels** (P1 = 0-100, step 10):
| Level | P1 Value | CSV Row |
|-------|----------|---------|
| 0 | 0x00 | 198 |
| 10 | 0x0A | 199 |
| 20 | 0x14 | 200 |
| 30 | 0x1E | 201 |
| 40 | 0x28 | 202 |
| 50 | 0x32 | 203 |
| 60 | 0x3C | 204 |
| 70 | 0x46 | 205 |
| 80 | 0x50 | 206 |
| 90 | 0x5A | 207 |
| 100 | 0x64 | 208 |
| Get level | 0x00 (P12=0x01) | 209 |

### 5.44 Gamma Intensity - All Levels

**Command**: Gamma curve adjustment (0-100 in 10% steps)
**Hex Code**: `0x10/0x04/0x4D` (SET), `0x10/0x04/0x8D` (GET)
**CSV Rows**: 210-221
**CRC Type**: Dynamic (CRC-16-CCITT)

**Gamma Levels** (P1 = 0-100, step 10):
| Level | P1 Value | CSV Row |
|-------|----------|---------|
| 0 | 0x00 | 210 |
| 10 | 0x0A | 211 |
| 20 | 0x14 | 212 |
| 30 | 0x1E | 213 |
| 40 | 0x28 | 214 |
| 50 | 0x32 | 215 |
| 60 | 0x3C | 216 |
| 70 | 0x46 | 217 |
| 80 | 0x50 | 218 |
| 90 | 0x5A | 219 |
| 100 | 0x64 | 220 |
| Get level | 0x00 (P12=0x01) | 221 |

---

## 6. CRC Calculation & Lookup

### 6.1 Dynamic CRC (CRC-16-CCITT)

**Used by**: Most commands (Brightness, Colormap, Contrast, Zoom, FFC, etc.)

**Algorithm**: CRC-16-CCITT
- Polynomial: 0x1021
- Initial value: 0xFFFF
- Input: Bytes [0-15] of packet
- Output: 16-bit CRC appended as [16]=LSB, [17]=MSB

**C Implementation** (from rs300.c:164-182):
```c
static uint16_t calculate_crc(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    size_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}
```

**Usage**:
```c
uint8_t cmd[18];
// ... fill cmd[0-15] with command and parameters ...
uint16_t crc = calculate_crc(cmd, 16);
cmd[16] = crc & 0xFF;        // LSB
cmd[17] = (crc >> 8) & 0xFF; // MSB
```

### 6.2 Hardcoded CRC Lookup Tables

**Used by**: Output Mode commands, Device Info GET commands

#### 6.2.1 Output Mode CRC Table (Command: 0x10/0x10/0x45)

| Mode | Value (P1) | CRC LSB (byte[16]) | CRC MSB (byte[17]) | CSV Row |
|------|------------|--------------------|--------------------|---------|
| IR   | 0x00 | 0xFB | 0xC0 | 95 |
| KBC  | 0x01 | 0x8E | 0xC3 | 96 |
| TNR  | 0x02 | 0x11 | 0xC6 | 97 |
| SNR  | 0x03 | 0x64 | 0xC5 | 98 |
| DDE  | 0x04 | 0x2F | 0xCD | 99 |
| YUV  | 0x05 | 0x5A | 0xCE | 100 |

**Implementation Example**:
```c
static const uint8_t output_mode_crc[6][2] = {
    {0xFB, 0xC0}, // Mode 0: IR
    {0x8E, 0xC3}, // Mode 1: KBC
    {0x11, 0xC6}, // Mode 2: TNR
    {0x64, 0xC5}, // Mode 3: SNR
    {0x2F, 0xCD}, // Mode 4: DDE
    {0x5A, 0xCE}, // Mode 5: YUV
};

// Usage:
cmd[16] = output_mode_crc[mode][0]; // LSB
cmd[17] = output_mode_crc[mode][1]; // MSB
```

#### 6.2.2 Device Info GET CRC Table (Command: 0x01/0x01/0x81)

| Info Type | P1 Value | P12 Value | CRC LSB | CRC MSB | CSV Row |
|-----------|----------|-----------|---------|---------|---------|
| Device Name | 0x01 | 0x20 | 0xFC | 0x1E | 56 |
| FW Version | 0x02 | 0x0B | 0x32 | 0x32 | 57 |
| VID | 0x04 | 0x02 | 0x7B | 0xCA | 58 |
| PID | 0x05 | 0x02 | 0x0E | 0xC9 | 59 |
| PN | 0x06 | 0x20 | 0xB7 | 0x16 | 60 |
| SN | 0x07 | 0x20 | 0xC2 | 0x15 | 61 |

#### 6.2.3 Shutter Control CRC Table (Command: 0x01/0x0F/0x45)

| Operation | P1 Value | CRC LSB | CRC MSB | CSV Row |
|-----------|----------|---------|---------|---------|
| Close Shutter | 0x00 | 0x8D | 0x5A | 4 |
| Open Shutter | 0x01 | 0xF8 | 0x59 | 5 |

**Implementation Example**:
```c
static const uint8_t shutter_crc[2][2] = {
    {0x8D, 0x5A}, // Close shutter
    {0xF8, 0x59}, // Open shutter
};

// Usage:
cmd[16] = shutter_crc[operation][0]; // LSB
cmd[17] = shutter_crc[operation][1]; // MSB
```

#### 6.2.4 Anti-burn Protection CRC Table (Command: 0x10/0x03/0x4B SET, 0x10/0x03/0x8B GET)

**SET Commands**:
| Setting | P1 Value | CRC LSB | CRC MSB | CSV Row |
|---------|----------|---------|---------|---------|
| OFF | 0x00 | 0x58 | 0x8D | 42 |
| ON | 0x01 | 0x2D | 0x8E | 43 |

**GET Command** (0x10/0x03/0x8B):
- CRC: 0x2D, 0x87 (fixed, CSV Row 44)

**Implementation Example**:
```c
static const uint8_t antiburn_crc[2][2] = {
    {0x58, 0x8D}, // OFF
    {0x2D, 0x8E}, // ON
};

static const uint8_t antiburn_get_crc[2] = {0x2D, 0x87}; // GET command

// Usage (SET):
cmd[16] = antiburn_crc[setting][0]; // LSB
cmd[17] = antiburn_crc[setting][1]; // MSB

// Usage (GET):
cmd[16] = antiburn_get_crc[0]; // LSB
cmd[17] = antiburn_get_crc[1]; // MSB
```

#### 6.2.5 Digital-Analog Output Format CRC (Command: 0x10/0x10/0x49)

| Command | CRC LSB | CRC MSB | CSV Row |
|---------|---------|---------|---------|
| Digital-Analog Output | 0x35 | 0xD6 | 89 |

This is a single fixed command with no variants, so the CRC is simply hardcoded as shown above.

#### 6.2.6 Sleep Control CRC Table (Command: 0x10/0x10/0x48 SET, 0x10/0x10/0x88 GET)

**SET Commands**:
| Operation | P1 Value | CRC LSB | CRC MSB | CSV Row |
|-----------|----------|---------|---------|---------|
| Wake up | 0x00 | 0x54 | 0xAD | 45 |
| Sleep | 0x01 | 0x21 | 0xAE | 46 |

**GET Command** (0x10/0x10/0x88):
- CRC: 0x21, 0xA7 (fixed, CSV Row 47)

#### 6.2.7 Boot Logo CRC Table (Command: 0x10/0x10/0x41 SET, 0x10/0x10/0x81 GET)

**SET Commands**:
| Operation | P1 Value | CRC LSB | CRC MSB | CSV Row |
|-----------|----------|---------|---------|---------|
| Disable | 0x00 | 0x5E | 0x3D | 48 |
| Enable | 0x01 | 0x2B | 0x3E | 49 |

**GET Command** (0x10/0x10/0x81):
- CRC: 0x2B, 0x37 (fixed, CSV Row 50)

#### 6.2.8 DVP/I2C Voltage Switching CRC Table (Command: 0x10/0x10/0x47 SET, 0x10/0x10/0x87 GET)

**SET Commands**:
| Voltage | P1 Value | CRC LSB | CRC MSB | CSV Row |
|---------|----------|---------|---------|---------|
| 1.8V | 0x00 | 0x39 | 0x36 | 51 |
| 3.3V | 0x01 | 0x4C | 0x35 | 52 |

**GET Command** (0x10/0x10/0x87):
- CRC: 0x4C, 0x3C (fixed, CSV Row 53)

#### 6.2.9 Parameter Save/Restore CRC Table

**Preservation** (0x10/0x10/0x51):
- CRC: 0xA9, 0xFB (CSV Row 54)

**Recovery** (0x10/0x10/0x52):
- CRC: 0x0A, 0x76 (CSV Row 55)

---

## 7. Working Examples (End-to-End)

### Example 1: Set Brightness to 50

**Goal**: Set thermal brightness to 50%
**CSV Row**: 167

**Step 1**: Construct packet
```
[0-2]   = 10 04 47 (Hex code)
[3]     = 00 (Reserved)
[4]     = 32 (P1: 50 decimal = 0x32)
[5-15]  = 00 00 00 00 00 00 00 00 00 00 00 (Padding)
```

**Step 2**: Calculate CRC over bytes [0-15]
```c
uint16_t crc = calculate_crc(cmd, 16);
cmd[16] = crc & 0xFF;        // LSB
cmd[17] = (crc >> 8) & 0xFF; // MSB
```

**Step 3**: Final packet (18 bytes)
```
10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]
```

**Step 4**: Execute
1. Write 18 bytes to I2C register 0x1d00
2. Poll register 0x0200 every 50ms until busy bit clears (timeout: 500ms)
3. Check error bit (bit 1): 0=success

### Example 2: Set Output Mode to TNR (Mode 2)

**Goal**: Enable temporal noise reduction processing
**CSV Row**: 97

**Step 1**: Construct packet
```
[0-2]   = 10 10 45 (Hex code - I2C protocol)
[3]     = 00 (Reserved)
[4]     = 02 (P1: TNR mode)
[5-15]  = 00 00 00 00 00 00 00 00 00 00 00 (Padding)
```

**Step 2**: Use hardcoded CRC (DO NOT calculate)
```
cmd[16] = 0x11; // LSB (from lookup table)
cmd[17] = 0xC6; // MSB (from lookup table)
```

**Step 3**: Final packet (18 bytes)
```
10 10 45 00 02 00 00 00 00 00 00 00 00 00 00 00 11 C6
```

**Step 4**: Execute
1. Write 18 bytes to I2C register 0x1d00
2. Poll register 0x0200 every 50ms until busy bit clears (timeout: 500ms)
3. Check error bit: 0=success

**Verification**: Compare against CSV row 97 - packet should match exactly.

### Example 3: Set Autoshutter Max Interval to 360 Seconds

**Goal**: Configure autoshutter maximum interval (multi-byte parameter)
**CSV Row**: 12

**Step 1**: Encode 360 seconds as little-endian
```
Decimal: 360
Hex: 0x0168
Little-endian: P2=0x68 (LSB), P3=0x01 (MSB)
```

**Step 2**: Construct packet
```
[0-2]   = 10 02 42 (Hex code)
[3]     = 00 (Reserved)
[4]     = 02 (P1: Maximum interval type)
[5]     = 68 (P2: LSB of 360)
[6]     = 01 (P3: MSB of 360)
[7-15]  = 00 00 00 00 00 00 00 00 00 (Padding)
```

**Step 3**: Calculate CRC
```c
uint16_t crc = calculate_crc(cmd, 16);
cmd[16] = crc & 0xFF;
cmd[17] = (crc >> 8) & 0xFF;
```

**Step 4**: Final packet
```
10 02 42 00 02 68 01 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]
```

**Step 5**: Execute and verify against CSV row 12

### Example 4: Trigger FFC (Flat Field Calibration)

**Goal**: Execute shutter calibration (longest timeout command)
**CSV Row**: 2

**Step 1**: Construct packet (all zeros except hex code)
```
[0-2]   = 10 02 43 (Hex code)
[3-15]  = 00 00 00 00 00 00 00 00 00 00 00 00 00 (All zeros)
```

**Step 2**: Calculate CRC
```c
uint16_t crc = calculate_crc(cmd, 16);
cmd[16] = crc & 0xFF;
cmd[17] = (crc >> 8) & 0xFF;
```

**Step 3**: Final packet
```
10 02 43 00 00 00 00 00 00 00 00 00 00 00 00 00 [CRC_L] [CRC_H]
```

**Step 4**: Execute with extended timeout
1. Write 18 bytes to I2C register 0x1d00
2. Poll register 0x0200 every 50ms until busy bit clears (timeout: **5000ms**)
3. Check error bit: 0=success

**Note**: FFC takes several seconds - use longer timeout (5000ms).

### Example 5: Get Device Name (Multi-Byte Response)

**Goal**: Read 40-byte device name ASCII string
**CSV Row**: 56

**Step 1**: Construct packet with hardcoded CRC
```
[0-2]   = 01 01 81 (Hex code)
[3]     = 00 (Reserved)
[4]     = 01 (P1: Device name selector)
[5-14]  = 00 00 00 00 00 00 00 00 00 00 (Padding)
[15]    = 20 (P12: Response length = 32 decimal)
[16]    = FC (CRC LSB - hardcoded)
[17]    = 1E (CRC MSB - hardcoded)
```

**Step 2**: Final packet
```
01 01 81 00 01 00 00 00 00 00 00 00 20 00 00 00 FC 1E
```

**Step 3**: Execute with multi-byte read
1. Write 18 bytes to I2C register 0x1d00
2. Poll register 0x0200 every 50ms until busy bit clears (timeout: 250ms)
3. Check error bit: 0=success
4. **Read 40 bytes from I2C register 0x1d00** (device name response)

**Step 4**: Parse response as ASCII string (40 bytes)

---

## 8. Common Mistakes & Prevention

### Mistake 1: Wrong Parameter Byte Position
```
❌ WRONG: Setting brightness at byte[5] instead of byte[4]
   Packet: 10 04 47 00 00 32 ... (brightness at P2)
   Error: Camera ignores parameter or uses default

✅ RIGHT: Setting brightness at byte[4]
   Packet: 10 04 47 00 32 00 ... (brightness at P1)
   Success: Brightness set correctly
```

### Mistake 2: Forgetting CRC Calculation
```
❌ WRONG: Leaving CRC bytes as 0x00
   Packet: 10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 00 00
   Error: Camera rejects command (CRC mismatch)

✅ RIGHT: Calculating and appending CRC
   Packet: 10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 [CRC] [CRC]
   Success: Command accepted
```

### Mistake 3: Insufficient Polling Delay
```
❌ WRONG: Polling too fast (every 1ms)
   Error: I2C bus overload, commands may fail

✅ RIGHT: Polling every 50ms with appropriate timeout
   Success: Reliable command execution
```

### Mistake 4: Zoom Level Not Multiplied by 10
```
❌ WRONG: Setting zoom P2=2 for 2x zoom
   Packet: 01 31 42 00 00 02 ... (P2=2)
   Error: Camera interprets as 0.2x zoom (wrong)

✅ RIGHT: Setting zoom P2=20 (2 × 10)
   Packet: 01 31 42 00 00 14 ... (P2=0x14=20)
   Success: 2x zoom correctly applied
```

### Mistake 5: Calculating CRC for Output Mode
```
❌ WRONG: Dynamically calculating CRC for output mode
   uint16_t crc = calculate_crc(cmd, 16); // Wrong CRC value
   Error: Camera rejects command

✅ RIGHT: Using hardcoded CRC from lookup table
   cmd[16] = 0x11; // TNR mode CRC LSB
   cmd[17] = 0xC6; // TNR mode CRC MSB
   Success: Command accepted
```

### Mistake 6: Wrong Reserved Byte Value
```
❌ WRONG: Setting byte[3] to 0x12 or non-zero
   Packet: 10 04 47 12 32 ... (reserved = 0x12)
   Error: Undefined behavior, may reject command

✅ RIGHT: Always setting byte[3] to 0x00
   Packet: 10 04 47 00 32 ... (reserved = 0x00)
   Success: Standard protocol followed
```

### Mistake 7: Multi-Byte Parameters as Big-Endian
```
❌ WRONG: Encoding 360 as big-endian (MSB first)
   P2 = 0x01 (MSB), P3 = 0x68 (LSB)
   Error: Camera interprets as 392 (0x0168 → 0x0168)

✅ RIGHT: Encoding 360 as little-endian (LSB first)
   P2 = 0x68 (LSB), P3 = 0x01 (MSB)
   Success: Camera interprets correctly as 360
```

---

## 9. Verification Checklist

Before sending any I2C command, verify:

- [ ] **Hex code matches CSV row number** - Cross-check command hex against CSV source
- [ ] **Packet is exactly 18 bytes** - Count all bytes (NOT 23 bytes)
- [ ] **Byte [0] is 0x10 or 0x01** - Verify correct I2C class (NOT 0x55)
- [ ] **Byte [3] (reserved) is 0x00** - Must always be zero
- [ ] **Parameters at correct positions** - P1=byte[4], P2=byte[5], etc. per CSV
- [ ] **CRC type correct** - Dynamic (calculate) vs Hardcoded (lookup table)
- [ ] **Timeout value appropriate** - FFC=5000ms, Device Info=250ms, others=500ms
- [ ] **Multi-byte values use little-endian** - LSB first, MSB second
- [ ] **Zoom levels multiplied by 10** - 2x zoom = P2=20, not P2=2
- [ ] **Poll 0x0200 until busy bit clears** - Check busy bit before reading response

---

## 10. Execution Flow

### 10.1 Standard Command Execution (SET Commands)

1. **Construct packet** (18 bytes)
   - Set class, module, subCmd (bytes [0-2])
   - Set reserved byte to 0x00 (byte [3])
   - Set parameters (bytes [4-15])
   - Calculate or lookup CRC (bytes [16-17])

2. **Write packet** to I2C register 0x1d00
   ```c
   i2c_write_block(client, 0x1d00, cmd, 18);
   ```

3. **Poll status register** 0x0200 until busy bit clears
   ```c
   timeout = get_timeout_for_command(cmd);
   do {
       status = i2c_read_byte(client, 0x0200);
       if (!(status & 0x01)) break; // Busy bit cleared
       msleep(50); // Wait 50ms
   } while (time_elapsed < timeout);
   ```

4. **Check error bit** (bit 1 of status byte)
   ```c
   if (status & 0x02) {
       return -EIO; // Command failed
   }
   return 0; // Command succeeded
   ```

### 10.2 GET Command Execution (Single-Byte Response)

1-3. **Same as SET commands** (construct, write, poll)

4. **Read response byte** from register 0x1d00 + offset
   ```c
   value = i2c_read_byte(client, 0x1d04); // byte[4] of response
   ```

### 10.3 GET Command Execution (Multi-Byte Response)

1-3. **Same as SET commands** (construct, write, poll)

4. **Read response buffer** from register 0x1d00
   ```c
   response_len = cmd[15]; // P12 specifies length
   i2c_read_block(client, 0x1d00, response, response_len);
   ```

---

## 11. CSV Cross-Reference

**Primary Source of Truth**: `.claude/skills/i2c-commands/Mini2_I2C_full_commands.csv`

All commands in this skill are verified against specific CSV rows. When in doubt, **always check the CSV**.

**Key CSV Row Ranges**:
- **Shutter Correction**: Row 2 (FFC trigger)
- **Background Correction**: Row 3 (Background offset)
- **Shutter Control**: Rows 4-5 (Close/Open operations)
- **Autoshutter Parameters**: Rows 6-16 (Temperature, intervals, GET)
- **K-Value Calibration**: Rows 17-23 (Collect, save, clear, restore)
- **Blind Element Calibration**: Rows 24-35 (Auto, cursor, manual marking, save)
- **Pot Lid Calibration**: Rows 36-40 (Calibration, save, clear, restore)
- **Firmware Update**: Row 41 (Update mode)
- **Anti-burn Protection**: Rows 42-44 (OFF, ON, GET)
- **Sleep Control**: Rows 45-47 (SET wake/sleep + GET)
- **Boot Logo**: Rows 48-50 (Disable, Enable, GET)
- **DVP/I2C Voltage**: Rows 51-53 (1.8V, 3.3V, GET)
- **Parameter Save/Restore**: Rows 54-55 (Preserve, recover)
- **Device Info**: Rows 56-61 (Name, FW, VID, PID, PN, SN)
- **Module Information**: Rows 62-65 (Temperature, hot spot, power-on time)
- **Digital Video Formats**: Rows 66-83 (USB, DVP, BT656, MIPI, disable)
- **Digital Format GET**: Row 84 (Query format)
- **Analog Video Formats**: Rows 85-88 (NTSC, PAL, disable, GET)
- **Digital-Analog Output Format**: Row 89 (Configuration save)
- **Detector Frame Rate**: Rows 90-94 (30Hz, 60Hz, 25Hz, 50Hz + GET)
- **Output Mode**: Rows 95-101 (6 modes + GET)
- **YUV Format**: Rows 102-106 (UYVY, VYUY, YUYV, YVYU, GET)
- **Screen Freeze**: Rows 107-109 (Disable, Enable, GET)
- **Mirror/Flip**: Rows 110-114 (No flip, H-flip, V-flip, Both, GET)
- **External Sync**: Rows 115-117 (Enable, Disable, GET)
- **Electronic Zoom Center**: Rows 118-123 (1x, 2x, 3x, 4x, 8x, GET)
- **Electronic Zoom AOI**: Rows 124-125 (Set coordinates, GET)
- **Scene Mode**: Rows 126-136 (10 scene modes + GET)
- **Colormap**: Rows 137-149 (12 color palettes + GET)
- **DDE (Digital Detail Enhancement)**: Rows 150-161 (11 values + GET)
- **Brightness**: Rows 162-173 (11 values + GET)
- **Contrast**: Rows 174-185 (11 values + GET)
- **SNR (Spatial Noise Reduction)**: Rows 186-197 (11 values + GET)
- **TNR (Temporal Noise Reduction)**: Rows 198-209 (11 values + GET)
- **Gamma Intensity**: Rows 210-221 (11 values + GET)
- **Hook Edge Position**: Rows 222-225 (3 positions + GET)

---

## 12. Related Documentation

### Primary References:
- **This Skill**: `.claude/skills/i2c-commands/SKILL.md` (Complete I2C reference)
- **CSV Source**: `.claude/skills/i2c-commands/Mini2_I2C_full_commands.csv` (Single source of truth)
- **Quick Reference**: `.claude/skills/i2c-commands/QUICK_REFERENCE.txt` (Terminal cheat sheet)

### Driver Implementation:
- **rs300.c**: Lines 164-2850
  - CRC function: 164-182
  - I2C command wrapper: 288-403
  - Output mode: 911-951

### Additional Documentation:
- **I2C Protocol Guide**: `docs/reference/I2C_PROTOCOL.md` (550 lines, detailed protocol)
- **Quick Reference**: `docs/reference/I2C_QUICK_REFERENCE.md` (Human-readable tables)
- **Troubleshooting**: `docs/reference/TROUBLESHOOTING.md` (I2C debugging section)

### Error Documentation:
- **Output Mode Error**: `.claude/lessons-learned/004-i2c-skill-output-mode-error.md` (Root cause analysis)

---

**End of RS300 I2C Commands Skill**

**Last Updated**: 2025-10-30
**Status**: COMPREHENSIVE - All 225+ I2C commands from CSV documented
**Documentation Coverage**: 45+ unique I2C commands across 11 categories
**Detailed Command Sections**: 44 detailed specifications (sections 5.1-5.44)
**CRC Tables**: 9 hardcoded CRC lookup tables documented
**All CSV Rows**: 2-225 mapped to command categories and specifications
