# I2C Commands Implementation Priority List

**Created**: 2025-10-24
**Purpose**: Track which I2C commands to add to rs300.c driver
**Strategy**: Implement 2-3 commands at a time, test thoroughly before moving to next batch

**📋 Sprint Planning**: See how these commands integrate into the overall modernization plan:
- **[MODERNIZATION_PLAN.md](MODERNIZATION_PLAN.md)** - Full technical sprint plan with detailed timelines
- **[docs/contributing/ROADMAP.md](docs/contributing/ROADMAP.md)** - High-level project roadmap

**Quick Sprint Reference**:
- **Sprint 1 (Week 1-2)**: Batches 1-2 (Autoshutter + Sleep) - 5 commands
- **Sprint 2 (Week 3-4)**: Batch 3 (Device Info) - 3 commands
- **Sprint 3 (Week 5-6)**: Batch 4 (Gamma) - 2 commands (optional)
- **Sprint 6 (Future)**: Batches 5-6 (Advanced) - 6 commands

---

## Implementation Status Legend

- 🔥 **Batch 1** (TOP PRIORITY) - Implement immediately
- ⚡ **Batch 2** - Implement next
- 📊 **Batch 3** - Implement after Batch 2
- 🎨 **Batch 4** - Nice to have
- ⏳ **Future** - Lower priority

---

## 🔥 Batch 1: Autoshutter Commands (TOP PRIORITY)

**Priority**: Highest - User requested autoshutter as #1 priority
**Commands**: 3 commands (SET, GET, PARAMETERS)

| Function Name | Command | Class | Module | SubCmd | Parameters | Description |
|---------------|---------|-------|--------|--------|------------|-------------|
| `rs300_set_autoshutter()` | Auto Shutter Switch | 0x10 | 0x02 | 0x41 | P1=0 (off) or 1 (on) | Enable/disable automatic FFC |
| `rs300_get_autoshutter()` | Auto Shutter Switch Get | 0x10 | 0x02 | 0x81 | P9=0x01, Len=0x0001 | Read autoshutter state |
| `rs300_set_autoshutter_params()` | Auto Shutter Parameters | 0x10 | 0x02 | 0x42 | See below | Configure temp threshold and intervals |

**Autoshutter Parameters**:
- **P1[0]=0**: Temperature threshold setting
  - P1[2:1] = temp value in units of 1/32°C
  - Example: 0x000A = 10/32 = 0.31°C, 0x0032 = 50/32 = 1.56°C
- **P1[0]=1**: Minimum interval time
  - P1[2:1] = seconds (e.g., 0x0001 = 1s, 0x0005 = 5s)
- **P1[0]=2**: Maximum interval time
  - P1[2:1] = seconds (e.g., 0x0078 = 120s, 0x0168 = 360s)

**V4L2 Control Integration**:
```c
// Main switch
static const struct v4l2_ctrl_config autoshutter_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 9,
    .name = "Auto Shutter",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,
    .max = 1,
    .step = 1,
    .def = 0,  // Off by default
};

// Temperature threshold (use existing temperature control pattern)
static const struct v4l2_ctrl_config autoshutter_temp_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 10,
    .name = "Auto Shutter Temperature",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 10,   // 0.31°C minimum
    .max = 100,  // 3.12°C maximum
    .step = 1,
    .def = 50,   // 1.56°C default
};

// Minimum interval
static const struct v4l2_ctrl_config autoshutter_min_interval_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 11,
    .name = "Auto Shutter Min Interval",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 1,
    .max = 300,
    .step = 1,
    .def = 1,    // 1 second default
};

// Maximum interval
static const struct v4l2_ctrl_config autoshutter_max_interval_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 12,
    .name = "Auto Shutter Max Interval",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 1,
    .max = 600,
    .step = 1,
    .def = 120,  // 120 seconds default
};
```

**Implementation Notes**:
- Autoshutter triggers FFC automatically based on temperature change
- Temperature threshold is change in detector temperature (not scene temp)
- Interval: Min = fastest FFC rate, Max = slowest FFC rate
- When temp change exceeds threshold, FFC triggers (respecting intervals)
- Use existing `rs300_send_command()` pattern from other controls

