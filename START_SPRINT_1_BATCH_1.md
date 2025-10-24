# 🚀 START HERE: Sprint 1 Batch 1 - Autoshutter Implementation

**Quick Start Prompt for New Claude Code Session**

---

## Copy-Paste This Prompt:

```
I'm working on the RS300 thermal camera driver modernization project. I need to implement Sprint 1, Batch 1: Autoshutter Commands.

Please read these files for context:
1. SESSION_STATE.md - Current project state
2. I2C_COMMANDS_TO_IMPLEMENT.md - Full command details (focus on Batch 1)
3. docs/prompts/sprint1-batch1-autoshutter.md - Detailed implementation guide

Then implement the following in rs300.c:
- 3 I2C command functions (rs300_set_autoshutter, rs300_get_autoshutter, rs300_set_autoshutter_params)
- 4 V4L2 controls (autoshutter enable, temperature, min interval, max interval)
- Control registration and handler integration

Follow the existing code patterns from rs300_brightness_correct() and rs300_shutter_cal().

After implementation:
1. Build and test the driver
2. Verify all 4 controls appear in v4l2-ctl --list-ctrls
3. Test autoshutter functionality
4. Update documentation (DEV_QUICK_REFERENCE.md)

This is user priority #1 - autoshutter for automatic FFC.
```

---

## Alternative: More Detailed Prompt

If you want to be more specific:

```
I'm implementing Sprint 1, Batch 1 of the RS300 driver modernization plan: Autoshutter Commands.

**Context Files** (read these first):
- SESSION_STATE.md - Shows current project state and last commit
- I2C_COMMANDS_TO_IMPLEMENT.md - Lines 28-114 (Batch 1 details)
- docs/prompts/sprint1-batch1-autoshutter.md - Complete implementation guide
- DRIVER_ANALYSIS.md - Section 4 for V4L2 control patterns

**Goal**: Add automatic FFC (Flat Field Calibration) with these components:

**Functions to implement** (in rs300.c):
1. rs300_set_autoshutter(struct rs300 *rs300, int enable)
   - I2C: 0x10 0x02 0x41, P1=0/1
2. rs300_get_autoshutter(struct rs300 *rs300, int *value)
   - I2C: 0x10 0x02 0x81, returns state
3. rs300_set_autoshutter_params(struct rs300 *rs300, int param_type, int value)
   - I2C: 0x10 0x02 0x42, configures temp/intervals

**V4L2 Controls to add**:
- Autoshutter Enable (boolean, 0-1)
- Autoshutter Temperature (int, 10-100, default 50)
- Autoshutter Min Interval (int, 1-300s, default 1)
- Autoshutter Max Interval (int, 1-600s, default 120)

**Steps**:
1. Add function prototypes (~line 185)
2. Implement functions (~line 1400)
3. Define control configs (~line 2300)
4. Add handler cases in rs300_set_ctrl() (~line 1336)
5. Register controls in rs300_init_controls() (~line 2236)
6. Add control pointers to struct rs300 (~line 222)

**Testing**:
- Build: make clean && make
- Load: sudo rmmod rs300 && sudo insmod rs300.ko
- Test: v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls | grep -i auto
- Verify: Enable autoshutter and observe automatic FFC triggers

**Documentation**:
After implementation, update DEV_QUICK_REFERENCE.md with new controls.

Follow existing patterns from rs300_brightness_correct() (line ~1161) and rs300_shutter_cal() (line ~1213).

**Estimated time**: 6-8 hours
**User priority**: #1 (highest)
```

---

## Files You Created

All necessary guidance is in:
- **docs/prompts/sprint1-batch1-autoshutter.md** - Complete implementation guide
- **I2C_COMMANDS_TO_IMPLEMENT.md** - Batch 1 section with hex commands and V4L2 control specs
- **MODERNIZATION_PLAN.md** - Sprint 1 breakdown showing this is Days 1-2

---

## Quick Reference Commands

After clearing context, these commands will help orient a new Claude session:

```bash
# See current project state
cat SESSION_STATE.md

# See what needs to be done
cat docs/prompts/sprint1-batch1-autoshutter.md

# See full I2C command details
cat I2C_COMMANDS_TO_IMPLEMENT.md | head -150

# See existing V4L2 control patterns
grep -A 20 "rs300_brightness_correct" rs300.c
```

---

## Success Indicators

You'll know Batch 1 is complete when:
- ✅ Driver compiles without errors
- ✅ 4 new autoshutter controls visible in v4l2-ctl
- ✅ Autoshutter can be enabled/disabled
- ✅ Parameters can be configured
- ✅ FFC triggers automatically based on temperature changes
- ✅ Documentation updated

---

## Next Steps After Batch 1

Once Batch 1 is complete and tested, proceed to:
**Sprint 1, Batch 2: Module Sleep Commands** (2 commands, 3-4 hours)

See I2C_COMMANDS_TO_IMPLEMENT.md Batch 2 section.
