# Documentation Maintenance Guide

This document explains how to keep project documentation current, accurate, and useful.

---

## Overview

Good documentation requires regular maintenance. Stale documentation is worse than no documentation because it misleads users and wastes time.

**Key Principle**: Documentation should be a living system that updates automatically when possible and has clear triggers for manual updates.

---

## Automated Maintenance: /update-docs Command

The `/update-docs` slash command handles most routine maintenance:

### What It Updates Automatically

1. **File Counts in CLAUDE.md**
   - Total .md files in repository
   - Files in docs/ hierarchy
   - Line counts if changed significantly (>10%)

2. **SESSION_STATE.md**
   - Git status (branch, dirty/clean, last commit)
   - Test status (runs /test-camera if appropriate)
   - Last updated timestamp
   - Current file modifications list

3. **Session Notes** (in .claude/sessions/)
   - Generates brief summary of current session
   - Archives in timestamped file (YYYY-MM-DD_HHMM.md)
   - 10-20 lines covering actions, outcomes, next steps

4. **Link Validation**
   - Scans all .md files for internal file references
   - Checks if referenced files exist
   - Reports broken links

5. **Staleness Detection**
   - Identifies outdated timestamps (>7 days old)
   - Checks for TODO/FIXME comments in recent changes
   - Warns about uncommitted work

### When to Run /update-docs

**Automatically** (ideal setup):
- Before every git commit (via pre-commit hook)
- Before ending a session
- After major changes

**Manually** (minimum):
- When you remember
- Before important commits
- After discovering stale information

---

## Manual Maintenance Triggers

Some updates require human judgment and should be done manually:

### 1. Code Structure Changes

**Trigger**: Modifying function signatures, moving code, refactoring

**Update**:
- Line numbers in CLAUDE.md "Code Location Map" (if functions moved)
- DRIVER_ANALYSIS.md sections referencing code locations
- DEV_QUICK_REFERENCE.md if command patterns changed

**Example**: If `rs300_set_ctrl()` moves from line 1336 to line 1450:
```bash
# Update CLAUDE.md
sed -i 's/rs300_set_ctrl.*1336/rs300_set_ctrl() | 1450/' CLAUDE.md

# Run /update-docs to validate
/update-docs
```

### 2. New Features

**Trigger**: Adding new V4L2 controls, commands, or functionality

**Update**:
- DEV_QUICK_REFERENCE.md (add to control reference table)
- CLAUDE.md (add to V4L2 Controls Reference if applicable)
- DRIVER_ANALYSIS.md (document in relevant section)
- docs/guides/camera-controls.md (user-facing guide)

**Checklist**:
- [ ] Update control/command tables
- [ ] Add usage examples
- [ ] Document any quirks or limitations
- [ ] Update test scripts if applicable
- [ ] Run /update-docs to validate links

### 3. Bug Fixes

**Trigger**: Fixing a known issue or discovering new limitation

**Update**:
- TROUBLESHOOTING.md (if user-facing issue)
- SESSION_STATE.md "Known Issues" section
- START_HERE.md (if affects project status)
- .claude/lessons-learned/ (if important lesson for future development)

### 4. Architecture Changes

**Trigger**: Changing system design, adding components, restructuring code

**Update**:
- DRIVER_ANALYSIS.md (Section 1: Architecture Overview)
- README.md (if changes high-level description)
- CLAUDE.md (if changes navigation or workflows)

### 5. Test Results

**Trigger**: Running tests, discovering new test requirements

**Update**:
- SESSION_STATE.md "Test & Validation Status"
- START_HERE.md (if status changes)
- Create .claude/sessions/ note with test results

---

## Documentation Validation Checklist

### Before Committing Documentation Changes

- [ ] All file references point to existing files
- [ ] Line numbers accurate (spot-check 2-3 references)
- [ ] Code examples compile/run successfully
- [ ] No TODO placeholders left unresolved
- [ ] Timestamps are current
- [ ] Links use relative paths (not absolute)

### Monthly Review (if active development)

- [ ] Verify file counts in CLAUDE.md match reality
- [ ] Check line numbers in Code Location Map
- [ ] Review Known Issues in SESSION_STATE.md (resolve or remove stale items)
- [ ] Update START_HERE.md if project status changed
- [ ] Archive or remove obsolete session-specific docs (POST_REBOOT_INSTRUCTIONS.md, etc.)

---

## Common Staleness Patterns

### Problem: File Counts Become Inaccurate

**Symptom**: CLAUDE.md says "56 files" but there are actually 70

**Prevention**: Run /update-docs regularly

**Fix**:
```bash
# Count actual files
find . -name "*.md" -type f | wc -l

# Update CLAUDE.md manually or run /update-docs
/update-docs
```

### Problem: Line Numbers Drift

**Symptom**: CLAUDE.md says `rs300_set_ctrl()` at line 1336 but it's actually at 1450

**Prevention**: Don't commit code refactoring without updating docs

**Fix**:
```bash
# Find actual line number
grep -n "rs300_set_ctrl()" rs300.c

# Update CLAUDE.md and all docs referencing this function
/update-docs  # Will warn about line number mismatches if detectable
```

### Problem: Broken Links

**Symptom**: Documentation references file that doesn't exist

**Prevention**: Run /update-docs which validates links

**Fix**:
```bash
# /update-docs will report broken links
/update-docs

# Example output:
# ⚠️  Broken link in CLAUDE.md:59 → START_HERE_OLD.md (file not found)

# Either fix the link or remove the reference
```

### Problem: Outdated Test Results

**Symptom**: SESSION_STATE.md shows test from days ago

**Prevention**: Update after every test run

**Fix**:
```bash
# Run current tests
/test-camera

# Update SESSION_STATE.md "Test & Validation Status" with results
nano SESSION_STATE.md

# Run /update-docs to timestamp the update
/update-docs
```

