# RS300 I2C Command Protocol Documentation

**Complete reference for RS300 thermal camera I2C communication**

---

## Table of Contents
1. [Overview](#overview)
2. [Register Map](#register-map)
3. [Command Packet Structure](#command-packet-structure)
4. [CRC-16 Calculation](#crc-16-calculation)
5. [Status Register](#status-register)
6. [Command Reference](#command-reference)
   - [Output Mode Values](#output-mode-values)
   - [YUV Format Values](#yuv-format-values)
   - [FPS Values](#fps-values)
7. [Command Execution Flow](#command-execution-flow)
8. [Error Handling](#error-handling)
9. [Example Transactions](#example-transactions)
10. [Implementation Notes](#implementation-notes)

---

## Overview

The RS300 thermal camera uses **I2C protocol** for command and control communication on Raspberry Pi. The protocol implements:

- **Bus**: I2C bus 10 (i2c-10, I2C CSI/DSI controller)
- **Address**: 0x3c (7-bit addressing)
- **Speed**: Standard/Fast mode (100kHz - 400kHz)
- **Command Buffer**: 18-byte structured packets (I2C format)
- **Error Detection**: CRC-16-CCITT checksums
- **Status Feedback**: Dedicated status register

**Key Characteristics**:
- Synchronous command/response model
- Command buffer size: 256 bytes (0x1d00)
- Status polling with timeout/retry
- Parameter validation by hardware

**Protocol Note**: This document covers **I2C protocol communication only** (0x10 class/module commands). The camera also supports a separate serial protocol (USB/UART) with different command formats (0x55/... prefix). Do not confuse the two protocols - I2C and serial use different command structures, packet sizes, and CRC schemes. This project (RS300 V4L2 driver on Raspberry Pi) uses **I2C exclusively**. For historical context, see `.claude/lessons-learned/004-i2c-skill-output-mode-error.md`.

---

## Register Map

### Primary Registers

| Address | Name | Access | Size | Purpose |
|---------|------|--------|------|---------|
| `0x1d00` | Command/Data Buffer (RW) | R/W | 256 bytes | Primary command and data exchange |
| `0x9d00` | Command/Data Buffer (Hold) | R/W | 256 bytes | Alternative buffer (hold mode) |
| `0x0200` | Status Register | R | 1 byte | Command execution status |
| `0x8000` | Access Check Flag | - | - | Access validation (internal) |

### Buffer Usage

**Write Operation**:
```
1. Write 18-byte command to 0x1d00
2. Hardware begins execution
3. Poll 0x0200 for completion
4. Read result from 0x1d00 (for GET commands)
```

**Read Operation**:
```
1. Write GET command to 0x1d00
2. Poll 0x0200 until not busy
3. Read result buffer from 0x1d00
4. Parse result data
```

---

## Command Packet Structure

All commands use an 18-byte packet format:

```
┌─────────┬──────┬──────────────────────────────────────┐
│ Offset  │ Size │ Description                          │
├─────────┼──────┼──────────────────────────────────────┤
│ 0       │ 1    │ Command Class                        │
│ 1       │ 1    │ Module Index                         │
│ 2       │ 1    │ Sub-Command ID                       │
│ 3       │ 1    │ Reserved (usually 0x00)              │
│ 4-15    │ 12   │ Parameters (command-specific)        │
│ 16      │ 1    │ CRC-16 Low Byte                      │
│ 17      │ 1    │ CRC-16 High Byte                     │
└─────────┴──────┴──────────────────────────────────────┘
```

### Field Descriptions

**Command Class** (Byte 0):
- `0x01` - Device/system commands
- `0x10` - Camera control commands

**Module Index** (Byte 1):
- `0x01` - Device information
- `0x02` - Shutter/FFC module
- `0x03` - Display/format module
- `0x04` - Image processing module
- `0x10` - MIPI interface module
- `0x31` - Zoom module

**Sub-Command ID** (Byte 2):
- Command-specific operation code
- GET operations typically: 0x80-0x8F
- SET operations typically: 0x40-0x4F

**Parameters** (Bytes 4-15):
- 12 bytes for command parameters
- Unused bytes should be zero
- Byte order: little-endian where applicable

**CRC-16** (Bytes 16-17):
- Calculated over first 16 bytes
- Polynomial: 0x1021 (CRC-16-CCITT)
- Byte order: LSB first (little-endian)

---

## CRC-16 Calculation

### Algorithm: CRC-16-CCITT

**Polynomial**: 0x1021
**Initial Value**: 0x0000
**Input**: First 16 bytes of command packet
**Output**: 16-bit checksum (bytes 16-17, LSB first)

### C Implementation

```c
unsigned short do_crc(unsigned char *ptr, int len)
{
    unsigned int i;
    unsigned short crc = 0x0000;

    while(len--) {
        crc ^= (unsigned short)(*ptr++) << 8;
        for (i = 0; i < 8; ++i) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}
```

### Python Implementation

```python
def crc16_ccitt(data):
    """Calculate CRC-16-CCITT checksum"""
    crc = 0x0000

    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF  # Keep 16 bits

    return crc
```

### Usage Example

```python
# Build command buffer (16 bytes)
cmd = bytearray([
    0x10,  # Class
    0x04,  # Module
    0x47,  # SubCmd
    0x00,  # Reserved
    0x32,  # Parameter: brightness = 50
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Unused params
    0x00, 0x00, 0x00, 0x00, 0x00
])

# Calculate CRC
crc = crc16_ccitt(cmd)

# Append CRC (LSB first)
cmd.append(crc & 0xFF)         # Byte 16: Low byte
cmd.append((crc >> 8) & 0xFF)  # Byte 17: High byte

# Result: 18-byte command ready to send
```

---

## Status Register

### Address: 0x0200 (1 byte)

```
Bit 7-2: Error Code (6 bits)
Bit 1:   Command Failed Flag
Bit 0:   Command Busy Flag
```

### Bit Layout

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
├───┴───┴───┴───┴───┴───┼───┼───┤
│    Error Code (6b)    │ F │ B │
└───────────────────────┴───┴───┘

B = Busy (0: Idle, 1: Busy)
F = Failed (0: Success, 1: Failed)
```

### Status Values

| Bit 0 (Busy) | Bit 1 (Failed) | Meaning |
|--------------|----------------|---------|
| 0 | 0 | Idle / Command completed successfully |
| 1 | 0 | Command executing |
| 0 | 1 | Command completed with error |
| 1 | 1 | Command busy and failed (transitional) |

### Error Codes (Bits 7-2)

| Code | Hex | Description |
|------|-----|-------------|
| 0x00 | 0x00 | Success (no error) |
| 0x01 | 0x04 | Length error (invalid data length) |
| 0x02 | 0x08 | Unknown command |
| 0x03 | 0x0C | Hardware error |
| 0x04 | 0x10 | Command not enabled |
| 0x05 | 0x14 | CRC check error |
| 0x06 | 0x18 | CRC check error (variant) |
| 0x07 | 0x1C | CRC check error (variant) |

### Decoding Example

```python
def decode_status(status_byte):
    """Decode status register byte"""
    busy = status_byte & 0x01
    failed = (status_byte & 0x02) >> 1
    error_code = (status_byte & 0xFC) >> 2

    return {
        'busy': bool(busy),
        'failed': bool(failed),
        'error_code': error_code,
        'status_hex': f'0x{status_byte:02X}'
    }

# Example
status = 0x08  # 0b00001000
result = decode_status(status)
# {'busy': False, 'failed': False, 'error_code': 2, 'status_hex': '0x08'}
# Interpretation: Command complete, error code 2 (unknown command)
```

---

## Command Reference

### Complete Command Table

| Command | Class | Module | SubCmd | Parameters | Response |
|---------|-------|--------|--------|------------|----------|
| **Device Information** |
| Get Device Name | 0x01 | 0x01 | 0x81 | P1=0x01, P9=0x20 | ASCII name in buffer |
| **Shutter/FFC** |
| Trigger FFC | 0x10 | 0x02 | 0x43 | All zeros | Status only |
| **Display/Format** |
| Get Colormap | 0x10 | 0x03 | 0x85 | P9=0x01 | P1=colormap value |
| Set Colormap | 0x10 | 0x03 | 0x45 | P1=0x00, P2=value (0-11) | Status only |
| Set YUV Format | 0x10 | 0x03 | 0x4D | P1=format (0-3) | Status only |
| **Image Processing** |
| Get Brightness | 0x10 | 0x04 | 0x87 | P1=0x01, P9=0x01 | P1=brightness value |
| Set Brightness | 0x10 | 0x04 | 0x47 | P1=value (0-100) | Status only |
| Set Scene Mode | 0x10 | 0x04 | 0x42 | P1=value (0-9) | Status only |
| Set DDE | 0x10 | 0x04 | 0x45 | P1=value (0-100) | Status only |
| Set Contrast | 0x10 | 0x04 | 0x4A | P1=value (0-100) | Status only |
| Set Spatial NR | 0x10 | 0x04 | 0x4B | P1=value (0-100) | Status only |
| Set Temporal NR | 0x10 | 0x04 | 0x4C | P1=value (0-100) | Status only |
| **MIPI Interface** |
| Get Output Mode | 0x10 | 0x10 | 0x85 | P9=0x01 | P1=mode value |
| Set Output Mode | 0x10 | 0x10 | 0x45 | P1=value (0-5) | Status only (hardcoded CRC) |
| Set FPS | 0x10 | 0x10 | 0x46 | P1=0x01, P2=0x03, P3=fps | Status only |
| **Zoom** |
| Set Zoom | 0x01 | 0x31 | 0x42 | P2=level×10 (10-80) | Status only (fixed CRC) |

### Output Mode Values

| Value | Mode | Description | CRC (LSB, MSB) |
|-------|------|-------------|----------------|
| 0 | IR | Raw infrared sensor output | 0xFB, 0xC0 |
| 1 | KBC | K-based contrast enhancement | 0x8E, 0xC3 |
| 2 | TNR | Temporal noise reduction | 0x11, 0xC6 |
| 3 | SNR | Spatial noise reduction | 0x64, 0xC5 |
| 4 | DDE | Digital detail enhancement | 0x2F, 0xCD |
| 5 | YUV | YUV color output (default) | 0x5A, 0xCE |

**⚠️ IMPORTANT**: Output mode uses **hardcoded CRC values** - do NOT calculate CRC. Always use the values from the table above.

### YUV Format Values

| Value | Format | Description |
|-------|--------|-------------|
| 0 | UYVY | U-Y-V-Y ordering |
| 1 | VYUY | V-Y-U-Y ordering |
| 2 | YUYV | Y-U-Y-V ordering |
| 3 | YVYU | Y-V-Y-U ordering |

### FPS Values

Valid FPS values: **25, 30, 50, 60**

---

## Command Execution Flow

### Standard Command Sequence

```
┌─────────────────────────────────────────────────────┐
│ 1. Construct Command Packet                        │
│    - Set class, module, subcmd                     │
│    - Fill parameters                               │
│    - Calculate CRC                                 │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│ 2. Write Command to 0x1d00                         │
│    - I2C write transaction                         │
│    - 18 bytes total                                │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│ 3. Poll Status Register (0x0200)                   │
│    - Read 1 byte                                   │
│    - Check bit 0 (busy)                            │
│    - Delay 50-1000ms between reads                 │
│    - Retry up to 5-15 times                        │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│ 4. Check Status                                    │
│    - Busy=0, Failed=0: Success                     │
│    - Busy=0, Failed=1: Error (check error code)    │
│    - Busy=1: Continue polling                      │
└─────────────────┬───────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────┐
│ 5. Read Result (GET commands only)                 │
│    - Read from 0x1d00                              │
│    - Parse response data                           │
│    - Extract result from parameters                │
└─────────────────────────────────────────────────────┘
```

### Timing Requirements

| Operation | Typical Delay | Max Retries | Total Timeout |
|-----------|---------------|-------------|---------------|
| Standard Command | 50ms | 5 | 250ms |
| Brightness GET | 200ms | 5 | 1000ms |
| FFC Trigger | 1000ms | 5 | 5000ms |
| FPS Setting | 300ms | 15 | 4500ms |

---

## Error Handling

### Common Error Scenarios

#### 1. CRC Error (0x05-0x07)

**Cause**: Incorrect checksum calculation or transmission error

**Debug**:
```bash
# Verify CRC implementation
echo "Test CRC calculation with known packet"

# Check I2C bus integrity
i2cdetect -y 10
```

**Solution**: Recalculate CRC, check I2C signal quality

#### 2. Unknown Command (0x02)

**Cause**: Invalid class/module/subcmd combination

**Debug**: Verify command values match documented commands

**Solution**: Check command reference table

#### 3. Length Error (0x01)

**Cause**: Parameter length mismatch

**Debug**: Verify parameter byte count

**Solution**: Ensure unused parameters are zero

#### 4. Command Timeout

**Cause**: Status register stuck in busy state

**Debug**:
```bash
# Read status register directly
i2cget -y 10 0x3c 0x02 b 0x00

# Check kernel logs
dmesg | grep "rs300.*timeout"
```

**Solution**: Reset camera, check power supply

### Error Recovery

```python
def send_command_with_retry(i2c_bus, command_packet, max_retries=3):
    """Send command with automatic retry on CRC errors"""
    for attempt in range(max_retries):
        # Write command
        i2c_bus.write_i2c_block_data(0x3c, 0x1d, 0x00, command_packet)

        # Poll status
        for poll in range(10):
            time.sleep(0.05)
            status = i2c_bus.read_byte_data(0x3c, 0x02, 0x00)

            if not (status & 0x01):  # Not busy
                if not (status & 0x02):  # Not failed
                    return True  # Success

                error_code = (status & 0xFC) >> 2
                if error_code in [0x05, 0x06, 0x07]:  # CRC error
                    print(f"CRC error, retry {attempt + 1}/{max_retries}")
                    break  # Retry
                else:
                    return False  # Permanent error

        if poll == 9:  # Timeout
            print(f"Timeout, retry {attempt + 1}/{max_retries}")

    return False  # All retries exhausted
```

---

## Example Transactions

### Example 1: Set Brightness to 75

**Command Construction**:
```
Byte  0: 0x10  (Class: Camera control)
Byte  1: 0x04  (Module: Image processing)
Byte  2: 0x47  (SubCmd: Set brightness)
Byte  3: 0x00  (Reserved)
Byte  4: 0x4B  (Parameter: 75 decimal = 0x4B hex)
Byte  5-15: 0x00 (Unused parameters)

CRC over bytes 0-15: 0x1234 (example)
Byte 16: 0x34  (CRC low byte)
Byte 17: 0x12  (CRC high byte)
```

**I2C Transaction**:
```
Write to 0x3c, register 0x1d00:
  10 04 47 00 4B 00 00 00 00 00 00 00 00 00 00 00 34 12

Read from 0x3c, register 0x0200:
  [Poll] 0x01 (busy)
  [Poll] 0x01 (busy)
  [Poll] 0x00 (success)
```

### Example 2: Get Current Colormap

**Command Construction**:
```
Byte  0: 0x10  (Class: Camera control)
Byte  1: 0x03  (Module: Display)
Byte  2: 0x85  (SubCmd: Get colormap)
Byte  3: 0x00  (Reserved)
Byte  4-11: 0x00 (Unused)
Byte 12: 0x01  (Parameter: request flag)
Byte 13-15: 0x00

CRC: calculated
```

**I2C Transaction**:
```
Write to 0x3c, register 0x1d00:
  10 03 85 00 00 00 00 00 00 00 00 00 01 00 00 00 [CRC]

Poll 0x0200 until not busy

Read from 0x3c, register 0x1d00:
  10 03 85 00 03 00 00 00 00 00 00 00 01 00 00 00 [CRC]
           ^--- Result: colormap = 3 (Ironbow)
```

### Example 3: Trigger FFC

**Command Construction**:
```
Byte  0: 0x10  (Class: Camera control)
Byte  1: 0x02  (Module: Shutter)
Byte  2: 0x43  (SubCmd: FFC trigger)
Byte  3-15: 0x00 (All zeros)

CRC: calculated
```

**I2C Transaction**:
```
Write to 0x3c, register 0x1d00:
  10 02 43 00 00 00 00 00 00 00 00 00 00 00 00 00 [CRC]

Poll 0x0200 with 1000ms delay (FFC takes time):
  [Poll at 1000ms] 0x01 (busy - shutter closing)
  [Poll at 2000ms] 0x00 (success - FFC complete)
```

### Example 4: Set Output Mode to TNR (Temporal Noise Reduction)

**Command Construction**:
```
Byte  0: 0x10  (Class: Camera control)
Byte  1: 0x10  (Module: MIPI interface)
Byte  2: 0x45  (SubCmd: Output mode selection)
Byte  3: 0x00  (Reserved)
Byte  4: 0x02  (Parameter: 2 = TNR mode)
Byte  5-15: 0x00 (Unused parameters)
Byte 16-17: 0x11, 0xC6  (Hardcoded CRC for TNR mode - DO NOT CALCULATE)
```

**Important**: Output mode command uses a hardcoded CRC lookup table. You must use the pre-calculated CRC values for each mode. Calculating CRC dynamically will fail.

**I2C Transaction**:
```
Write to 0x3c, register 0x1d00:
  10 10 45 00 02 00 00 00 00 00 00 00 00 00 00 00 11 C6
                 ^^ (mode = 2 for TNR)                ^^^^^
                                                   CRC (hardcoded)

Poll 0x0200 with 50ms delay:
  [Poll] 0x01 (busy)
  [Poll] 0x00 (success)
```

**All Output Mode CRCs** (Use these exact values):
```
Mode 0 (IR):  0xFB, 0xC0
Mode 1 (KBC): 0x8E, 0xC3
Mode 2 (TNR): 0x11, 0xC6
Mode 3 (SNR): 0x64, 0xC5
Mode 4 (DDE): 0x2F, 0xCD
Mode 5 (YUV): 0x5A, 0xCE
```

**GET Transaction** (reading current mode):
```
Write GET command to 0x3c, register 0x1d00:
  10 10 85 00 00 00 00 00 00 00 00 00 01 00 00 00 [CRC]
                                          ^^ (request flag at byte 12)

Poll 0x0200 until not busy

Read from 0x3c, register 0x1d00:
  10 10 85 00 MM 00 00 00 00 00 00 00 01 00 00 00 [CRC]
           ^^ (MM = current mode value: 0-5)
```

---

## Implementation Notes

### Python Example (using smbus2)

```python
from smbus2 import SMBus
import time

class RS300I2C:
    def __init__(self, bus_number=10, device_addr=0x3c):
        self.bus = SMBus(bus_number)
        self.addr = device_addr
        self.cmd_buffer_addr = 0x1d00
        self.status_addr = 0x0200

    def crc16(self, data):
        """Calculate CRC-16-CCITT"""
        crc = 0x0000
        for byte in data:
            crc ^= (byte << 8)
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc = crc << 1
                crc &= 0xFFFF
        return crc

    def write_command(self, class_byte, module, subcmd, params):
        """Write command to camera"""
        # Build 18-byte packet
        cmd = bytearray([class_byte, module, subcmd, 0x00])
        cmd.extend(params[:12])  # Up to 12 parameter bytes
        cmd.extend([0x00] * (12 - len(params)))  # Pad to 12 bytes

        # Calculate and append CRC
        crc = self.crc16(cmd)
        cmd.append(crc & 0xFF)
        cmd.append((crc >> 8) & 0xFF)

        # Write to command buffer
        # Note: I2C block write for 16-bit register address
        reg_high = (self.cmd_buffer_addr >> 8) & 0xFF
        reg_low = self.cmd_buffer_addr & 0xFF

        # Send register address + data
        data = [reg_high, reg_low] + list(cmd)
        self.bus.write_i2c_block_data(self.addr, data[0], data[1:])

    def poll_status(self, timeout_ms=1000, poll_delay_ms=50):
        """Poll status register until command completes"""
        max_polls = timeout_ms // poll_delay_ms

        for _ in range(max_polls):
            time.sleep(poll_delay_ms / 1000.0)

            # Read status register
            status = self.bus.read_byte_data(self.addr, self.status_addr)

            busy = status & 0x01
            failed = status & 0x02
            error_code = (status & 0xFC) >> 2

            if not busy:
                if failed:
                    raise Exception(f"Command failed with error code {error_code}")
                return True  # Success

        raise TimeoutError("Command timed out")

    def set_brightness(self, value):
        """Set brightness (0-100)"""
        if not 0 <= value <= 100:
            raise ValueError("Brightness must be 0-100")

        self.write_command(0x10, 0x04, 0x47, [value])
        self.poll_status()

    def trigger_ffc(self):
        """Trigger flat field correction"""
        self.write_command(0x10, 0x02, 0x43, [])
        self.poll_status(timeout_ms=5000, poll_delay_ms=1000)

# Usage
camera = RS300I2C()
camera.set_brightness(75)
camera.trigger_ffc()
```

### C/Linux Kernel Driver Pattern

See `rs300.c` for production implementation:
- I2C transfer functions: rs300.c:205-267
- Command execution: rs300.c:635-1624
- CRC calculation: rs300.c:152-170

### Common Pitfalls

1. **Forgetting CRC**: Always calculate and append CRC
2. **Wrong byte order**: CRC is LSB first (little-endian)
3. **Insufficient polling**: Some commands need 1000ms+ delays
4. **Not checking busy flag**: Must poll until busy=0
5. **Parameter padding**: Unused params must be 0x00
6. **Timeout too short**: FFC needs 5+ seconds

---

## Debugging Tools

### Command-Line I2C Testing

```bash
# Read status register
i2cget -y 10 0x3c 0x02 b 0x00

# Write raw command (example: brightness=50)
# Note: This is complex due to 16-bit register addressing
# Use Python/C tools instead

# Monitor I2C traffic
i2cdetect -y 10
i2cdump -y 10 0x3c
```

### Logic Analyzer Capture

For protocol analysis, capture I2C signals:
- **SCL**: Clock line
- **SDA**: Data line
- **Trigger**: On I2C start condition, address 0x3c

Verify:
- Correct command byte sequence
- CRC bytes match calculation
- Status register polling timing

---

## Cross-References

- **SKILL.md** - Structured skill database with complete command listing and CSV references
- **CHANGES.md** - Documentation update history and protocol corrections
- **Mini2_I2C_full_commands.csv** - Complete command reference table with all parameters
- **Lesson Learned (004)** - `.claude/lessons-learned/004-i2c-skill-output-mode-error.md` - Historical context on I2C vs serial protocol confusion
- **Driver Implementation** - `rs300.c` - Production C/Linux kernel driver with full I2C implementation
- **Quick Reference** - `.claude/skills/i2c-commands/QUICK_REFERENCE.txt` - Command cheat sheet

---

**Document Version**: 1.1
**Last Updated**: 2025-10-30
**Device**: RS300 Thermal Camera (Mini2)
**I2C Address**: 0x3c (7-bit)
**Recent Updates**: Added complete output mode command section (SET/GET), clarified I2C vs serial protocol distinction
