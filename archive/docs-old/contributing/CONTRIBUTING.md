# Contributing to RS300 Thermal Camera Driver

Thank you for your interest in contributing to the RS300 driver project! This document provides guidelines and instructions for contributing.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Testing Requirements](#testing-requirements)
- [Submitting Changes](#submitting-changes)
- [Documentation](#documentation)
- [Community](#community)

---

## Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inclusive environment for all contributors.

**Expected Behavior**:
- ✅ Be respectful and constructive
- ✅ Focus on technical merit
- ✅ Help others learn and grow
- ✅ Accept constructive criticism gracefully

**Unacceptable Behavior**:
- ❌ Harassment or discrimination
- ❌ Trolling or inflammatory comments
- ❌ Personal attacks
- ❌ Publishing others' private information

**Reporting**: If you experience or witness unacceptable behavior, please report via GitHub or email.

---

## How Can I Contribute?

### 1. Reporting Bugs 🐛

**Before submitting**:
- Check [existing issues](https://github.com/Kodrea/rs300-v4l2-driver/issues)
- Review [TROUBLESHOOTING.md](../../TROUBLESHOOTING.md)
- Verify your setup is correct

**Good bug report includes**:
```markdown
**Environment**:
- Platform: Raspberry Pi 5 / Pi 4B
- OS Version: Raspberry Pi OS Bookworm
- Kernel: 6.6.x
- Module Resolution: 640×512 / 384×288 / 256×192

**Steps to Reproduce**:
1. Install driver via ./setup.sh
2. Run ./configure_media.sh
3. Execute: v4l2-ctl -d /dev/video0 --stream-mmap

**Expected Behavior**:
Should stream at 60fps

**Actual Behavior**:
Gets error: "Format mismatch!"

**Logs**:
```bash
dmesg | grep rs300
# [paste output]
```

**Additional Context**:
- FFC cable connections verified
- I2C detection: i2cdetect -y 10 shows 0x3c
```

**Submit**: [New Bug Report](https://github.com/Kodrea/rs300-v4l2-driver/issues/new)

### 2. Suggesting Features 💡

**Good feature request includes**:
- Clear use case / problem to solve
- Proposed solution (if you have one)
- Alternative approaches considered
- Impact on existing functionality

**Example**:
```markdown
**Feature**: Automatic FFC scheduling

**Problem**:
Users must manually trigger FFC calibration. For long-running applications,
forgetting FFC leads to image quality degradation.

**Proposed Solution**:
Add V4L2 control for FFC interval (e.g., every 5 minutes).
Driver spawns kernel timer to trigger automatic FFC.

**Alternatives**:
- User-space daemon (less elegant, requires separate process)
- ISP integration with auto-calibration (more complex)

**Impact**:
- New V4L2 control (backward compatible)
- Optional feature (disabled by default)
```

**Submit**: [New Feature Request](https://github.com/Kodrea/rs300-v4l2-driver/issues/new)

### 3. Contributing Code 🔧

**Areas needing help**:
- ISP integration (Pi 5)
- Code refactoring (reduce duplication)
- Python SDK development
- Additional platform support (Jetson, Orange Pi, etc.)
- Bug fixes

**See**: [ROADMAP.md](ROADMAP.md) for planned features

### 4. Improving Documentation 📝

**Documentation needs**:
- Typo fixes
- Clarity improvements
- Additional examples
- Translations (future)
- Tutorial videos

**Quick fixes** (typos, links):
- Submit PR directly

**Major changes** (restructuring, new guides):
- Open issue first to discuss

### 5. Testing 🧪

**Valuable testing**:
- Different hardware (384×288, 256×192 modules)
- Different platforms (Pi 5 vs Pi 4)
- Edge cases and error conditions
- Performance benchmarking
- Long-running stability tests

**Report results**: Open issue with "Testing:" prefix

---

## Development Setup

### Prerequisites

**Hardware**:
- Raspberry Pi 5 or 4B
- RS300 thermal camera module
- Adequate power supply

**Software**:
- Raspberry Pi OS Bookworm
- Git
- Kernel headers
- Development tools

### Setup Steps

**1. Fork the repository**:
- Visit [rs300-v4l2-driver](https://github.com/Kodrea/rs300-v4l2-driver)
- Click "Fork" button

**2. Clone your fork**:
```bash
git clone https://github.com/YOUR-USERNAME/rs300-v4l2-driver.git
cd rs300-v4l2-driver
```

**3. Add upstream remote**:
```bash
git remote add upstream https://github.com/Kodrea/rs300-v4l2-driver.git
```

**4. Install development dependencies**:
```bash
sudo apt update
sudo apt install raspberrypi-kernel-headers dkms git v4l-utils \
                 build-essential device-tree-compiler
```

**5. Build and test**:
```bash
./setup.sh
sudo reboot
# After reboot:
./configure_media.sh  # Pi 5 only
./test_controls.sh
```

### Development Workflow

**1. Create a feature branch**:
```bash
git checkout -b feature/your-feature-name
# Or: git checkout -b bugfix/issue-123
```

**2. Make your changes**:
- Edit source files
- Add tests if applicable
- Update documentation

**3. Test thoroughly**:
```bash
# Rebuild module
./setup.sh

# Reboot to load new module
sudo reboot

# Run tests
./test_controls.sh

# Manual testing
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
```

**4. Commit your changes**:
```bash
git add .
git commit -m "Add feature: brief description"
```

**Good commit message**:
```
Add automatic FFC scheduling control

- Implement ffc_interval V4L2 control (range: 0-60 minutes)
- Add kernel timer for periodic FFC triggering
- Default disabled (0 = manual FFC only)
- Update documentation with usage examples

Fixes #123
```

**5. Push to your fork**:
```bash
git push origin feature/your-feature-name
```

**6. Open Pull Request**:
- Visit your fork on GitHub
- Click "Pull Request" button
- Fill out PR template

---

## Coding Standards

### Linux Kernel Style

This is a kernel driver - follow Linux kernel coding style.

**Key points**:
- Indentation: **Tabs** (width 8)
- Line length: 80 characters preferred, 100 maximum
- Braces: K&R style (opening brace on same line)
- Naming: `lowercase_with_underscores`
- Comments: `/* C-style */` for multi-line, `// C++` acceptable for single-line

**Check your code**:
```bash
# Linux kernel checkpatch script (if available)
scripts/checkpatch.pl --no-tree -f rs300.c

# Or manually review against coding style
```

### Code Organization

**Follow existing patterns**:

1. **Command execution** (see rs300.c:1327-1456 for brightness example):
   ```c
   static int rs300_command_name(struct rs300 *rs300, int value)
   {
       u8 regs[18];
       int ret;

       /* Build command packet */
       regs[0] = 0xAA;  // Header
       regs[1] = 0x55;
       // ... fill packet

       /* Calculate CRC */
       crc = do_crc(regs, 16);
       regs[16] = crc & 0xFF;
       regs[17] = (crc >> 8) & 0xFF;

       /* Send command */
       ret = write_regs(rs300->client, regs, 18);

       /* Poll for completion */
       ret = read_regs(rs300->client, &status, 1);

       return ret;
   }
   ```

2. **V4L2 control handler** (rs300.c:1626-1680):
   ```c
   static int rs300_set_ctrl(struct v4l2_ctrl *ctrl)
   {
       struct rs300 *rs300 = container_of(ctrl->handler,
                                           struct rs300, ctrls);

       switch (ctrl->id) {
       case V4L2_CID_YOUR_CONTROL:
           return rs300_your_command(rs300, ctrl->val);
       default:
           return -EINVAL;
       }
   }
   ```

3. **Error handling**:
   ```c
   ret = some_function();
   if (ret < 0) {
       dev_err(&client->dev, "Function failed: %d\n", ret);
       return ret;
   }
   ```

### Documentation in Code

**Function comments**:
```c
/*
 * rs300_function_name - Brief description
 * @param1: Description of param1
 * @param2: Description of param2
 *
 * Detailed description of what this function does.
 *
 * Return: 0 on success, negative error code on failure
 */
static int rs300_function_name(struct rs300 *rs300, int param1, int param2)
{
    // Implementation
}
```

---

## Testing Requirements

### Required Tests

**Before submitting PR**:

1. **Compilation**: Must build without errors or warnings
   ```bash
   ./setup.sh
   # Check for errors
   ```

2. **Basic functionality**: Driver loads and camera streams
   ```bash
   sudo reboot
   ./configure_media.sh  # Pi 5
   v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=100
   ```

3. **Control tests**: All controls work as expected
   ```bash
   ./test_controls.sh
   ```

4. **No regressions**: Existing features still work
   ```bash
   # Test all previously working features
   ```

### Platform Testing

**Minimum requirement**:
- Test on your development platform (Pi 5 or Pi 4)

**Ideal**:
- Test on both Pi 5 and Pi 4
- Test with different module resolutions
- Test with different kernel versions

**Report results in PR**:
```markdown
**Testing**:
- [x] Raspberry Pi 5 (Bookworm, kernel 6.6.51)
- [x] 640×512 module @ 60fps
- [x] All controls tested with ./test_controls.sh
- [ ] Raspberry Pi 4 (not available for testing)
```

### Performance Testing

**For performance-impacting changes**:

```bash
# Measure frame rate
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=300

# Check CPU usage
top
# (run streaming in another terminal)

# Check thermal throttling
vcgencmd get_throttled
```

**Report**: Include before/after measurements in PR

---

## Submitting Changes

### Pull Request Process

**1. Ensure PR is ready**:
- [ ] Code follows style guidelines
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Commit messages are clear
- [ ] Branch is up-to-date with main

**2. Update your branch**:
```bash
git fetch upstream
git rebase upstream/main
# Or: git merge upstream/main
```

**3. Create Pull Request**:
- Descriptive title: `Add feature: automatic FFC scheduling`
- Detailed description (use template)
- Reference related issues: `Fixes #123` or `Relates to #456`

**4. PR template**:
```markdown
## Description
Brief description of changes

## Motivation
Why is this change needed?

## Changes
- Change 1
- Change 2

## Testing
- [x] Tested on Pi 5
- [x] All tests pass
- [x] No regressions

## Screenshots / Logs
(if applicable)

## Related Issues
Fixes #123
```

**5. Code Review**:
- Maintainers will review your PR
- Address feedback constructively
- Make requested changes
- Push updates to same branch

**6. Merge**:
- Once approved, maintainer will merge
- Your contribution will be credited

### Commit Message Guidelines

**Format**:
```
Short summary (50 chars or less)

Detailed explanation of what changed and why.
Can span multiple paragraphs.

- Bullet points for specific changes
- Reference issues: Fixes #123

Signed-off-by: Your Name <your.email@example.com>
```

**Types of commits**:
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation only
- `refactor:` Code refactoring
- `test:` Adding tests
- `perf:` Performance improvement

**Example**:
```
feat: Add automatic FFC scheduling control

Implement periodic FFC calibration via new V4L2 control.
Users can set interval (0-60 minutes) for automatic FFC.
Default is 0 (disabled, manual FFC only).

- Add ffc_interval control (0-60 minutes)
- Implement kernel timer for periodic triggering
- Update documentation and examples

Fixes #123
```

---

## Documentation

### What to Document

**Required for new features**:
- [ ] Code comments (in-line documentation)
- [ ] User-facing documentation (README, guides)
- [ ] API documentation (if applicable)
- [ ] Example usage

**Documentation files to update**:
- `README.md` - If user-visible change
- `docs/` - Relevant guide files
- `DEV_QUICK_REFERENCE.md` - If adding commands
- `DRIVER_ANALYSIS.md` - If changing architecture
- `ROADMAP.md` - Update status if implementing planned feature

### Documentation Style

**User documentation**:
- Clear, concise language
- Step-by-step instructions
- Examples for common use cases
- Troubleshooting tips

**Technical documentation**:
- Precise terminology
- Line number references for code
- Architecture diagrams (if helpful)
- Cross-references to related docs

---

## Community

### Communication Channels

**GitHub Issues**:
- Bug reports
- Feature requests
- Technical discussions

**GitHub Discussions** (if enabled):
- General questions
- Show and tell (your projects)
- Ideas and brainstorming

**Video Tutorials**:
- [linktr.ee/kodrea](https://linktr.ee/kodrea)
- Community contributions welcome

### Getting Help

**Stuck on something?**

1. Check existing documentation
2. Search closed issues
3. Ask in GitHub Discussions
4. Open new issue with question

**We're here to help!**

---

## Recognition

### Contributors

All contributors will be recognized in:
- GitHub contributors page
- `CHANGELOG.md` (for significant contributions)
- Project README (for major features)

### Attribution

**Types of contribution**:
- 💻 Code
- 📖 Documentation
- 🐛 Bug reports
- 🧪 Testing
- 💡 Ideas
- 🎨 Design

**All contributions matter!**

---

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (MIT License).

**See**: [LICENSE](../../LICENSE) file

---

## Questions?

**Not sure where to start?**

- Check [ROADMAP.md](ROADMAP.md) for ideas
- Look for issues labeled `good first issue`
- Ask in Discussions

**Thank you for contributing!** 🎉

---

**Last Updated**: 2025-10-21
