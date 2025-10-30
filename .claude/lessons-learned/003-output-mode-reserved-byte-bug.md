# Output Mode Reserved Byte Bug (Byte [3] = 0x12)

**Date**: 2025-10-29
**Severity**: High - Protocol violation
**Status**: Fixed
**Commit**: Pending

---

## Issue Summary

The **Set Output Mode** I2C command (0x55/0x43/0x49) had an incorrect value in the reserved byte field.

**Bug**: `cmd_buffer[3] = 0x12` (wrong)
**Correct**: `cmd_buffer[3] = 0x00` (protocol compliant)

---

## Root Cause Analysis

The 23-byte Output Mode command structure has 4 header bytes:

```
[0] = 0x55 (Class)
[1] = 0x43 (Module)
[2] = 0x49 (SubCmd)
[3] = Reserved field
```

Per I2C protocol conventions and RS300 specification, **reserved bytes must always be 0x00**. The driver incorrectly used `0x12` for this field.

### Why This Matters

Reserved bytes exist to:
1. Allow firmware to distinguish malformed packets
2. Provide forward compatibility if protocol evolves
3. Maintain alignment with protocol specification

Using `0x12` instead of `0x00` violates the I2C_PROTOCOL.md specification:

```
Byte 3: Reserved (always 0x00)
```

---

## Implementation Details

### Location
**File**: `rs300.c:932`
**Function**: `rs300_set_output_mode()`

### Before (Incorrect)
```c
cmd_buffer[3] = 0x12;  /* Reserved */
```

### After (Fixed)
```c
cmd_buffer[3] = 0x00;  /* Reserved - MUST be 0x00 per I2C protocol */
```

---

## Expected Impact

### What Changed
- Output mode commands now send correct 23-byte packets
- Reserved byte now complies with I2C_PROTOCOL.md specification
- All 6 output modes affected (IR, KBC, TNR, SNR, DDE, YUV)

### Potential Issues This Could Have Caused
- Packet rejection by stricter firmware implementations
- Command timeout on some hardware revisions
- Unpredictable behavior in edge cases

### Testing Required
```bash
# Verify output mode switching works correctly
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl output_mode=0  # IR mode
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl output_mode=4  # DDE mode
v4l2-ctl -d /dev/v4l-subdev2 --set-ctrl output_mode=5  # YUV mode

# Check kernel logs for errors
dmesg | grep "rs300.*output"
```

---

## Packet Structure Reference

Complete 23-byte Output Mode command format:

| Byte | Value | Type | Purpose |
|------|-------|------|---------|
| [0] | 0x55 | Fixed | Command Class |
| [1] | 0x43 | Fixed | Module Index |
| [2] | 0x49 | Fixed | SubCmd |
| **[3]** | **0x00** | **Reserved** | **Must be 0x00** |
| [4] | 0x00 | Fixed | Parameter (unknown purpose) |
| [5] | 0x10 | Fixed | Parameter (unknown purpose) |
| [6] | 0x10 | Fixed | Parameter (unknown purpose) |
| [7] | 0x45 | Fixed | Parameter (unknown purpose) |
| [8] | 0x00 | Fixed | Parameter (unknown purpose) |
| [9] | 0-5 | Variable | Output mode (0=IR, 1=KBC, 2=TNR, 3=SNR, 4=DDE, 5=YUV) |
| [10-20] | 0x00 | Padding | 11 zero bytes |
| [21] | CRC_Low | Lookup | Mode-specific CRC low byte |
| [22] | CRC_High | Lookup | Mode-specific CRC high byte |

---

## Comparison with Other Commands

For reference, all other commands use 0x00 in the reserved byte position:

**Brightness SET** (standard 18-byte):
```
10 04 47 00 4B 00 00 00 00 00 00 00 00 00 00 00 [CRC]
             ^^
          0x00 ✓
```

**Colormap SET** (standard 18-byte):
```
10 03 45 00 00 XX 00 00 00 00 00 00 00 00 00 00 [CRC]
             ^^
          0x00 ✓
```

**Output Mode SET** (extended 23-byte):
```
55 43 49 00 00 10 10 45 00 YY 00 00 00 00 00 00 00 00 [CRC] [CRC]
             ^^
          0x00 ✓ (NOW FIXED)
```

---

## CRC Lookup Table (Unchanged)

The hardcoded CRC values for each output mode are correct:

```c
static const u8 mode_crc[6][2] = {
    {0xFB, 0xC0},  /* Mode 0 (IR) */
    {0x8E, 0xC3},  /* Mode 1 (KBC) */
    {0x11, 0xC6},  /* Mode 2 (TNR) */
    {0x64, 0xC5},  /* Mode 3 (SNR) */
    {0x2F, 0xCD},  /* Mode 4 (DDE) */
    {0x5A, 0xCE},  /* Mode 5 (YUV) */
};
```

These CRCs remain valid—they were calculated correctly. Only the reserved byte value was wrong.

---

## Lessons Learned

1. **Reserved bytes are not optional**: Always use 0x00 for reserved fields, even if the device appears to accept other values
2. **Protocol compliance matters**: Specification violations can cause intermittent failures
3. **Code review catches protocol errors**: This was found through systematic I2C packet structure validation
4. **Comment precision helps**: "Reserved" comment is vague; better to say "Reserved - MUST be 0x00"

---

## Related Documentation

- **I2C Protocol Spec**: `docs/reference/I2C_PROTOCOL.md` (Output Mode section)
- **I2C Commands Skill**: `.claude/skills/i2c-commands/` (reference guide)
- **Driver Implementation**: `rs300.c:911-994` (rs300_set_output_mode function)

---

**Fixed by**: Code review audit (2025-10-29)
**Status**: Ready for testing and commit