**Testing**:
```bash
# Enable autoshutter
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter=1

# Set temperature threshold to 50 (1.56°C)
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter_temperature=50

# Set min interval to 5 seconds
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter_min_interval=5

# Set max interval to 300 seconds
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter_max_interval=300

# Get current state
v4l2-ctl -d /dev/v4l-subdev2 --get-ctrl=autoshutter
```

**Hex Command Examples**:
```c
// Enable autoshutter
// 10 02 41 00 01 00 00 00 00 00 00 00 00 00 00 00 78 3D

// Set temp threshold to 50 (1.56°C)
// 10 02 42 00 00 32 00 00 00 00 00 00 00 00 00 00 5A EC

// Set min interval to 1s
// 10 02 42 00 01 01 00 00 00 00 00 00 00 00 00 00 92 68

// Set max interval to 120s
// 10 02 42 00 02 78 00 00 00 00 00 00 00 00 00 00 58 AC
```

---

## 🔥 Batch 2: Module Sleep Commands (HIGH PRIORITY)

**Priority**: High - User requested sleep as #2 priority
**Commands**: 3 commands (SLEEP, WAKE, GET)

| Function Name | Command | Class | Module | SubCmd | Parameters | Description |
|---------------|---------|-------|--------|--------|------------|-------------|
| `rs300_set_sleep()` | Module Sleep | 0x10 | 0x10 | 0x48 | P1=0 (wake) or 1 (sleep) | Put module to sleep or wake up |
| `rs300_get_sleep()` | Module Sleep Get | 0x10 | 0x10 | 0x88 | P9=0x01, Len=0x0001 | Read sleep state |

**Sleep Behavior** (from CSV):
- After sleeping: video is frozen
- Only responds to wake-up command (0x48 with P1=0)
- Does NOT respond to other commands while asleep
- Useful for power saving

**V4L2 Control Integration**:
```c
static const struct v4l2_ctrl_config sleep_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 13,
    .name = "Module Sleep",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,  // Awake
    .max = 1,  // Sleep
    .step = 1,
    .def = 0,  // Awake by default
};
```

**Implementation Notes**:
- Sleep mode freezes video output
- Wake command: 0x10 0x10 0x48 0x00 0x00... (P1=0)
- Sleep command: 0x10 0x10 0x48 0x00 0x01... (P1=1)
- Driver should track sleep state to avoid command timeouts
- Consider: Should streaming be stopped before sleep?

**Testing**:
```bash
# Put module to sleep
v4l2-ctl -d /dev/v4l-subdev2 -c module_sleep=1

# Wake up module
v4l2-ctl -d /dev/v4l-subdev2 -c module_sleep=0

# Get current state
v4l2-ctl -d /dev/v4l-subdev2 --get-ctrl=module_sleep
```

**Hex Command Examples**:
```c
// Wake up
// 10 10 48 00 00 00 00 00 00 00 00 00 00 00 00 00 54 AD

// Sleep
// 10 10 48 00 01 00 00 00 00 00 00 00 00 00 00 00 21 AE
```

---

## ⚡ Batch 3: Device Information Commands

**Priority**: Medium-High - Useful for auto-detection and diagnostics
**Commands**: 3 commands

| Function Name | Command | Class | Module | SubCmd | Parameters | Description |
|---------------|---------|-------|--------|--------|------------|-------------|
| `rs300_get_device_name()` | Get Device Name | 0x01 | 0x01 | 0x81 | P1=0x01, P9=0x20 | Read device name string |
| `rs300_get_firmware_version()` | Get Firmware Version | 0x01 | 0x01 | 0x81 | P1=0x02, P9=0x0B | Read FW version string |
| `rs300_get_module_temperature()` | Get Module Temp | 0x10 | 0x10 | 0x91 | P9=0x02, Len=0x0002 | Read detector temperature |

**Usage**:
- Device Name: "Camera MINI2384" or similar (15-32 bytes ASCII)
- Firmware Version: "00.00.01.11" format (11 bytes ASCII)
- Module Temp: uint16 in 0.01°C units (e.g., 0x0C3D = 3133 = 31.33°C)

**V4L2 Integration**:
- **Device Name**: Use in probe, log to dmesg, could help auto-detect resolution
- **Firmware Version**: Use in probe, log to dmesg
- **Temperature**: Read-only control (like pixel_rate and link_freq)

