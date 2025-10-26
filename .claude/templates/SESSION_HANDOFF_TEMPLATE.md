# Session Handoff Guide

This document provides standardized checklists for starting and ending Claude Code sessions to ensure perfect continuity.

---

## 🚀 Starting a New Session

### Quick Start (2 minutes)

**Read these files in order**:

1. **SESSION_STATE.md** (AI-optimized) - 1 minute
   - Current work in progress
   - Blockers and questions
   - Recent decisions
   - Test status
   - Quick context summary

2. **START_HERE.md** (human-readable) - 30 seconds
   - Project status overview
   - Key file locations
   - Quick commands

3. **CLAUDE.md** (if needed) - 5-10 minutes
   - Navigate to relevant sections based on task
   - Use Quick Navigation table to find what you need

### Full Session Startup Checklist

- [ ] Read **SESSION_STATE.md** completely
  - Note any in-progress work
  - Check for blockers
  - Review recent decisions

- [ ] Check **git status**
  ```bash
  git status
  git log --oneline -5
  ```

- [ ] Verify **driver status** (if applicable)
  ```bash
  lsmod | grep rs300
  dmesg | grep -i rs300 | tail -5
  ```

- [ ] Review **recent test results**
  - Check SESSION_STATE.md "Test & Validation Status" section
  - Run `/test-camera` if significant time has passed

- [ ] Understand **current context**
  - What was the last session working on?
  - What should this session accomplish?
  - Are there any dependencies or prerequisites?

---

## 🏁 Ending a Session

### Before Ending Checklist

- [ ] **Commit all significant work**
  ```bash
  git status
  git add [files]
  git commit -m "descriptive message"
  ```

- [ ] **Run /update-docs** (automatic before commits)
  - Updates SESSION_STATE.md
  - Generates session note
  - Validates documentation

- [ ] **Update SESSION_STATE.md manually if needed**
  - Add any blockers encountered
  - Note decisions made and rationale
  - Update test status if tests were run
  - Update "In-Progress Work" section

- [ ] **Verify START_HERE.md is current** (if major changes)
  - Update project status if changed
  - Update quick commands if new workflows added
  - Update decision tree if priorities shifted

### Session End Documentation

Create or verify a brief session note exists in `.claude/sessions/`:

```bash
# Verify session note was auto-generated
ls -lt .claude/sessions/*.md | head -1

# Or create manually if /update-docs wasn't run
cp .claude/templates/session-note-template.md .claude/sessions/$(date +%Y-%m-%d_%H%M).md
# Edit with session details
```

**Session Note Should Include** (10-20 lines):
- What was accomplished
- What's still in progress
- Any blockers or issues
- Next steps for the following session

---

## 🔧 Mid-Session State Updates

### When to Update State

Update SESSION_STATE.md during the session when:

1. **After major discoveries or decisions**
   - Found root cause of a bug
   - Decided on architecture approach
   - Discovered important quirk or limitation

2. **Before long operations**
   - About to reboot system
   - Starting time-consuming build/test
   - Before testing risky changes

3. **When blocked**
   - Hit an error you can't immediately fix
   - Waiting for external information
   - Need to escalate or ask for help

4. **After test validation**
   - Completed test run with results
   - Validated security fixes
   - Confirmed functionality works

### Quick Update Command

```bash
# Edit SESSION_STATE.md
nano SESSION_STATE.md
# Update "Last Updated" timestamp
# Add to "In-Progress Work" or "Blockers & Questions"
```

---

## 🚨 Emergency Handoff

If session ends unexpectedly (crash, timeout, etc.):

### Immediate Recovery (Next Session)

1. **Check git status first**
   ```bash
   git status
   git diff
   ```

2. **Review SESSION_STATE.md**
   - May be stale if crash happened
   - Check "In-Progress Work" for what might be incomplete

3. **Check recent dmesg for errors**
   ```bash
   dmesg | tail -50
   ```

4. **Verify system state**
   - Is driver loaded? `lsmod | grep rs300`
   - Any crashes? `dmesg | grep -i "kernel bug\|oops"`
   - Disk full? `df -h`

5. **Resume work cautiously**
   - Run `/test-camera` to verify system health
   - Review last few commands if shell history available
   - Check for partially written files

---

## 📋 Session Types and Their Protocols

### 1. Quick Bug Fix Session (< 30 min)

**Start**: Read SESSION_STATE.md "Known Issues" → Fix → Test
**End**: Run /update-docs → Verify git commit → Session note auto-generated

### 2. Feature Development Session (1-3 hours)

**Start**: Read SESSION_STATE.md + relevant docs → Plan approach
**Mid**: Update STATE after design decisions
**End**: Run /update-docs → Update "In-Progress Work" → Commit with descriptive message

### 3. Testing/Validation Session (30-60 min)

**Start**: Read SESSION_STATE.md "Test Status" → Plan test scenarios
**Mid**: Update STATE with test results
**End**: Run /update-docs → Update "Test & Validation Status" section

### 4. Documentation Session (1-2 hours)

**Start**: Read SESSION_STATE.md → Identify stale docs
**Mid**: Run /update-docs periodically to validate changes
**End**: Run /update-docs → Verify all links and counts updated

---

## 🎯 Success Criteria for Good Handoff

A new Claude session should be able to answer these questions immediately from SESSION_STATE.md:

- ✅ What is the current project status?
- ✅ What work is in progress and how far along is it?
- ✅ Are there any blockers preventing progress?
- ✅ What was the last significant decision and why?
- ✅ When were tests last run and what was the result?
- ✅ What should the next session focus on?

If any of these cannot be answered quickly, the handoff documentation needs improvement.

---

## 📝 Tips for Effective Handoffs

### Do:
- ✅ Be specific about in-progress work (not "fixing driver" but "fixing ioctl NULL pointer dereference in rs300_set_ctrl")
- ✅ Document WHY decisions were made, not just WHAT was done
- ✅ Include test results with clear PASS/FAIL/WARN status
- ✅ Note blockers with enough context to understand the problem
- ✅ Keep session notes brief (10-20 lines) but informative

### Don't:
- ❌ Leave vague notes like "working on driver" or "testing stuff"
- ❌ Forget to update SESSION_STATE.md after major changes
- ❌ Skip /update-docs before committing
- ❌ Assume the next session will remember context
- ❌ Let SESSION_STATE.md become stale (update timestamps regularly)

---

## 🔄 Workflow Integration

### With Git Commits

```bash
# 1. Make changes
git add [files]

# 2. /update-docs runs automatically (pre-commit hook)
#    - Updates SESSION_STATE.md
#    - Generates session note
#    - Validates docs

# 3. Commit with descriptive message
git commit -m "fix: Add NULL check to prevent ioctl crash (CRITICAL-004)"

# 4. SESSION_STATE.md and session note automatically included
```

### With Testing

```bash
# 1. Run tests
/test-camera

# 2. Update SESSION_STATE.md with results
#    - Update "Test & Validation Status"
#    - Note any failures in "Blockers & Questions"

# 3. If significant results, run /update-docs
#    - Archives test results
#    - Updates quick context
```

---

**Remember**: Good documentation is an investment. The 5 minutes spent updating SESSION_STATE.md can save 30 minutes of confusion in the next session.
