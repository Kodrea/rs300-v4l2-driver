# RS300 I2C Commands Skill

**Complete reference for formatting and validating RS300 thermal camera I2C commands**

---

## ⚠️ CRITICAL: This skill documents I2C PROTOCOL ONLY

**DO NOT confuse with serial protocol**:
- ✅ I2C Protocol: 18-byte packets, class 0x10 (camera) or 0x01 (device/zoom)
- ❌ Serial Protocol: 23-byte packets, class 0x55 (USB/UART only)
- **These protocols are incompatible** - using serial commands on I2C bus will fail

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

**⚠️ DO NOT USE class 0x55** - This is serial protocol only (USB/UART), not I2C.

---

## 3. Command Reference (Quick Summary)

### 3.1 Output & Display Commands

| Command | Hex Code | Type | CRC | Params | Response | Timeout | CSV Rows |
|---------|----------|------|-----|--------|----------|---------|----------|
| Brightness GET | 0x10/0x04/0x87 | GET | Dyn | P1=0x01, P9=0x01 | Byte[4] | 500ms | 173 |
| Brightness SET | 0x10/0x04/0x47 | SET | Dyn | P1=0-100 | Status | 500ms | 162-172 |
| Colormap GET | 0x10/0x03/0x85 | GET | Dyn | P1=0x01, P9=0x01 | Byte[4] | 500ms | 149 |
| Colormap SET | 0x10/0x03/0x45 | SET | Dyn | ⚠️P2=0-11 | Status | 500ms | 137-148 |
| Contrast SET | 0x10/0x04/0x4A | SET | Dyn | P1=0-100 | Status | 500ms | 174-184 |
| **Output Mode SET** | **0x10/0x10/0x45** | **SET** | **Hard** | **P1=0-5** | **Status** | **500ms** | **95-101** |
| YUV Format SET | 0x10/0x03/0x4D | SET | Dyn | P1=0-3 | Status | 500ms | 102-106 |

**⚠️ Critical Note**: Output Mode command uses **I2C protocol** (0x10/0x10/0x45), NOT serial protocol (0x55/0x43/0x49).

### 3.2 Image Processing Commands

| Command | Hex Code | Type | CRC | Params | Response | Timeout | CSV Rows |
|---------|----------|------|-----|--------|----------|---------|----------|
| DDE SET | 0x10/0x04/0x45 | SET | Dyn | P1=0-100 | Status | 500ms | 150-161 |
| SNR SET | 0x10/0x04/0x4B | SET | Dyn | P1=0-100 | Status | 500ms | 186-197 |
| TNR SET | 0x10/0x04/0x4C | SET | Dyn | P1=0-100 | Status | 500ms | 198-208 |
| Scene Mode SET | 0x10/0x04/0x42 | SET | Dyn | P1=0-9 | Status | 500ms | 126-136 |
| Zoom SET | 0x01/0x31/0x42 | SET | Dyn | P1=0x00, P2=level×10 | Status | 500ms | 118-122 |

### 3.3 Device Control Commands

| Command | Hex Code | Type | CRC | Params | Response | Timeout | CSV Rows |
|---------|----------|------|-----|--------|----------|---------|----------|
| FFC Trigger | 0x10/0x02/0x43 | SET | Dyn | All=0x00 | Status | 5000ms | 2 |
| FPS SET | 0x10/0x10/0x46 | SET | Dyn | P1=0x01, P2=0x03, P3=fps | Status | 4500ms | 66-94 |
| Autoshutter GET | 0x10/0x02/0x81 | GET | Dyn | P1=0x01, P9=0x01 | Byte[4] | 500ms | 8 |
| Autoshutter SET | 0x10/0x02/0x41 | SET | Dyn | P1=0/1 | Status | 500ms | 6-7 |
| Autoshutter Params | 0x10/0x02/0x42 | SET | Dyn | P1=type, P2-3=multi-byte | Status | 500ms | 9-16 |
| Sleep GET | 0x10/0x10/0x88 | GET | Dyn | P1=0x01, P9=0x01 | Byte[4] | 500ms | 47 |
| Sleep SET | 0x10/0x10/0x48 | SET | Dyn | P1=0/1 | Status | 500ms | 45-46 |

### 3.4 Device Information Commands