**Implementation Priority**:
1. **Get Device Name** - HIGH (useful for auto-detection in MODERNIZATION_PLAN Priority 1)
2. **Get Firmware Version** - MEDIUM (useful for diagnostics)
3. **Get Module Temperature** - MEDIUM (interesting but not critical)

**Hex Command Examples**:
```c
// Get device name
// 01 01 81 00 01 00 00 00 00 00 00 00 20 00 00 00 FC 1E

// Get firmware version
// 01 01 81 00 02 00 00 00 00 00 00 00 0B 00 00 00 32 32

// Get module temperature
// 10 10 91 00 00 00 00 00 00 00 00 00 02 00 00 00 00 6A
```

---

## 📊 Batch 4: Image Enhancement Commands

**Priority**: Medium - Nice to have, not critical
**Commands**: 2 commands

| Function Name | Command | Class | Module | SubCmd | Parameters | Description |
|---------------|---------|-------|--------|--------|------------|-------------|
| `rs300_set_gamma()` | Gamma Intensity | 0x10 | 0x04 | 0x4D | P1=0-100 | Adjust gamma curve (0-100) |
| `rs300_get_gamma()` | Gamma Intensity Get | 0x10 | 0x04 | 0x8D | P1=0x01, P9=0x01, Len=0x0001 | Read gamma value |

**V4L2 Control Integration**:
```c
static const struct v4l2_ctrl_config gamma_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_GAMMA,  // Standard V4L2 control!
    .type = V4L2_CTRL_TYPE_INTEGER,
    .name = "Gamma",
    .min = 0,
    .max = 100,
    .step = 1,
    .def = 50,
};
```

**Implementation Notes**:
- Use standard `V4L2_CID_GAMMA` instead of custom control
- Gamma adjusts brightness curve (0=linear, 100=maximum curve)
- Similar to contrast but affects mid-tones differently

**Hex Command Example**:
```c
// Set gamma to 50
// 10 04 4D 00 32 00 00 00 00 00 00 00 00 00 00 00 B2 A1
```

---

## 🎨 Batch 5: Advanced Features (Nice to Have)

**Priority**: Lower - Useful but not essential
**Commands**: 2-3 commands

| Function Name | Command | Class | Module | SubCmd | Parameters | Description |
|---------------|---------|-------|--------|--------|------------|-------------|
| `rs300_set_edge_position()` | Hook Edge Position | 0x10 | 0x04 | 0x4E | P1=0-2 | Edge enhancement position |
| `rs300_set_antiburn()` | Anti-burn Protection | 0x10 | 0x03 | 0x4B | P1=0 or 1 | Prevent OLED burn-in |
| `rs300_get_hotspot_coords()` | Get Hotspot Coords | 0x10 | 0x10 | 0x92 | ROI coords | Find hottest point in region |

**Hook Edge Position**:
- 0: No edge enhancement position (edge lines off)
- 1: Position 1 (moderate edge lines)
- 2: Position 2 (strong edge lines)
- Complements existing DDE (digital detail enhancement) control

**Anti-burn Protection**:
- Prevents screen burn-in on OLED/LCD displays
- Periodically shifts image slightly
- Useful for long-term static displays

**Hotspot Coordinates**:
- Returns (x, y) coordinates of hottest pixel in ROI
- Also returns min, max, average temperatures in region
- Very useful for thermal analysis applications
- ROI specified as (x1, y1, x2, y2)

---

## ⏳ Batch 6: System Features (Future)

**Priority**: Low - Advanced users only
**Commands**: 3 commands

| Function Name | Command | Class | Module | SubCmd | Parameters | Description |
|---------------|---------|-------|--------|--------|------------|-------------|
| `rs300_set_boot_logo()` | Boot Logo | 0x10 | 0x10 | 0x41 | P1=0 or 1 | Show logo at startup |
| `rs300_save_params()` | Save Parameters | 0x10 | 0x10 | 0x51 | None | Save current settings to flash |
| `rs300_restore_params()` | Restore Parameters | 0x10 | 0x10 | 0x52 | None | Restore saved settings |

**Parameter Save/Restore**:
- Save: Persists current control values to camera flash memory
- Restore: Loads saved values from flash
- Useful for creating "presets"
- Settings persist across power cycles

---

## Commands Explicitly EXCLUDED (Per User Request)

