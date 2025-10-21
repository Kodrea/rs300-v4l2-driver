# CLAUDE.md - RS300 Thermal Camera Driver

This file provides intelligent navigation and guidance for Claude Code (claude.ai/code) when working with this repository.

---

## Project Identity

**What**: V4L2 driver for RS300 thermal camera (640×512@60fps) on Raspberry Pi 5
**Architecture**: Linux kernel driver via MIPI CSI-2 + I2C
**Platform**: Raspberry Pi 5 (BCM2712/RP1-CFE)
**Status**: Production Ready (deadlock fix validated), 90 files, ~18,000 lines total

## ✅ Recent Fixes

### rp1-cfe Driver Deadlock (RESOLVED - 2025-10-21)
- **Symptom:** Camera intermittently reports hardware error status 0x0e (~25% of stream starts)
- **Impact:** Previously triggered upstream rp1-cfe driver deadlock, created stuck processes requiring reboot
- **Root Cause:** Upstream bug in rp1-cfe `csi2_stop_channel()` cleanup code
- **Fix:** Retry logic implemented (3 attempts, exponential backoff) - commit eb99791
- **Status:** ✅ **TESTED & VALIDATED** - 100% success rate (25/25 tests), zero stuck processes
- **Test Results:** See [~/rs300-test-results/TEST_SUMMARY.txt](~/rs300-test-results/TEST_SUMMARY.txt)
- **Documentation:** [ISSUE_SUMMARY_20251021.md](ISSUE_SUMMARY_20251021.md), [UPSTREAM_BUG_REPORT.md](UPSTREAM_BUG_REPORT.md)
- **Result:** Success rate improved from ~75% to 100% in testing, no stuck processes

## File Organization Principle