### Problem: Stale "In-Progress Work"

**Symptom**: SESSION_STATE.md lists tasks from old sessions as in-progress

**Prevention**: Mark tasks complete or remove when finished

**Fix**:
```bash
# Review SESSION_STATE.md "In-Progress Work"
nano SESSION_STATE.md

# Mark completed items with [x]
# Remove items that are no longer relevant
# Move abandoned work to "Blockers" if needs attention
```

---

## Documentation Health Metrics

### Green (Healthy)

- ✅ /update-docs ran within last 24 hours
- ✅ No broken links detected
- ✅ File counts within 10% of actual
- ✅ Test results from current week
- ✅ No uncommitted documentation changes

### Yellow (Needs Attention)

- ⚠️ /update-docs last ran 2-7 days ago
- ⚠️ 1-3 broken links exist
- ⚠️ Minor staleness (old timestamps)
- ⚠️ Test results 1-2 weeks old

### Red (Critical)

- ❌ /update-docs not run in 7+ days
- ❌ Multiple broken links (4+)
- ❌ File counts off by >20%
- ❌ Test results 1+ month old
- ❌ Major uncommitted changes

**Action**: If documentation is RED, stop feature work and run /update-docs + manual review before continuing.

---

## File-Specific Maintenance

### CLAUDE.md (AI Assistant Guide)

**Update Frequency**: After major changes (new features, restructures)

**Common Updates**:
- File counts (auto via /update-docs)
- Quick Navigation table (manual - add new sections)
- Code Location Map (manual - after function moves)
- Document Change Log (manual - after significant doc changes)

### SESSION_STATE.md (Current State)

**Update Frequency**: Multiple times per session

**Common Updates**:
- In-Progress Work (after starting/completing tasks)
- Blockers & Questions (when stuck)
- Recent Decisions (after making important choices)
- Test Status (after running tests)
- Quick Context (to summarize current state)

### START_HERE.md (Human-Readable Status)

**Update Frequency**: When project status changes

**Common Updates**:
- Current Phase (if moving between phases)
- Quick Commands (if workflows change)
- Key Files & Locations (if output locations change)
- Decision Tree (if priorities shift)

### DEV_QUICK_REFERENCE.md (Command Cheat Sheet)

**Update Frequency**: When commands or controls change

**Common Updates**:
- V4L2 control tables (if adding new controls)
- Command examples (if syntax changes)
- Code locations (if functions move significantly)

### DRIVER_ANALYSIS.md (Technical Deep-Dive)

**Update Frequency**: When driver internals change

**Common Updates**:
- Architecture diagrams (if adding/removing components)
- Line number references (if code restructured)
- Known issues (if bugs fixed or discovered)

---

## Integration with Git

### Pre-Commit Hook (Recommended)

Create `.git/hooks/pre-commit`:

```bash
#!/bin/bash
# Auto-run /update-docs before each commit

echo "Running documentation updates..."

# Check if Claude Code is available
if command -v claude &> /dev/null; then
    # Run /update-docs (TODO: implement this when hook support added)
    echo "Documentation updated"
else
    echo "⚠️  Claude Code not available, skipping doc updates"
    echo "    Run /update-docs manually if docs changed"
fi

exit 0
```

**Note**: Full automation requires Claude Code hook support (future enhancement).

### Commit Message Conventions

When committing documentation changes, use prefixes:

- `docs:` - Documentation updates
- `docs(fix):` - Fixing stale/broken documentation
- `docs(add):` - Adding new documentation
- `docs(update):` - Updating existing documentation

Examples:
```bash
git commit -m "docs: Update line numbers in CLAUDE.md after refactoring"
git commit -m "docs(fix): Repair broken links in DRIVER_ANALYSIS.md"
git commit -m "docs(add): Create SESSION_STATE.md for session handoff"
```

---

## Troubleshooting Documentation Issues

### "I don't know which doc to update"

**Decision Tree**:
```
What changed?
│
├─ Code structure/functions → DRIVER_ANALYSIS.md + CLAUDE.md (line numbers)
├─ User-facing feature → docs/guides/ + DEV_QUICK_REFERENCE.md
├─ Bug fixed → TROUBLESHOOTING.md + SESSION_STATE.md (Known Issues)
├─ Test results → SESSION_STATE.md (Test Status)
├─ Project status → START_HERE.md
└─ Important lesson → .claude/lessons-learned/
```

### "Documentation is overwhelming"

**Priority Order**:
1. SESSION_STATE.md (always update this)
2. /update-docs (runs automatically)
3. File-specific updates (only if relevant)

You don't need to update everything - focus on what changed.

### "I updated docs but they still seem stale"

**Checklist**:
- [ ] Did you run /update-docs?
- [ ] Did you update timestamp in SESSION_STATE.md?
- [ ] Did you commit the changes?
- [ ] Are there multiple files referencing the same info?

**Tip**: Use grep to find all references:
```bash
grep -r "specific topic" *.md docs/
```

---

## Best Practices

### Do:
- ✅ Update documentation AT THE SAME TIME as code changes
- ✅ Use /update-docs regularly (automatic is best)
- ✅ Keep session notes brief but informative
- ✅ Validate links before committing
- ✅ Use consistent formatting and terminology

### Don't:
- ❌ Leave docs "for later" (you'll forget details)
- ❌ Update code without updating docs
- ❌ Assume /update-docs catches everything (some things need manual fixes)
- ❌ Let SESSION_STATE.md go >48 hours without update
- ❌ Create duplicate documentation (consolidate instead)

---

**Remember**: Documentation maintenance is not a separate task - it's part of every code change. Build it into your workflow, and future sessions will thank you.