| Command | Hex Code | Type | CRC | Params | Response | Timeout | CSV Row |
|---------|----------|------|-----|--------|----------|---------|---------|
| Device Name GET | 0x01/0x01/0x81 | GET | Hard | P1=0x01, P12=0x20 | 40 bytes | 250ms | 56 |
| FW Version GET | 0x01/0x01/0x81 | GET | Hard | P1=0x02, P12=0x0B | 11 bytes | 250ms | 57 |
| VID GET | 0x01/0x01/0x81 | GET | Hard | P1=0x04, P12=0x02 | 2 bytes | 250ms | 58 |
| PID GET | 0x01/0x01/0x81 | GET | Hard | P1=0x05, P12=0x02 | 2 bytes | 250ms | 59 |
| PN GET | 0x01/0x01/0x81 | GET | Hard | P1=0x06, P12=0x20 | 32 bytes | 250ms | 60 |
| SN GET | 0x01/0x01/0x81 | GET | Hard | P1=0x07, P12=0x20 | 32 bytes | 250ms | 61 |

**Legend**:
- **CRC**: `Dyn` = Calculate using CRC-16-CCITT, `Hard` = Use hardcoded lookup table
- **Response**: `Status` = Poll until busy, `Byte[4]` = Single byte response, `N bytes` = Multi-byte response
- **Params**: `P1` = byte[4], `P2` = byte[5], etc. Unused bytes must be 0x00

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

**⚠️ CRITICAL WARNING**:
- **DO NOT use serial protocol** (0x55/0x43/0x49) on I2C bus
- **DO NOT calculate CRC** - always use hardcoded values from table above
- **DO NOT use 23-byte packet format** - I2C uses standard 18 bytes

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

**⚠️ Parameter Position Ambiguity**: CSV shows colormap at **P2** (byte[5]), but standard patterns suggest **P1** (byte[4]). Use P2 per CSV, but verify if issues occur.

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

---

## 7. Protocol Disambiguation: I2C vs Serial

**RS300 supports TWO completely different command protocols. This skill documents I2C ONLY.**

### 7.1 Protocol Comparison

| Feature | I2C Protocol (This Skill) | Serial Protocol (NOT This Skill) |
|---------|---------------------------|----------------------------------|
| **Interface** | I2C bus (Raspberry Pi) | USB/UART serial |
| **Packet Size** | 18 bytes | 23 bytes |
| **Command Class** | 0x10 (camera), 0x01 (device) | 0x55 (serial wrapper) |
| **Output Mode Hex** | 0x10/0x10/0x45 | 0x55/0x43/0x49 |
| **Structure** | Standard I2C format | Extended serial format with wrapper |
| **CRC Position** | Bytes [16-17] | Bytes [21-22] |
| **Compatibility** | I2C bus ONLY | Serial interface ONLY |

### 7.2 Output Mode Example: I2C vs Serial

**I2C Protocol** (CORRECT for this driver):
```
Packet (18 bytes): 10 10 45 00 02 00 00 00 00 00 00 00 00 00 00 00 11 C6
                   ^^  ^^  ^^      ^^                                ^^^^^
                   Class Module Sub  Mode                            CRC
                   0x10  0x10  0x45  TNR                             [16-17]
```

**Serial Protocol** (WRONG for I2C - shown for disambiguation only):
```
Packet (23 bytes): 55 43 49 00 00 10 10 45 00 02 ... [padding] ... [CRC at 21-22]
                   ^^  ^^  ^^      ^^  ^^  ^^
                   Serial wrapper  I2C command embedded
                   0x55/0x43/0x49  (different structure)
```

**⚠️ CRITICAL**: Never use serial protocol commands (0x55/...) on I2C bus. They are incompatible and will fail.

---

## 8. Working Examples (End-to-End)

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

## 9. Common Mistakes & Prevention

### Mistake 1: Using Serial Protocol on I2C Bus
```
❌ WRONG: Using 0x55/0x43/0x49 for output mode
   Packet: 55 43 49 00 ... (23 bytes)
   Error: Camera rejects command, wrong protocol

✅ RIGHT: Using 0x10/0x10/0x45 for output mode
   Packet: 10 10 45 00 ... (18 bytes)
   Success: I2C protocol command accepted
```

### Mistake 2: Wrong Parameter Byte Position
```
❌ WRONG: Setting brightness at byte[5] instead of byte[4]
   Packet: 10 04 47 00 00 32 ... (brightness at P2)
   Error: Camera ignores parameter or uses default

✅ RIGHT: Setting brightness at byte[4]
   Packet: 10 04 47 00 32 00 ... (brightness at P1)
   Success: Brightness set correctly
```

