# RS300 Thermal Camera Driver

Intelligent navigation for Claude Code when working with this V4L2 driver.

---

## Project Identity

**What**: V4L2 driver for RS300 thermal camera (640×512@60fps) on Raspberry Pi
**Architecture**: Linux kernel driver via MIPI CSI-2 + I2C
**Platform**: Raspberry Pi 5 (BCM2712/RP1-CFE)
**Status**: Beta - Security fixes applied (2025-10-22), production testing recommended
**Documentation**: Minimal .md files in docs/ and .claude/, GitHub used for issue tracking and status

---

## Critical Platform Quirks

**Pi 5 Format Compatibility** (CRITICAL):
- RP1-CFE **only supports 16-bit packed formats** (`UYVY8_1X16`, `YUYV8_1X16`)
- 8-bit dual-lane formats (`*8_2X8`) will cause `"Format mismatch!"` errors
- Always use 16-bit packed formats in pipeline configuration

**Hardware Error Retry Logic**:
- Camera reports 0x0e status error on ~25% of stream starts (hardware quirk)
- Driver implements 3-attempt retry with exponential backoff (commit eb99791)
- 100% success rate achieved in testing (25/25 tests)

**Pipeline Persistence**:
- Media pipeline configuration **does not persist across reboots**
- Must run `./configure_media.sh` after each reboot
- Or enable systemd/udev auto-config (see docs/reference/SETUP_AND_TROUBLESHOOTING.md)

**I2C Communication**:
- I2C bus: `i2c-10` (Pi 5), `i2c-1` (Pi 4)
- Device address: `0x3c`
- 18-byte packet structure with CRC-16-CCITT checksum

---

## Essential Commands

```bash
# Installation & Setup
./setup.sh                         # Install driver (DKMS)
sudo reboot                        # Required after install
./configure_media.sh               # Configure media pipeline (Pi 5)

# Testing & Verification
dmesg | grep rs300                 # Check driver logs
lsmod | grep rs300                 # Verify driver loaded
i2cdetect -y 10                    # Check I2C device (Pi 5)
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls  # List all controls

# Quick Capture Test
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 --stream-to=test.yuv
```

**See**: docs/reference/DEV_QUICK_REFERENCE.md for complete command reference

---

## Code Style & Gotchas

**CRITICAL RULES (Kernel Crash Prevention)**:
- ❌ **NEVER modify existing function signatures** (especially adding parameters)
- ❌ **NEVER attempt code consolidation** of working command functions
- ❌ **NEVER refactor** just for code aesthetics in kernel drivers
- ✅ **Accept ~500 lines of duplicate code** - consolidation abandoned after kernel crashes
- ✅ **Use parameter structs** if you must add function parameters
- ✅ **Create new functions** rather than modifying existing ones

**See**: .claude/lessons-learned/002-consolidation-kernel-crash.md for detailed case study

**Code Patterns to Follow**:
- All camera commands follow same structure (see docs/reference/DRIVER_ANALYSIS.md Section 3.3)
- Match existing error handling patterns (`dev_info`, `dev_err`)
- Maintain consistent debug logging style
- Test with `./test_controls.sh` after any changes

**Known Code Quirks**:
- Zoom command uses hardcoded CRC instead of calculation (rs300.c:1486-1487)
- Mode switching requires driver reload (runtime switching not implemented)
- ~500 lines of duplicate command execution code (intentionally kept)

---

## Session Handoff Protocol

**NEW SESSION - Required Reading** (2 minutes):

1. **SESSION_STATE.md** (.claude/SESSION_STATE.md) - Current phase, blockers, next steps
2. **GitHub Project Board** - Visual task status and linked issues
3. **docs/README.md** - Reference guide to essential documentation

**ENDING SESSION - Required Actions**:

1. Update SESSION_STATE.md with current phase/progress
2. Commit changes with descriptive message
3. Create GitHub issues for any new blockers/tasks discovered

---

## Documentation Structure

**Entry Points**:
- `.claude/SESSION_STATE.md` - Session status and next steps
- `CLAUDE.md` (this file) - Project guide and critical quirks

**Essential References**:
- `docs/reference/SETUP_AND_TROUBLESHOOTING.md` - Setup & diagnostics
- `docs/reference/DRIVER_ANALYSIS.md` - Driver internals
- `docs/reference/DEV_QUICK_REFERENCE.md` - Command cheat sheets
- `docs/reference/RS300_Media_Pipeline_Guide.md` - Pipeline architecture
- `docs/reference/SECURITY_AUDIT.md` - Security analysis

**Setup Guides**:
- `docs/getting-started/installation-pi5.md` - Pi 5 driver install
- `docs/getting-started/first-capture.md` - Quick start test

**Lessons Learned**:
- `.claude/lessons-learned/` - Documented solutions and design decisions

**Archived Docs**:
- `/archive/docs-old/` - Outdated docs preserved for reference only

**Code & Tests**:
- rs300.c - Main driver (2,946 lines)
- ./test_controls.sh - Automated control testing

---

## Quick Setup (Pi 5)

After installing driver (`./setup.sh`) and rebooting:

```bash
./configure_media.sh        # Configure media pipeline
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100 --stream-to=test.yuv
```

See `docs/reference/SETUP_AND_TROUBLESHOOTING.md` for detailed configuration options.

---

## V4L2 Controls

**11 Available Controls**:
- `brightness` (0-100) - Thermal brightness level
- `contrast` (0-100) - Image contrast
- `zoom_absolute` (1-8) - Digital zoom
- `colormap` (0-11) - Color palette (White Hot, Ironbow, etc.)
- `ffc_trigger` (button) - Flat field calibration
- `scene_mode` (0-9) - Scene optimization
- `digital_detail_enhancement` (0-100) - Edge enhancement
- `spatial_noise_reduction` (0-100) - Spatial noise filter
- `temporal_noise_reduction` (0-100) - Temporal noise filter
- `pixel_rate` (read-only) - Pixel clock rate
- `link_freq` (read-only) - MIPI link frequency

**See**: docs/reference/DEV_QUICK_REFERENCE.md for complete tables with all options

---

## Known Issues & Status

**Security Status** (2025-10-22):
- ✅ All 6 CRITICAL/HIGH vulnerabilities fixed (ioctl, NULL checks, race conditions)
- See docs/reference/SECURITY_AUDIT.md for complete analysis

**Recent Fixes**:
- ✅ rp1-cfe deadlock resolved via retry logic (commit eb99791)
- ✅ Driver reload now safe (regulator cleanup fixed)

**Current Limitations**:
- Mode switching requires driver reload (runtime switching not implemented)
- Pipeline configuration doesn't persist across reboots (use auto-config)
- Zoom command uses hardcoded CRC (works, but not ideal)

---

**For complete documentation**: See `docs/README.md` - Start here for technical reference.

**Important**: Always rebuild driver via `setup.sh` - never use `make` or `insmod` directly.