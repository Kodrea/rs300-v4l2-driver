# Sprint 1, Batch 1: Implement Autoshutter Commands

Implement the autoshutter I2C commands for the RS300 thermal camera driver.

## Context

This is **Sprint 1, Days 1-2** of the driver modernization plan. I need to implement **Batch 1: Autoshutter Commands** (3 commands) as the highest priority feature requested by the user.

## Task

Implement the following I2C commands in `rs300.c`:
1. `rs300_set_autoshutter()` - Enable/disable automatic FFC
2. `rs300_get_autoshutter()` - Read autoshutter state
3. `rs300_set_autoshutter_params()` - Configure temperature threshold and time intervals

Add 4 new V4L2 controls for user access to these features.

## Documentation References

**Start here**: Read these sections for complete implementation details:
1. **[I2C_COMMANDS_TO_IMPLEMENT.md](../../I2C_COMMANDS_TO_IMPLEMENT.md)** - Section "Batch 1: Autoshutter Commands"
   - Contains exact I2C command parameters (Class, Module, SubCmd)
   - V4L2 control configurations (ready to copy-paste)
   - Hex command examples
   - Testing procedures
   - Implementation workflow (6 steps)

2. **[MODERNIZATION_PLAN.md](../../MODERNIZATION_PLAN.md)** - Sprint 1, Days 1-2
   - High-level sprint context
   - Timeline: 6-8 hours estimated

3. **[DRIVER_ANALYSIS.md](../../DRIVER_ANALYSIS.md)** - Section 4 (V4L2 Controls)
   - Understand existing control patterns
   - Code locations with line numbers

## Implementation Checklist

Follow the workflow in I2C_COMMANDS_TO_IMPLEMENT.md:

- [ ] **Step 1**: Add function prototypes (~line 185 in rs300.c)
- [ ] **Step 2**: Implement the 3 functions (~line 1400)
  - `rs300_set_autoshutter()`
  - `rs300_get_autoshutter()`
  - `rs300_set_autoshutter_params()`
- [ ] **Step 3**: Add to control handler (~line 1336 in `rs300_set_ctrl()`)
- [ ] **Step 4**: Register 4 controls (~line 2236 in `rs300_init_controls()`)
  - Main autoshutter switch (boolean)
  - Temperature threshold (0-100)
  - Min interval (1-300 seconds)
  - Max interval (1-600 seconds)
- [ ] **Step 5**: Test with v4l2-ctl commands
- [ ] **Step 6**: Update documentation (DEV_QUICK_REFERENCE.md)

## Testing Commands

After implementation:
```bash
# Enable autoshutter
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter=1

# Set temperature threshold to 50 (1.56°C)
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter_temperature=50

# Set intervals
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter_min_interval=5
v4l2-ctl -d /dev/v4l-subdev2 -c autoshutter_max_interval=300

# Get current state
v4l2-ctl -d /dev/v4l-subdev2 --get-ctrl=autoshutter
```

## Success Criteria

- [ ] All 3 functions implemented and compile without errors
- [ ] 4 V4L2 controls registered and visible in `v4l2-ctl --list-ctrls`
- [ ] Autoshutter can be enabled/disabled
- [ ] Parameters can be set and read back correctly
- [ ] dmesg shows no errors during operation
- [ ] Documentation updated

## Estimated Time

6-8 hours total

## After Completion

When finished, move to **Sprint 1, Days 3-4: Batch 2 (Module Sleep Commands)**.

See I2C_COMMANDS_TO_IMPLEMENT.md for Batch 2 details.