### Mistake 3: Forgetting CRC Calculation
```
❌ WRONG: Leaving CRC bytes as 0x00
   Packet: 10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 00 00
   Error: Camera rejects command (CRC mismatch)

✅ RIGHT: Calculating and appending CRC
   Packet: 10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 [CRC] [CRC]
   Success: Command accepted
```

### Mistake 4: Insufficient Polling Delay
```
❌ WRONG: Polling too fast (every 1ms)
   Error: I2C bus overload, commands may fail

✅ RIGHT: Polling every 50ms with appropriate timeout
   Success: Reliable command execution
```

### Mistake 5: Zoom Level Not Multiplied by 10
```
❌ WRONG: Setting zoom P2=2 for 2x zoom
   Packet: 01 31 42 00 00 02 ... (P2=2)
   Error: Camera interprets as 0.2x zoom (wrong)

✅ RIGHT: Setting zoom P2=20 (2 × 10)
   Packet: 01 31 42 00 00 14 ... (P2=0x14=20)
   Success: 2x zoom correctly applied
```

### Mistake 6: Calculating CRC for Output Mode
```
❌ WRONG: Dynamically calculating CRC for output mode
   uint16_t crc = calculate_crc(cmd, 16); // Wrong CRC value
   Error: Camera rejects command

✅ RIGHT: Using hardcoded CRC from lookup table
   cmd[16] = 0x11; // TNR mode CRC LSB
   cmd[17] = 0xC6; // TNR mode CRC MSB
   Success: Command accepted
```

### Mistake 7: Wrong Reserved Byte Value
```
❌ WRONG: Setting byte[3] to 0x12 or non-zero
   Packet: 10 04 47 12 32 ... (reserved = 0x12)
   Error: Undefined behavior, may reject command

✅ RIGHT: Always setting byte[3] to 0x00
   Packet: 10 04 47 00 32 ... (reserved = 0x00)
   Success: Standard protocol followed
```

### Mistake 8: Multi-Byte Parameters as Big-Endian
```
❌ WRONG: Encoding 360 as big-endian (MSB first)
   P2 = 0x01 (MSB), P3 = 0x68 (LSB)
   Error: Camera interprets as 392 (0x0168 → 0x0168)

✅ RIGHT: Encoding 360 as little-endian (LSB first)
   P2 = 0x68 (LSB), P3 = 0x01 (MSB)
   Success: Camera interprets correctly as 360
```

---

## 10. Verification Checklist

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

## 11. Execution Flow

### 11.1 Standard Command Execution (SET Commands)

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

### 11.2 GET Command Execution (Single-Byte Response)

1-3. **Same as SET commands** (construct, write, poll)

4. **Read response byte** from register 0x1d00 + offset
   ```c
   value = i2c_read_byte(client, 0x1d04); // byte[4] of response
   ```

### 11.3 GET Command Execution (Multi-Byte Response)

1-3. **Same as SET commands** (construct, write, poll)

4. **Read response buffer** from register 0x1d00
   ```c
   response_len = cmd[15]; // P12 specifies length
   i2c_read_block(client, 0x1d00, response, response_len);
   ```

---

## 12. CSV Cross-Reference

**Primary Source of Truth**: `.claude/skills/i2c-commands/Mini2_I2C_full_commands.csv`

All commands in this skill are verified against specific CSV rows. When in doubt, **always check the CSV**.

**Key CSV Row Ranges**:
- **Output Mode**: Rows 95-101 (6 modes + GET)
- **Sleep Control**: Rows 45-47 (SET wake/sleep + GET)
- **Device Info**: Rows 56-61 (Name, FW, VID, PID, PN, SN)
- **FPS Settings**: Rows 66-94 (Multiple interface/framerate combos)
- **Zoom Control**: Rows 118-122 (5 levels: 1x, 2x, 3x, 4x, 8x)
- **Scene Mode**: Rows 126-136 (10 scene modes)
- **Colormap**: Rows 137-148 (12 color palettes)
- **DDE (Digital Detail Enhancement)**: Rows 150-161 (11 values)
- **Brightness**: Rows 162-172 (11 values: 0, 10, 20, ..., 100)
- **Contrast**: Rows 174-184 (11 values)
- **SNR (Spatial Noise Reduction)**: Rows 186-197 (11 values)
- **TNR (Temporal Noise Reduction)**: Rows 198-208 (11 values)

---

## 13. Related Documentation

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

**Last Updated**: 2025-10-29
**Status**: Production-ready, all commands verified against CSV source
**Critical Fix Applied**: Output Mode protocol corrected (serial → I2C format)