**Do NOT implement these**:

### Calibration Commands (Module 0x11)
- ❌ K-value calibration (0x11, 0x41-0x45)
- ❌ Blind element/dead pixel calibration (0x11, 0x51-0x58)
- ❌ Pot lid/background calibration (0x11, 0x61-0x65)
- **Reason**: Complex calibration procedures, require special equipment

### Non-MIPI Digital Outputs
- ❌ USB output modes (0x10, 0x10, 0x46 with P1[1]=0x00)
- ❌ DVP output modes (0x10, 0x10, 0x46 with P1[1]=0x01)
- **Reason**: Driver is MIPI-only, these modes not relevant

### Dangerous Commands
- ❌ Firmware update (0x01, 0x01, 0x42) - Puts module in DFU mode
- **Reason**: Can brick camera if used incorrectly

---

## Implementation Workflow (For Each Batch)

### Step 1: Add Function Prototypes (rs300.c ~line 185)
```c
static int rs300_set_autoshutter(struct rs300 *rs300, int enable);
static int rs300_get_autoshutter(struct rs300 *rs300, int *value);
static int rs300_set_autoshutter_params(struct rs300 *rs300, int param_type, int value);
```

### Step 2: Implement Functions (rs300.c ~line 1400)
```c
static int rs300_set_autoshutter(struct rs300 *rs300, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(&rs300->sd);
    u8 params[12] = {0};

    params[0] = enable ? 0x01 : 0x00;

    dev_info(&client->dev, "Setting autoshutter: %s", enable ? "ON" : "OFF");

    return rs300_send_command(rs300, 0x10, 0x02, 0x41, params, 1, 500);
}
```

### Step 3: Add to Control Handler (rs300.c ~line 1336)
```c
case V4L2_CID_CUSTOM_BASE + 9:  // Autoshutter
    ret = rs300_set_autoshutter(rs300, ctrl->val);
    break;
```

### Step 4: Register Control (rs300.c ~line 2236)
```c
rs300->autoshutter_ctrl = v4l2_ctrl_new_custom(&rs300->ctrl_handler,
                                                &autoshutter_ctrl, NULL);
```

### Step 5: Test
```bash
# Load driver
sudo rmmod rs300
sudo modprobe rs300

# Test new control
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls | grep -i auto
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter=1

# Check dmesg
dmesg | tail -20
```

### Step 6: Document
- Update `DEV_QUICK_REFERENCE.md` with new controls
- Update `I2C_PROTOCOL.md` if needed
- Add examples to docs

---

## Summary: Implementation Order

**Total Commands Identified**: 18 commands (excluding already-implemented ones)

**Recommended Implementation Order**:

1. **Batch 1** (3 cmds): Autoshutter - 🔥 **START HERE**
2. **Batch 2** (2 cmds): Module Sleep
3. **Batch 3** (3 cmds): Device Information (focus on Get Device Name for auto-detection)
4. **Batch 4** (2 cmds): Gamma
5. **Batch 5** (3 cmds): Edge Position, Anti-burn, Hotspot
6. **Batch 6** (3 cmds): Boot Logo, Save/Restore

**Estimated Time per Batch**: 3-4 hours (implementation + testing)
**Total Estimated Time**: 18-24 hours (2-3 days)

---

## Already Implemented (12 commands)

✅ Set Brightness (0x10, 0x04, 0x47)
✅ Set Colormap (0x10, 0x03, 0x45)
✅ Trigger FFC/Shutter Cal (0x10, 0x02, 0x43)
✅ Set FPS (0x10, 0x10, 0x46)
✅ Set Zoom (0x01, 0x31, 0x42)
✅ Set Contrast (0x10, 0x04, 0x4A)
✅ Set DDE (0x10, 0x04, 0x45)
✅ Set Scene Mode (0x10, 0x04, 0x42)
✅ Set Spatial NR (0x10, 0x04, 0x4B)
✅ Set Temporal NR (0x10, 0x04, 0x4C)
✅ Start Streaming (via MIPI enable command)
✅ Stop Streaming (via MIPI disable command)

**Total Coverage**: 12 implemented + 18 planned = **30 commands** (out of ~80 total in CSV)

---

**Next Step**: Implement Batch 1 (Autoshutter) - 3 commands
