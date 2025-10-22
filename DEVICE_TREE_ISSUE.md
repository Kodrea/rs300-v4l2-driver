# Device Tree Initialization Issue

**Date Discovered**: October 22, 2025
**Status**: UNRESOLVED - Documented for future investigation
**Impact**: Prevents camera device instantiation after reboot (requires investigation)
**Priority**: Medium (workaround: reboot loads it correctly initially)

---

## Problem Description

The RS300 thermal camera device is **not being instantiated** by the device tree overlay after certain reboots, despite the overlay being properly configured and the driver module loading successfully.

### Symptoms

1. **Module loads but no device created**:
   ```bash
   $ lsmod | grep rs300
   rs300    49152  0    # Module is loaded ✓

   $ media-ctl -e "rs300"
   Entity 'rs300' not found    # But no device created ✗
   ```

2. **I2C address claimed by unknown entity**:
   ```bash
   $ i2cdetect -y 10
   [...]
   30: -- -- -- -- -- -- -- -- -- -- -- -- UU -- -- --    # UU = in use

   $ sudo i2cget -y 10 0x3c 0x00
   Error: Device or resource busy    # Something has 0x3c but not rs300 driver
   ```

3. **No device tree node created**:
   ```bash
   $ ls /proc/device-tree/soc/i2c@7d0d5c000/rs300@3c
   ls: cannot access [...]: No such file or directory    # Device node missing
   ```

4. **Media controller has no RS300 entity**:
   ```bash
   $ media-ctl -d /dev/media0 -p | grep rs300
   [no output]    # Camera not in media topology
   ```

### Configuration Verified Correct

```bash
# Overlay enabled in config.txt:
$ cat /boot/firmware/config.txt | grep rs300
dtoverlay=rs300    ✓

# Overlay file exists and is valid:
$ ls /boot/firmware/overlays/rs300.dtbo
/boot/firmware/overlays/rs300.dtbo    ✓

# Overlay is for BCM2712 (Pi 5):
$ sudo dtc -I dtb -O dts /boot/firmware/overlays/rs300.dtbo | grep compatible
compatible = "brcm,bcm2712";    ✓
```

---

## Impact Assessment

**Functionality**: When this occurs, the camera is completely non-functional:
- No video capture possible
- No V4L2 controls accessible
- Media pipeline configuration fails
- Camera hardware inaccessible

**Workaround**: **Reboot** - On fresh boot, device tree usually instantiates correctly

**Frequency**: Unknown - observed after one specific reboot, needs more testing to determine reproducibility

**Security Impact**: **NONE** - This is separate from the security vulnerabilities which have been fixed

---

## Investigation Notes

### What Works
- ✅ Driver compiles and installs correctly (DKMS)
- ✅ Module loads without errors at boot (`modprobe rs300` succeeds)
- ✅ Overlay file is syntactically valid (dtc compiles it)
- ✅ Configuration files are correct

### What Fails
- ✗ Device instantiation (no probe() call or probe fails silently)
- ✗ I2C device binding (address claimed by something else?)
- ✗ Media controller registration

### Possible Root Causes

1. **Boot Timing Race Condition**:
   - Device tree overlay loads before I2C controller ready?
   - Driver probe deferred and never retried?
   - Check: `dmesg | grep "deferred probe"`

2. **I2C Address Conflict**:
   - Another driver claiming 0x3c before rs300?
   - Camera HAT EEPROM or other device at 0x3c?
   - Check: `cat /sys/bus/i2c/devices/10-003c/name` (if exists)

3. **Device Tree Overlay Loading Failure**:
   - Overlay not actually applied despite config.txt entry?
   - Overlay applied but fragment conditions not met?
   - Check: `vcgencmd get_config dtoverlay` output

4. **Probe Function Failure**:
   - probe() called but fails silently?
   - Error not logged to dmesg?
   - Check: Add debug prints to rs300_probe()

5. **Power/Regulator Issue**:
   - Camera regulators not available at probe time?
   - Probe deferred waiting for regulators that never appear?
   - Check: `ls /sys/class/regulator/` for cam1_reg

---

## Diagnostic Procedure

When this issue occurs, run these commands to gather data:

