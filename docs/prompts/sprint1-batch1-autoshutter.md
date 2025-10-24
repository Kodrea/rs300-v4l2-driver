# Sprint 1 Batch 1: Implement Autoshutter Commands

**Context**: This is the first batch of commands in the RS300 driver modernization plan.

**Goal**: Implement automatic FFC (Flat Field Calibration) functionality with 3 I2C commands and 4 V4L2 controls.

---

## Background

Read these files first:
1. **[I2C_COMMANDS_TO_IMPLEMENT.md](../../I2C_COMMANDS_TO_IMPLEMENT.md)** - Section "Batch 1: Autoshutter Commands" (lines 28-114)
2. **[MODERNIZATION_PLAN.md](../../MODERNIZATION_PLAN.md)** - Sprint 1, Days 1-2 (lines 614-621)
3. **[DRIVER_ANALYSIS.md](../../DRIVER_ANALYSIS.md)** - Section 4 (V4L2 Controls) for existing patterns

---

## What to Implement

### 3 I2C Command Functions:

1. **`rs300_set_autoshutter(struct rs300 *rs300, int enable)`**
   - I2C: Class 0x10, Module 0x02, SubCmd 0x41
   - Parameter: P1=0 (off) or 1 (on)
   - Enable/disable automatic FFC

2. **`rs300_get_autoshutter(struct rs300 *rs300, int *value)`**
   - I2C: Class 0x10, Module 0x02, SubCmd 0x81
   - Parameters: P9=0x01, Len=0x0001
   - Read autoshutter state

3. **`rs300_set_autoshutter_params(struct rs300 *rs300, int param_type, int value)`**
   - I2C: Class 0x10, Module 0x02, SubCmd 0x42
   - Parameters:
     - param_type=0: Temperature threshold (P1[2:1] = value in 1/32°C units)
     - param_type=1: Minimum interval (P1[2:1] = seconds)
     - param_type=2: Maximum interval (P1[2:1] = seconds)

### 4 V4L2 Controls:

1. **Autoshutter Enable** (Boolean, 0-1)
2. **Autoshutter Temperature** (Integer, 10-100, default 50)
3. **Autoshutter Min Interval** (Integer, 1-300s, default 1)
4. **Autoshutter Max Interval** (Integer, 1-600s, default 120)

---

## Implementation Steps

### Step 1: Add Function Prototypes
Location: `rs300.c` around line 185 (after existing prototypes)

```c
static int rs300_set_autoshutter(struct rs300 *rs300, int enable);
static int rs300_get_autoshutter(struct rs300 *rs300, int *value);
static int rs300_set_autoshutter_params(struct rs300 *rs300, int param_type, int value);
```

### Step 2: Implement Functions
Location: `rs300.c` around line 1400-1500 (after existing control functions)

Follow the pattern from existing commands like `rs300_brightness_correct()` (line ~1161).

**Key points:**
- Use `rs300_send_command()` helper (if available) or follow existing pattern
- Use `dev_info()` for logging
- Return 0 on success, negative error code on failure
- For GET command: use `read_regs()` to read result buffer

### Step 3: Add Control Definitions
Location: `rs300.c` around line 2300 (after existing control configs)

```c
static const struct v4l2_ctrl_config autoshutter_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 9,
    .name = "Auto Shutter",
    .type = V4L2_CTRL_TYPE_BOOLEAN,
    .min = 0,
    .max = 1,
    .step = 1,
    .def = 0,
};

static const struct v4l2_ctrl_config autoshutter_temp_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 10,
    .name = "Auto Shutter Temperature",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 10,
    .max = 100,
    .step = 1,
    .def = 50,
};

static const struct v4l2_ctrl_config autoshutter_min_interval_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 11,
    .name = "Auto Shutter Min Interval",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 1,
    .max = 300,
    .step = 1,
    .def = 1,
};

static const struct v4l2_ctrl_config autoshutter_max_interval_ctrl = {
    .ops = &rs300_ctrl_ops,
    .id = V4L2_CID_CUSTOM_BASE + 12,
    .name = "Auto Shutter Max Interval",
    .type = V4L2_CTRL_TYPE_INTEGER,
    .min = 1,
    .max = 600,
    .step = 1,
    .def = 120,
};
```

### Step 4: Add to Control Handler
Location: `rs300.c` in `rs300_set_ctrl()` function (around line 1336)

Add new cases in the switch statement:

```c
case V4L2_CID_CUSTOM_BASE + 9:  // Autoshutter enable
    ret = rs300_set_autoshutter(rs300, ctrl->val);
    break;

case V4L2_CID_CUSTOM_BASE + 10:  // Autoshutter temperature
    ret = rs300_set_autoshutter_params(rs300, 0, ctrl->val);
    break;

case V4L2_CID_CUSTOM_BASE + 11:  // Autoshutter min interval
    ret = rs300_set_autoshutter_params(rs300, 1, ctrl->val);
    break;

case V4L2_CID_CUSTOM_BASE + 12:  // Autoshutter max interval
    ret = rs300_set_autoshutter_params(rs300, 2, ctrl->val);
    break;
```

### Step 5: Register Controls
Location: `rs300.c` in `rs300_init_controls()` function (around line 2236)

Add after existing control registrations:

```c
rs300->autoshutter_ctrl = v4l2_ctrl_new_custom(&rs300->ctrl_handler,
                                                &autoshutter_ctrl, NULL);
rs300->autoshutter_temp_ctrl = v4l2_ctrl_new_custom(&rs300->ctrl_handler,
                                                     &autoshutter_temp_ctrl, NULL);
rs300->autoshutter_min_interval_ctrl = v4l2_ctrl_new_custom(&rs300->ctrl_handler,
                                                             &autoshutter_min_interval_ctrl, NULL);
rs300->autoshutter_max_interval_ctrl = v4l2_ctrl_new_custom(&rs300->ctrl_handler,
                                                             &autoshutter_max_interval_ctrl, NULL);
```

### Step 6: Add Control Pointers to Struct
Location: `rs300.c` in `struct rs300` (around line 222)

Add after existing control pointers:

```c
struct v4l2_ctrl *autoshutter_ctrl;
struct v4l2_ctrl *autoshutter_temp_ctrl;
struct v4l2_ctrl *autoshutter_min_interval_ctrl;
struct v4l2_ctrl *autoshutter_max_interval_ctrl;
```

---

## Testing Procedure

### Build and Load
```bash
# Build driver
make clean && make

# Reload driver
sudo rmmod rs300
sudo insmod rs300.ko

# Check dmesg for errors
dmesg | tail -30
```

### Test Controls
```bash
# List new controls
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls | grep -i "auto\|shutter"

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

### Verify Functionality
1. Enable autoshutter
2. Wait and observe for automatic FFC triggers
3. Check dmesg for command execution logs
4. Test different temperature thresholds and intervals

---

## Success Criteria

- [ ] Driver compiles without errors
- [ ] Driver loads without kernel errors
- [ ] 4 new controls appear in `v4l2-ctl --list-ctrls`
- [ ] Autoshutter can be enabled/disabled
- [ ] Temperature threshold can be set (10-100)
- [ ] Min/max intervals can be set
- [ ] FFC automatically triggers based on settings
- [ ] No kernel crashes or timeouts

---

## Documentation Updates

After implementation, update:
1. **DEV_QUICK_REFERENCE.md** - Add new controls to V4L2 Control Reference section
2. **I2C_COMMANDS_TO_IMPLEMENT.md** - Mark Batch 1 as complete
3. **SESSION_STATE.md** - Update "In-Progress Work" section

---

## Hex Command Reference

For debugging, these are the expected I2C commands:

```
Enable autoshutter:
10 02 41 00 01 00 00 00 00 00 00 00 00 00 00 00 78 3D

Disable autoshutter:
10 02 41 00 00 00 00 00 00 00 00 00 00 00 00 00 0D 3E

Set temp threshold to 50 (1.56°C):
10 02 42 00 00 32 00 00 00 00 00 00 00 00 00 00 5A EC

Set min interval to 1s:
10 02 42 00 01 01 00 00 00 00 00 00 00 00 00 00 92 68

Set max interval to 120s:
10 02 42 00 02 78 00 00 00 00 00 00 00 00 00 00 58 AC

Get autoshutter state:
10 02 81 00 00 00 00 00 00 00 00 00 01 00 00 00 78 34
```

---

## Notes

- Autoshutter triggers FFC based on detector temperature change (not scene temperature)
- Temperature threshold is change threshold (e.g., 50 = 1.56°C change triggers FFC)
- Min interval prevents too-frequent FFC (e.g., not faster than every 5s)
- Max interval ensures periodic FFC even without temperature change (e.g., at least every 120s)
- Follow existing code patterns from `rs300_shutter_cal()` and `rs300_brightness_correct()`

---

## Estimated Time

6-8 hours total:
- Function implementation: 3-4 hours
- Control registration: 1-2 hours
- Testing and debugging: 2-3 hours

---

## Next Batch

After completing and testing Batch 1, proceed to:
**Sprint 1, Batch 2: Module Sleep Commands** (Days 3-4)