**Professional Documentation System**:
- **docs/** - Organized user & contributor guides (12 files, ~4,900 lines)
- **Root** - Technical deep-dives (DRIVER_ANALYSIS.md, I2C_PROTOCOL.md, etc.)

**Layered Access**:
Quick Start (docs/getting-started/) → Usage Guides (docs/guides/) → Technical Deep-Dives (root docs) → Source Code

Choose documentation depth based on task complexity. Start with user guides, escalate to technical docs only as needed.

## Quick Navigation

| Task | Start Here | Time |
|------|-----------|------|
| **Check project status** | [START_HERE.md](START_HERE.md) | 30s |
| **Test retry logic fix** | [POST_REBOOT_TESTING.md](POST_REBOOT_TESTING.md) | 25min |
| **Install the driver** | [docs/getting-started/](docs/getting-started/) | 10min |
| **First thermal capture** | [docs/getting-started/first-capture.md](docs/getting-started/first-capture.md) | 5min |
| **Learn camera controls** | [docs/guides/camera-controls.md](docs/guides/camera-controls.md) | 15min |
| **Browse all docs** | [docs/README.md](docs/README.md) | 2min |
| Quick command lookup | [DEV_QUICK_REFERENCE.md](DEV_QUICK_REFERENCE.md) | 30s |
| Understand driver internals | [DRIVER_ANALYSIS.md](DRIVER_ANALYSIS.md) | 30min |
| Fix broken feature | [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | 5min |
| I2C protocol details | [I2C_PROTOCOL.md](I2C_PROTOCOL.md) | 15min |
| ISP integration | [RASPBERRY_PI_ISP_GUIDE.md](RASPBERRY_PI_ISP_GUIDE.md) | 20min |
| Test controls | `./test_controls.sh` | 2min |
| Find any file | [FILE_INVENTORY.md](FILE_INVENTORY.md) | 1min |

---

## Documentation Hierarchy

### Tier 0: User Documentation (docs/) - Start Here for Most Tasks
**Purpose**: Installation, usage, contribution
**When**: First-time setup, learning to use the camera, contributing

**NEW: Professional docs/ Structure** (12 files, ~4,900 lines)

**[docs/README.md](docs/README.md)** - Documentation hub with complete navigation

**Getting Started** (docs/getting-started/):
- **[installation-pi5.md](docs/getting-started/installation-pi5.md)** - Pi 5 setup guide
- **[installation-pi4.md](docs/getting-started/installation-pi4.md)** - Pi 4 setup guide
- **[first-capture.md](docs/getting-started/first-capture.md)** - Quick start tutorial

**Hardware** (docs/hardware/):
- **[compatibility.md](docs/hardware/compatibility.md)** - Platform testing matrix
- **[specifications.md](docs/hardware/specifications.md)** - Complete tech specs
- **[purchasing-guide.md](docs/hardware/purchasing-guide.md)** - Where to buy

**User Guides** (docs/guides/):
- **[basic-usage.md](docs/guides/basic-usage.md)** - Streaming, recording, examples
- **[camera-controls.md](docs/guides/camera-controls.md)** - All 11 V4L2 controls

**Contributing** (docs/contributing/):
- **[CONTRIBUTING.md](docs/contributing/CONTRIBUTING.md)** - Contribution guidelines
- **[ROADMAP.md](docs/contributing/ROADMAP.md)** - Development roadmap
- **[CHANGELOG.md](docs/contributing/CHANGELOG.md)** - Project history

### Tier 1: Quick Reference (Read First)
**Purpose**: Fast answers without deep understanding
**When**: Daily development, testing, quick debugging

**1. [DEV_QUICK_REFERENCE.md](DEV_QUICK_REFERENCE.md)** (~450 lines)
- Command cheat sheets (v4l2-ctl, media-ctl, i2c)
- All 11 V4L2 controls with examples
- Colormap (0-11) and scene mode (0-9) tables
- Code location map
- Testing checklists

**2. [FILE_INVENTORY.md](FILE_INVENTORY.md)** (~350 lines)
- All 29 files categorized by type and date
- File purposes and relationships
- New vs. existing file tracking
- Recommended reading order

### Tier 2: Operational Guides (Read for Setup/Procedures)
**Purpose**: Complete step-by-step procedures
**When**: Initial setup, configuration, troubleshooting

**3. [README.md](README.md)**
- Project overview
- Quick start guide

**4. [BOOT_CONFIGURATION.md](BOOT_CONFIGURATION.md)** (~200 lines)
- Auto-start media pipeline at boot
- Systemd and udev setup options

**5. [RS300_Media_Pipeline_Guide.md](RS300_Media_Pipeline_Guide.md)** (~300 lines)
- Media controller concepts
- Pipeline configuration details

**6. [TROUBLESHOOTING.md](TROUBLESHOOTING.md)** (~600 lines)
- Quick diagnostics decision trees
- Common error messages with solutions
- Media pipeline issues
- I2C communication debugging
- Performance troubleshooting

### Tier 3: Deep Technical (Read for Complete Understanding)
**Purpose**: Comprehensive technical documentation
**When**: Modifying code, system integration, deep debugging

**7. [DRIVER_ANALYSIS.md](DRIVER_ANALYSIS.md)** ⭐ (~600 lines) **PRIMARY TECHNICAL REFERENCE**
- Complete driver architecture analysis
- Data structures with line numbers (rs300.c:303-340)
- I2C protocol deep-dive (18-byte packets, CRC-16)
- All 14 camera commands documented
- V4L2 integration (controls, formats, streaming)
- Video format and streaming management
- Code quality assessment
- Known issues and improvement opportunities

**8. [I2C_PROTOCOL.md](I2C_PROTOCOL.md)** (~550 lines)
- Complete I2C command specification
- 18-byte packet structure
- CRC-16-CCITT algorithm (C + Python)
- Command reference table (14 commands)
- Status register decoding
- Example transactions with hex dumps
- Python implementation using smbus2

**9. [RASPBERRY_PI_ISP_GUIDE.md](RASPBERRY_PI_ISP_GUIDE.md)** (~800 lines)
- Pi 5 ISP architecture (Front End + Back End)
- Why thermal cameras differ from Bayer sensors
- PiSP Backend (pispbe) integration for YUV processing
- Hardware-accelerated temporal noise reduction
- Dual-stream output (full-res + thumbnail)
- GStreamer and V4L2 M2M examples
- Performance comparison: direct vs ISP processing

### Tier 4: Source Code
**Purpose**: Implementation details
**When**: After reading Tier 3 analysis

**10. [rs300.c](rs300.c)** (2,946 lines)
- Main driver implementation
- **Read with DRIVER_ANALYSIS.md as your guide** (contains line numbers for all functions)

**11. [rs300-overlay.dts](rs300-overlay.dts)** (~150 lines)
- Device tree configuration for Pi 5

**12. [examples/](examples/)** (Scripts and code samples)
- `isp_processing_example.sh`: Interactive ISP demo
- `capture_for_isp.py`: Python capture + ISP info tool

---

## Task → Documentation Decision Tree

```
START: What are you trying to do?
│
├─ Need quick command?
│  └─► DEV_QUICK_REFERENCE.md (30 seconds)
│      Example: "How do I set brightness?"
│
├─ Something broken?
│  ├─► TROUBLESHOOTING.md → Quick Diagnostics (Section 1)
│  ├─► Check decision tree for your symptom
│  └─► If not solved → DRIVER_ANALYSIS.md (relevant section)
│      Example: "Video capture fails"
│
├─ Understand how driver works?
│  ├─► Quick overview? → DRIVER_ANALYSIS.md (Section 1 only, ~50 lines)
│  ├─► Specific feature? → DRIVER_ANALYSIS.md (targeted section)
│  └─► Complete understanding? → Full DRIVER_ANALYSIS.md (30 minutes)
│      Example: "How does streaming work?"
│
├─ Modify I2C communication?
│  ├─► Read: I2C_PROTOCOL.md (protocol spec)
│  ├─► Then: DRIVER_ANALYSIS.md (Section 2-3)
│  └─► Then: rs300.c (lines 205-267, 502-1624)
│      Example: "Add new camera command"
│
├─ Add new V4L2 control?
│  ├─► Read: DRIVER_ANALYSIS.md (Section 4)
│  ├─► Reference: DEV_QUICK_REFERENCE.md (existing controls)
│  └─► Modify: rs300.c (lines 1626-1680, 2512-2594)
│      Example: "Add temperature readout control"
│
├─ Test functionality?
│  └─► Run: ./test_controls.sh [--quick|--control <name>]
│      Example: "Verify all controls work"
│
├─ Find a specific file?
│  └─► Check: FILE_INVENTORY.md
│      Example: "Where is the test script?"
│
└─ Configure boot automation?
   └─► BOOT_CONFIGURATION.md → Choose systemd/udev/hybrid
       Example: "Pipeline should start automatically"
```

---

## Reading Depth Guidelines

### Skim Level (1-5 minutes)
- Read section headers only
- Scan tables and code examples
- Use Ctrl+F to find specific terms

**Good for**: Quick lookups, finding code locations

**Example**: "Where is the brightness control handler?"
→ Skim DRIVER_ANALYSIS.md → Section 4 → Table → rs300.c:1327-1456

### Standard Level (10-20 minutes)
- Read relevant sections fully
- Understand concepts, skip implementation details
- Review decision trees and tables

**Good for**: Understanding workflows, troubleshooting procedures

**Example**: "How do I debug I2C timeouts?"
→ Read TROUBLESHOOTING.md → Section 7 → Follow procedure

### Deep Level (30-60 minutes)
- Read complete documents
- Study code examples and algorithms
- Cross-reference between documents

**Good for**: Code modifications, system integration, architecture understanding

**Example**: "I need to refactor the command execution code"
→ Read DRIVER_ANALYSIS.md (Sections 2, 3, 7) → I2C_PROTOCOL.md → rs300.c

---

## Development Workflows

### Standard Development Cycle

1. **Install/Update Driver**: `./setup.sh` (handles DKMS build)
2. **Reboot System**: `sudo reboot`
3. **Configure Media Pipeline**: `./configure_media.sh`
4. **Verify Driver Loaded**: `dmesg | grep rs300`
5. **Quick Test**: `./test_controls.sh --quick` (2 minutes)
6. **Develop**: Make your changes
7. **Full Test**: `./test_controls.sh` (5 minutes)
8. **Capture Logs** (if issues): `./capture_logs.sh`

**Documentation**: See [DEV_QUICK_REFERENCE.md](DEV_QUICK_REFERENCE.md) for all command details

### Boot-Time Configuration

Media pipeline configuration does not persist across reboots. Options:

- **Manual (default)**: Run `./configure_media.sh` after each reboot
- **Automatic (systemd)**: Install systemd service via `./setup.sh`
- **Automatic (udev)**: Install udev rule via `./setup.sh`
- **Hybrid**: Both systemd + udev for maximum reliability

See [BOOT_CONFIGURATION.md](BOOT_CONFIGURATION.md) for detailed setup.

### Testing Workflow

```bash
# Quick sanity check (2 minutes)
./test_controls.sh --quick

# Full automated test (5 minutes, all 11 controls)
./test_controls.sh

# Test specific control
./test_controls.sh --control brightness

# Manual testing
v4l2-ctl -d /dev/v4l-subdev2 --list-ctrls  # List all controls
v4l2-ctl -d /dev/v4l-subdev2 -c brightness=75  # Test one
```

**Documentation**: Run `./test_controls.sh --help` for options

### Troubleshooting Workflow

```
Problem Occurs
    ↓
1. Run Quick Diagnostics (TROUBLESHOOTING.md → Section 1)
    ├─ lsmod | grep rs300
    ├─ i2cdetect -y 10
    ├─ media-ctl -p | grep ENABLED
    └─ dmesg | grep -i "rs300.*error"
    ↓
2. Check Common Issues (TROUBLESHOOTING.md → relevant section)
    ├─ Found? → Follow solution
    └─ Not found? → Continue
    ↓
3. Check Error Message Index (TROUBLESHOOTING.md → Section 10)
    ↓
4. Still stuck?
    ├─ Run: ./capture_logs.sh
    ├─ Check: DRIVER_ANALYSIS.md (relevant system)
    └─ If I2C related: I2C_PROTOCOL.md
```

### Code Modification Workflow

```
1. Understand the Feature
   └─► DRIVER_ANALYSIS.md (find relevant section via Table of Contents)

2. Locate Code
   └─► Use line numbers from DRIVER_ANALYSIS.md
   └─► Or DEV_QUICK_REFERENCE.md (Code Locations section)

3. Understand Context
   └─► Read surrounding functions in rs300.c
   └─► Check data structures (DRIVER_ANALYSIS.md Section 1.2)

4. Make Changes
   └─► Follow existing patterns (see DRIVER_ANALYSIS.md Section 3.3)
   └─► Consider refactoring (Section 7.2: ~500 lines of duplicate code)
   └─► Match existing error handling

5. Test Thoroughly
   └─► ./test_controls.sh
   └─► Manual verification
   └─► Check dmesg for errors

6. Update Documentation
   └─► Update relevant .md file
   └─► Update FILE_INVENTORY.md if new files added
```

---

## Code Location Map

### Critical Functions (with line numbers in rs300.c)

| Category | Function | Lines | Purpose |
|----------|----------|-------|---------|
| **Initialization** |
| Driver probe | `rs300_probe()` | 2484+ | Driver initialization, I2C detection |
| Control init | `rs300_init_controls()` | 2236+ | Register all 11 V4L2 controls |
| Format setup | `rs300_set_default_format()` | 620+ | Initialize default 640×512 format |
| **Video Streaming** |
| Stream control | `rs300_set_stream()` | 1754+ | Start/stop video streaming (includes retry logic) |
| FPS config | `rs300_set_fps()` | 1710+ | Set frame rate (25/30/50/60) |
| Stop streaming | `rs300_stop_streaming()` | 1696+ | Send stop command |
| **Format Operations** |
| Set format | `rs300_set_pad_fmt()` | 1563+ | Negotiate and set pad format |
| Get format | `rs300_get_pad_fmt()` | 1548+ | Query current format |
| Format code | `rs300_get_format_code()` | 529+ | Validate media bus format |
| **V4L2 Controls** |
| Control handler | `rs300_set_ctrl()` | 1336+ | Dispatch control operations |
| Brightness | `rs300_brightness_correct()` | 1161+ | Set brightness (0-100) |
| Colormap | `rs300_set_colormap()` | 1109+ | Set color palette (0-11) |
| FFC trigger | `rs300_shutter_cal()` | 1151+ | Flat field calibration |
| Zoom | `rs300_set_zoom()` | 1294+ | Digital zoom (1-8x) |
| Scene mode | `rs300_set_scene_mode()` | 1316+ | Set scene mode (0-9) |
| Contrast | `rs300_set_contrast()` | 962+ | Set contrast (0-100) |
| DDE | `rs300_set_dde()` | 786+ | Digital detail enhancement |
| **I2C Communication** |
| Read registers | `read_regs()` | 184 | I2C read transaction (forward declaration) |
| Write registers | `write_regs()` | 185 | I2C write transaction (forward declaration) |
| CRC calculation | `do_crc()` | 163+ | CRC-16-CCITT checksum |

### Key Data Structures

| Structure | Location | Purpose |
|-----------|----------|---------|
| `struct rs300` | rs300.c:222+ | Main driver state (V4L2 subdev, controls, format, mode) |
| `struct rs300_mode` | rs300.c:194+ | Video mode definition (width, height, fps, format code) |
| `supported_modes[]` | rs300.c:493+ | 3 available modes: 640×512, 256×192, 384×288 |
| `codes[]` | rs300.c:214+ | 4 supported media bus formats (16-bit packed for Pi 5) |

### Module Parameters

| Parameter | Default | Location | Purpose | Valid Values |
|-----------|---------|----------|---------|--------------|
| `mode` | 0 | rs300.c:88 | Select video resolution | 0=640×512, 1=256×192, 2=384×288 |
| `fps` | 30 | rs300.c:89 | Frame rate | 25, 30, 50, 60 |
| `type` | 16 | rs300.c:90 | Bit depth | 8 or 16 |
| `debug` | 1 | rs300.c:91 | Debug verbosity | 0 (off) or 1 (on) |

### Hardware Specifications

| Specification | Value | Reference |
|---------------|-------|-----------|
| I2C Address | 0x3c | rs300.c:33 |
| I2C Bus (Pi 5) | i2c-10 | Device tree |
| MIPI Link Frequency | 80 MHz | rs300.c:43 |
| Pixel Rate (8-bit) | 200 MHz | rs300.c:44 |
| Pixel Rate (16-bit) | 400 MHz | rs300.c:45 |
| CSI-2 Data Lanes | 2 lanes | Device tree |
| Supported Formats (Pi 5) | UYVY8_1X16, YUYV8_1X16 | 16-bit packed only (RP1-CFE requirement) |

---

## AI Assistant Guidance

### Context Window Optimization

**Token Budget Strategy**:
- **CLAUDE.md**: Always read (this file, ~380 lines)
- **Tier 1 Docs**: Read sections as needed (~100-200 lines per doc)
- **Tier 2 Docs**: Read relevant sections only (~50-100 lines)
- **Tier 3 Docs**: Read targeted sections (~100-300 lines)
- **Source Code**: Read specific functions only (~20-100 lines)

**Rule**: Start with minimum reading, escalate only as needed.

### START_HERE.md - Current Status Tracking

**Check first**: `ls START_HERE.md && cat START_HERE.md`

**Purpose**: Tracks current project status, pending work, next actions across sessions.

**When it exists**:
- ✅ Read FIRST at session start (before CLAUDE.md)
- ✅ Follow "Next Actions" listed
- ✅ Update after commits/significant work
- ✅ Update before ending session with pending work

**Update triggers**:
- After git commits → Move "Pending" to "Completed", update "Next Actions"
- Before reboot/restart → Document post-reboot steps
- When blocked → Document blocker and what was tried
- Before context clear → Ensure next session can resume

**Template structure**: Current status, completed items, pending items, next actions, quick commands.

**Don't create for**: Quick one-off tasks, stable projects, read-only exploration.

**Relationship to other docs**:
- README.md: What is this project?
- CLAUDE.md: How to navigate this project?
- START_HERE.md: What's happening RIGHT NOW? What do I do next?

### When to Deep-Read vs. Skim

| Scenario | Action | Documentation | Depth |
|----------|--------|---------------|-------|
| User asks "how do I..." | Skim | DEV_QUICK_REFERENCE.md | Headers + tables only |
| User reports error | Standard | TROUBLESHOOTING.md | Decision tree → specific section |
| User wants to modify control | Deep | DRIVER_ANALYSIS.md (Section 4) + I2C_PROTOCOL.md | Full sections |
| User asks "why does X..." | Deep | DRIVER_ANALYSIS.md (relevant section) | Complete understanding |
| User wants to add feature | Deep | DRIVER_ANALYSIS.md (multiple sections) + rs300.c | Full analysis + code |

### Documentation Precedence Rules

When information conflicts or authoritative answer needed:

1. **Source code (rs300.c)** = Ground truth
2. **DRIVER_ANALYSIS.md** = Most comprehensive analysis
3. **I2C_PROTOCOL.md** = Protocol authority
4. **DEV_QUICK_REFERENCE.md** = Operational truth
5. **Other documentation** = Supporting information

### Code Modification Guidelines

**Before modifying code**:

1. ✅ Read DRIVER_ANALYSIS.md Section 7 (Code Quality Assessment)
2. ✅ Identify relevant section (use Table of Contents)
3. ✅ Read that section completely with context
4. ✅ Check for TODOs and known issues
5. ✅ Review related functions (line numbers provided)

**Patterns to Follow**:
- All camera commands follow same structure (see DRIVER_ANALYSIS.md Section 3.3)
- Consider refactoring opportunity (Section 7.2: helper function could reduce 500→150 lines)
- Match existing error handling patterns
- Maintain debug logging style (dev_info/dev_err)

**Known Issues to Avoid** (from DRIVER_ANALYSIS.md Section 7):
- ⚠️ ~500 lines of duplicate command execution code (consider helper function)
- ⚠️ Zoom command uses hardcoded CRC (rs300.c:1486-1487) instead of calculation
- ⚠️ Mode switching requires driver reload (runtime switching not implemented)

### Task-Specific Reading Paths

**Task: "Debug I2C timeout error"**
```
1. TROUBLESHOOTING.md → Section 7 (I2C Communication Issues)
   └─ Check quick diagnostics and common causes
2. If not resolved → I2C_PROTOCOL.md → Section 7 (Command Execution Flow)
   └─ Understand polling and timeout logic
3. If still stuck → DRIVER_ANALYSIS.md → Section 2 (I2C Protocol)
   └─ Deep dive into communication patterns
4. Check code → rs300.c:184-185 (read_regs, write_regs forward declarations)
```

**Task: "Add new V4L2 control for temperature readout"**
```
1. DRIVER_ANALYSIS.md → Section 4 (V4L2 Control Interface)
   └─ Understand control architecture and registration
2. DEV_QUICK_REFERENCE.md → V4L2 Control Reference
   └─ See examples of existing controls
3. I2C_PROTOCOL.md → Section 6 (Command Reference)
   └─ Find temperature command (if exists) or design new one
4. Code locations:
   - Add control config: rs300.c:2168+ (after temporal_nr_ctrl)
   - Add to handler: rs300.c:1336+ (new case in rs300_set_ctrl)
   - Register control: rs300.c:2236+ (in rs300_init_controls)
   - Implement command: Follow pattern from brightness (rs300.c:1161+)
```

**Task: "Understand complete streaming flow from start to first frame"**
```
1. DRIVER_ANALYSIS.md → Section 5.5 (Stream Start Sequence)
   └─ Read complete flow diagram with 9 steps
2. See detailed explanation of each step with timing
3. Code walk-through: rs300.c:1754+ (rs300_set_stream function with retry logic)
4. Cross-reference: I2C_PROTOCOL.md for start_regs command structure
```

**Task: "Optimize driver performance (reduce code duplication)"**
```
1. TROUBLESHOOTING.md → Section 8 (Performance Issues)
   └─ Check if performance is actually a problem
2. DRIVER_ANALYSIS.md → Section 7.2 (Code Duplication Analysis)
   └─ Understand the 500-line duplication issue
3. See proposed refactoring: rs300_send_command() helper function
4. Implementation plan:
   - Create helper function based on common pattern
   - Migrate one command (e.g., brightness) to verify
   - Migrate remaining 10+ commands
   - Test with ./test_controls.sh
```

### Response Strategy Guidelines

**For Quick Questions** ("How do I set brightness to 75?"):
- Answer directly from DEV_QUICK_REFERENCE.md
- Provide command + example
- No need to read analysis docs
- **Example**: "Use: `v4l2-ctl -d /dev/v4l-subdev2 -c brightness=75`"

**For How-To Questions** ("How do I configure the media pipeline?"):
- Check TROUBLESHOOTING.md or operational guides first
- Provide step-by-step from guides
- Link to relevant quick reference for commands
- **Example**: Point to configure_media.sh or BOOT_CONFIGURATION.md

**For Why Questions** ("Why does the driver use UYVY8_1X16 format?"):
- Read DRIVER_ANALYSIS.md relevant section
- Explain architecture/design decision
- Reference line numbers and code for details
- **Example**: "Pi 5 RP1-CFE only supports 16-bit packed formats (DRIVER_ANALYSIS.md Section 5.2)"

**For Debugging** ("Video capture is failing"):
- Start with TROUBLESHOOTING.md decision tree
- Follow diagnostic steps
- Escalate to analysis docs if not resolved
- Provide specific diagnostic commands
- **Example**: Walk through TROUBLESHOOTING.md Section 3 → check pipeline → verify format

**For Code Modifications** ("I want to add a new control"):
- **Always** read DRIVER_ANALYSIS.md first (relevant sections)
- Understand existing patterns before suggesting changes
- Follow code style and error handling from Section 7
- Provide line numbers for modification locations
- Recommend testing with ./test_controls.sh after changes
- **Example**: Read Section 4 → follow pattern → give exact locations

---

## V4L2 Controls Reference

### All 11 Available Controls

| Control Name | Type | Range | Default | Description |
|--------------|------|-------|---------|-------------|
| `brightness` | Integer | 0-100 | 50 | Thermal brightness level |
| `contrast` | Integer | 0-100 | 50 | Image contrast |
| `zoom_absolute` | Integer | 1-8 | 1 | Digital zoom (1x-8x) |
| `colormap` | Menu | 0-11 | 0 | Color palette (White Hot, Ironbow, etc.) |
| `ffc_trigger` | Button | - | - | Trigger flat field calibration |
| `scene_mode` | Menu | 0-9 | 3 | Scene optimization (General, High Contrast, etc.) |
| `digital_detail_enhancement` | Integer | 0-100 | 50 | Edge enhancement (DDE) |
| `spatial_noise_reduction` | Integer | 0-100 | 50 | Spatial noise filter |
| `temporal_noise_reduction` | Integer | 0-100 | 50 | Temporal noise filter |
| `pixel_rate` | Integer (RO) | - | 200-400 MHz | Pixel clock rate (read-only) |
| `link_freq` | Integer (RO) | - | 80 MHz | MIPI link frequency (read-only) |

**See**: [DEV_QUICK_REFERENCE.md](DEV_QUICK_REFERENCE.md#v4l2-control-reference) for complete tables with all colormap and scene mode options.

**Test**: `./test_controls.sh` to verify all controls work correctly.

---

## Media Controller Pipeline

**IMPORTANT**: Pi 5 RP1-CFE **only supports 16-bit packed formats** (`UYVY8_1X16`, `YUYV8_1X16`).
8-bit dual lane formats (`*8_2X8`) will cause "Format mismatch!" errors.

### Quick Pipeline Configuration

```bash
# Automated (recommended)
./configure_media.sh

# Manual (for reference)
media-ctl -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0[1]"
media-ctl -V "'rs300 10-003c':0 [fmt:UYVY8_1X16/640x512 field:none colorspace:smpte170m xfer:709 ycbcr:601 quantization:lim-range]"
media-ctl -V "'csi2':0 [fmt:UYVY8_1X16/640x512 ...]"
media-ctl -V "'csi2':4 [fmt:UYVY8_1X16/640x512 ...]"
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=512,pixelformat=UYVY,colorspace=smpte170m,xfer=709,ycbcr=601,quantization=lim-range
```

**Note**: Pipeline configuration does not persist across reboots. See [BOOT_CONFIGURATION.md](BOOT_CONFIGURATION.md) for auto-start options.

---

## Raspberry Pi 5 Specific Notes

- **Architecture**: BCM2712 with dedicated CSI0 port via RP1 controller
- **I2C Bus**: i2c-10 through i2c_csi_dsi0 controller
- **Performance**: 640×512@60fps thermal imaging
- **Cable**: Requires 22-pin to 15-pin adapter for camera connection
- **Driver**: Uses rp1-cfe (Camera Front End), not legacy Unicam
- **Format Compatibility**: Only 16-bit packed formats (UYVY8_1X16, YUYV8_1X16)

**Device Tree Configuration** (`/boot/firmware/config.txt`):
```
camera_auto_detect=0
dtoverlay=rs300
```

---

## Document Change Log

**2025-10-21**: TESTING COMPLETE - Issue resolved
- Updated: Project status "Active Development" → "Production Ready (deadlock fix validated)"
- Updated: Critical Known Issues section → Recent Fixes (RESOLVED status)
- Added: Test results reference (100% success rate, 25/25 tests)
- Updated: ISSUE_SUMMARY_20251021.md with test results and resolved status
- Updated: START_HERE.md with completion status and test artifacts
- Created: TEST_SUMMARY.txt with comprehensive test results
- Status: Retry logic fully validated, ready for production use

**2025-10-21**: CRITICAL ACCURACY FIXES (commit eb99791 aftermath)
- Fixed: All function line numbers after struct reorganization (18 functions updated)
- Fixed: Project status "Production Ready" → "Active Development (testing deadlock fix)"
- Fixed: File count 50 → 90 (actual current count)
- Added: ⚠️ Critical Known Issues section (rp1-cfe deadlock warning)
- Added: POST_REBOOT_TESTING.md to Quick Navigation
- Updated: rs300_set_stream now noted as including retry logic
- Impact: Line numbers were off by ~300-500 lines, now accurate
- See: CLAUDE_MD_AUDIT.md for complete analysis

**2025-10-21**: Added START_HERE.md workflow guidance
- Added: START_HERE.md to Quick Navigation (top entry)
- Added: START_HERE.md workflow in AI Assistant Guidance (~26 lines)
- Purpose: Track current project status and next actions across sessions
- Provides: Check-first workflow, update triggers, relationship to other docs

**2025-10-21**: Comprehensive restructure to 5-layer architecture
- Added: Documentation hierarchy (4 tiers)
- Added: Task decision tree
- Added: Reading depth guidelines
- Added: Code location map (18 functions with line numbers)
- Added: AI assistant optimization guidance
- Added: Task-specific reading paths
- Enhanced: All workflows with specific documentation references
- Total: 81 lines → 380 lines (+299 lines, +369%)

**Purpose**: Transform CLAUDE.md into intelligent navigation and documentation architecture system.