```bash
# 1. Check if device tree node exists
ls -la /proc/device-tree/soc/i2c@7d0d5c000/rs300@3c
# Expected: Directory with properties (name, compatible, reg, etc.)
# If missing: Device tree overlay didn't create the node

# 2. Check what has I2C address 0x3c
cat /sys/bus/i2c/devices/10-003c/name 2>/dev/null
# Expected: "rs300" if driver bound
# If different: Another driver claimed the address first

# 3. Check for deferred probes
dmesg | grep -i "deferred\|rs300" | tail -30
# Look for: "probe deferred", "no such device", timing issues

# 4. Check overlay loading
vcgencmd get_config dtoverlay | grep rs300
# Expected: "dtoverlay=rs300" (or might show nothing if applied)

# 5. Check all I2C devices on bus 10
ls /sys/bus/i2c/devices/ | grep "10-"
# Look for: 10-003c should exist

# 6. Check driver binding
ls /sys/bus/i2c/drivers/rs300/
# Expected: Directory with "10-003c" symlink if bound

# 7. Manually trigger probe (if device exists but unbound)
echo 10-003c > /sys/bus/i2c/drivers/rs300/bind
# This will show if probe() is failing
```

---

## Potential Fixes to Test

### Fix 1: Ensure I2C Bus is Ready
Add to device tree overlay:
```dts
&i2c10 {
    status = "okay";
    clock-frequency = <400000>;  // Ensure bus is configured
};
```

### Fix 2: Add Probe Deferral Handling
Check in rs300_probe():
```c
// If regulator_bulk_get fails with -EPROBE_DEFER
if (ret == -EPROBE_DEFER) {
    dev_info(dev, "Probe deferred, waiting for dependencies");
    return ret;  // Allow deferred probe mechanism to retry
}
```

### Fix 3: Explicit Module Loading Order
Create `/etc/modules-load.d/rs300.conf`:
```
i2c-dev
i2c-bcm2835
rs300
```

### Fix 4: Verify Overlay Fragment Targets
Check that overlay fragments point to correct nodes:
```bash
# Decompile and verify phandles resolve correctly
sudo dtc -I dtb -O dts /boot/firmware/overlays/rs300.dtbo | grep "target ="
```

### Fix 5: Add Debug Logging
Temporarily modify rs300_probe() to add extensive logging:
```c
static int rs300_probe(struct i2c_client *client) {
    dev_err(&client->dev, "=== RS300 PROBE START ===");
    dev_err(&client->dev, "I2C adapter: %s", client->adapter->name);
    dev_err(&client->dev, "I2C address: 0x%02x", client->addr);
    // ... existing code with added logging at each step
}
```

---

## Comparison with Working State

When working correctly (after successful boot):
```bash
$ media-ctl -d /dev/media0 -p | grep rs300
- entity 16: rs300 10-003c (2 pads, 2 links)

$ ls /sys/bus/i2c/devices/10-003c/
driver  modalias  name  of_node  power  subsystem  uevent

$ cat /sys/bus/i2c/devices/10-003c/name
rs300

$ v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls
# Shows all 11 RS300 controls
```

---

## Testing Recommendations

1. **Reproducibility Test**:
   - Perform multiple reboots
   - Document success/failure rate
   - Check for pattern (cold boot vs warm reboot)

2. **Timing Test**:
   - Add `initcall_debug` to kernel command line
   - Check probe timing relative to I2C bus initialization
   - Look for dependencies not being met

3. **Isolation Test**:
   - Remove all other overlays
   - Test with minimal config.txt
   - Determine if conflict with other hardware

4. **Manual Binding Test**:
   - Boot without overlay
   - Manually instantiate I2C device:
     ```bash
     echo rs300 0x3c > /sys/bus/i2c/devices/i2c-10/new_device
     ```
   - Check if probe succeeds

---

## Related Files

- **Overlay Source**: `rs300-overlay.dts`
- **Compiled Overlay**: `/boot/firmware/overlays/rs300.dtbo`
- **Boot Config**: `/boot/firmware/config.txt`
- **Driver Source**: `rs300.c` (probe function at line ~2570)
- **Configuration Script**: `configure_media.sh` (expects rs300 to exist)

---

## Status

**Priority**: Medium
**Assigned**: Future investigation
**Blocker**: No - workaround exists (reboot)
**Affects Security Fixes**: No - security fixes are independent

**Next Steps**:
1. Perform reproducibility testing across multiple reboots
2. Add debug logging to probe function
3. Check for I2C bus timing/ordering issues
4. Review device tree overlay fragment targets
5. Test manual device instantiation

---

**Last Updated**: October 22, 2025
**Reported By**: Automated testing during security fix validation
