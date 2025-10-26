# RS300 Thermal Camera Driver

Intelligent navigation for Claude Code when working with this V4L2 driver.

---

## Project Identity

**What**: V4L2 driver for RS300 thermal camera (640×512@60fps) on Raspberry Pi
**Architecture**: Linux kernel driver via MIPI CSI-2 + I2C
**Platform**: Raspberry Pi 5 (BCM2712/RP1-CFE)
**Status**: Beta - Security fixes applied (2025-10-22), production testing recommended
**Documentation**: 70 .md files (~24,000 lines) in docs/, .claude/, and docs/reference/

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
- Or enable systemd/udev auto-config (see docs/reference/BOOT_CONFIGURATION.md)

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

**NEW SESSION - Required Reading** (1-2 minutes):

1. **SESSION_STATE.md** (.claude/SESSION_STATE.md) - Read first
   - Current work status
   - Blockers and open questions
   - Recent decisions with rationale

2. **START_HERE.md** (docs/START_HERE.md) - Human-readable status
   - Current phase
   - Quick commands

**ENDING SESSION - Required Actions**:

1. Run `/update-docs` command (automatic before commits)
   - Updates SESSION_STATE.md
   - Generates session note in .claude/sessions/
   - Validates documentation links

2. Commit work if applicable:
   ```bash
   git add [files]
   git commit -m "descriptive message"
   ```

3. Update SESSION_STATE.md manually if:
   - Tasks are half-done (update "In-Progress Work")
   - You're blocked on something (add to "Blockers & Questions")
   - Important decisions made (document in "Recent Decisions")

**Session Checklists**: See .claude/templates/SESSION_HANDOFF_TEMPLATE.md

---

## Key Documentation Paths

**Session Management**:
- .claude/SESSION_STATE.md - AI-optimized current state
- docs/START_HERE.md - Human-readable project status

**Getting Started**:
- docs/getting-started/installation-pi5.md - Pi 5 installation
- docs/getting-started/first-capture.md - Quick start tutorial
- docs/README.md - Complete documentation hub

**Technical Reference**:
- docs/reference/DRIVER_ANALYSIS.md - Complete driver internals (~600 lines)
- docs/reference/I2C_PROTOCOL.md - I2C command specification (~550 lines)
- docs/reference/DEV_QUICK_REFERENCE.md - Command cheat sheets (~450 lines)
- docs/reference/TROUBLESHOOTING.md - Debug procedures (~600 lines)

**Code & Tests**:
- rs300.c - Main driver (2,946 lines)
- ./test_controls.sh - Automated control testing

---

## Media Pipeline Quick Reference (Pi 5)

**Automated Configuration** (recommended):
```bash
./configure_media.sh
```

**Manual Configuration** (for reference):
```bash
# Link entities
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"

# Set formats (MUST use UYVY8_1X16 - 16-bit packed)
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512]"

# Configure video device
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY
```

**See**: docs/reference/RS300_Media_Pipeline_Guide.md for complete details

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

**For complete documentation**: See docs/README.md or run `/update-docs` for system health check.
