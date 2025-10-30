# GitHub Workflow Tutorial: Issue #11 (I2C Skill Output Mode)

**Purpose**: Teach you how GitHub components work together by doing real work on issue #11.

**Outcome**: You'll understand ISSUES → PLANS → BRANCHES → PRs → MERGES through hands-on example.

---

## **WHAT IS ISSUE #11?**

Open GitHub, go to Issues, click #11:

```
Title: I2C Skill Output Mode Command Mismatch
Labels: docs, priority: high
Milestone: Beta v1.0
Status: Open

Description:
  Skill has incorrect output mode command format.
  Driver has been fixed, but skill documentation needs updating.

  Skill documented (WRONG): 0x55/0x43/0x49 (23-byte serial format)
  Driver uses (CORRECT): 0x10/0x10/0x45 (18-byte I2C format)
```

**Translation**: "The I2C skill file has wrong info. Fix it to match what driver actually uses."

---

## **WORKFLOW FLOW - The Big Picture**

```
YOU OPEN ISSUE #11
        ↓
YOU READ THE ISSUE (understand what's needed)
        ↓
YOU CREATE A PLAN (add to issue description)
        ↓
YOU DO RESEARCH (if needed - optional)
        ↓
YOU CREATE A BRANCH (safe workspace)
        ↓
YOU MAKE CHANGES (edit skill file + docs)
        ↓
YOU COMMIT LOCALLY (save changes with message)
        ↓
YOU PUSH BRANCH (send to GitHub)
        ↓
YOU CREATE A PR (request to merge)
        ↓
YOU REVIEW YOUR OWN CODE (GitHub PR page)
        ↓
YOU MERGE (changes go to main)
        ↓
ISSUE #11 AUTO-CLOSES (GitHub sees "Fixes #11" in PR)
        ↓
PROJECT BOARD UPDATES (moves to Done)
        ↓
DONE
```

Now let's walk through EACH step.

---

## **STEP 1: READ THE ISSUE**

**Location**: GitHub.com → Issues → #11

**What you do**:
1. Click issue #11
2. Read the title: What needs fixing?
   - "I2C Skill Output Mode Command Mismatch"
3. Read description: Why and what?
   - Wrong command in skill file
   - Driver is already fixed
   - Need to update skill docs
4. Check labels: What type of work?
   - `docs` (documentation)
   - `priority: high` (important)
5. Check milestone: What release?
   - `Beta v1.0` (this should ship with v1.0)

**Your understanding now**:
> "I need to update the I2C skill file to show correct command format. It's documentation work, high priority."

---

## **STEP 2: OPTIONAL - CREATE A DISCUSSION (IF RESEARCH NEEDED)**

**Location**: GitHub.com → Discussions (only if you need to research)

For issue #11, you probably DON'T need research because:
- The correct format is already documented in driver comments
- The problem is clearly stated in the issue
- You just need to copy the correct info to skill file

**Skip this if**: Work is straightforward (like #11)

**Do this if**: You need to explore options or understand deeply

---

## **STEP 3: CREATE/UPDATE THE PLAN**

**Location**: GitHub.com → Issue #11 → Edit description

Right now, issue #11 just describes the problem. Add a PLAN section:

Click "Edit" on the issue description and add:

```markdown
## Implementation Plan

### Phase 1: Verify Correct Command Format
- [ ] Find correct I2C output mode command in rs300.c
- [ ] Confirm command header is 0x10/0x10/0x45
- [ ] Confirm packet size is 18 bytes
- [ ] Confirm mode field position and CRC position

### Phase 2: Update Skill File
- [ ] Remove incorrect 0x55/0x43/0x49 entry
- [ ] Add correct 0x10/0x10/0x45 entry with proper fields
- [ ] Add note distinguishing serial vs I2C protocol

### Phase 3: Update Documentation
- [ ] Add output mode command section to I2C_PROTOCOL.md
- [ ] Cross-reference to I2C_QUICK_REFERENCE.md

## Acceptance Criteria
- [ ] Skill file shows correct I2C command format
- [ ] No conflicting information in docs
- [ ] Serial vs I2C protocol difference is documented
```

**Why plan in the issue?**
- Anyone opening issue #11 sees the plan
- Checkboxes let you track progress
- Plan stays with the work (when issue closes, plan is archived)

---

## **STEP 4: CREATE A BRANCH (Safe Workspace)**

**Location**: Your terminal/Claude Code

A branch is like making a copy of the code to work on safely.

```bash
git checkout -b fix/i2c-skill-output-mode
```

**What this does**:
```
BEFORE:
main branch (untouched, the "official" version)

AFTER:
main branch (untouched)
fix/i2c-skill-output-mode branch (YOUR copy, ready to edit)
```

**Why branch?**
- You work on YOUR copy, not the official one
- If you mess up, main is safe
- No one sees your work until you want them to
- Easy to undo: `git checkout main` then delete branch

---

## **STEP 5: MAKE CHANGES (Edit Files)**

**Location**: Your terminal/Claude Code

Now you edit the files. For issue #11:

### **File 1: `.claude/skills/i2c-commands/SKILL.md`**

Find the OUTPUT MODE command section. Currently (WRONG):
```markdown
## Output Mode (0x55/0x43/0x49) - SERIAL FORMAT

[23-byte packet structure]
[WRONG INFO]
```

Change to (CORRECT):
```markdown
## Output Mode (0x10/0x10/0x45) - I2C FORMAT

Packet structure: 18 bytes
Byte 0-1: 0x10, 0x10
Byte 2: 0x45
Byte 3: reserved (0x00)
Byte 4: mode (0=White Hot, 1=Ironbow, etc.)
...
Bytes 16-17: CRC-16-CCITT

### Serial vs I2C Note
This project uses I2C protocol (0x10/0x10/0x45).
RS300 also supports serial protocol (0x55/0x43/0x49) but driver doesn't use it.
Make sure you're using I2C format.
```

### **File 2: `docs/reference/I2C_PROTOCOL.md`**

Add new section:
```markdown
## Output Mode Command (0x10/0x10/0x45)

Sets the thermal image color palette/mode.

### Packet Structure (18 bytes)
| Byte(s) | Value | Description |
|---------|-------|-------------|
| 0-1 | 0x10, 0x10 | Command header |
| 2 | 0x45 | Output mode subcommand |
| 3 | 0x00 | Reserved |
| 4 | mode | Mode value (0-11) |
| 5-15 | 0x00 | Reserved |
| 16-17 | CRC | CRC-16-CCITT checksum |

### Mode Values
- 0: White Hot
- 1: Ironbow
- ... (complete list)

### Example
Set to Ironbow (mode 1):
```
10 10 45 00 01 00 00 00 00 00 00 00 00 [CRC_H CRC_L]
```
```

---

## **STEP 6: COMMIT LOCALLY (Save with Message)**

**Location**: Your terminal/Claude Code

You've made changes. Now save them with a message:

```bash
git add .claude/skills/i2c-commands/SKILL.md docs/reference/I2C_PROTOCOL.md
git commit -m "docs: fix I2C skill output mode command format

Fixes #11

- Updated skill file to show correct I2C command (0x10/0x10/0x45)
- Removed incorrect serial command (0x55/0x43/0x49)
- Added output mode section to I2C_PROTOCOL.md
- Added note distinguishing serial vs I2C protocol formats

The driver was already using correct format.
Skill documentation was outdated (referenced serial protocol)."
```

**Breaking down the commit message**:
```
docs: fix I2C skill output mode command format
      ↑ (type: what kind of change)

Fixes #11
         ↑ (CRITICAL: tells GitHub this fixes issue #11)

- Updated skill file...
  ↑ (bullet points of what changed)
```

**Why "Fixes #11"?**
When you later merge this commit, GitHub sees "Fixes #11" and:
1. Automatically closes issue #11
2. Links the commit to the issue
3. Shows in issue history: "Fixed by commit XYZ"

---

## **STEP 7: PUSH BRANCH (Send to GitHub)**

**Location**: Your terminal/Claude Code

You've committed locally (on your computer). Now send to GitHub:

```bash
git push origin fix/i2c-skill-output-mode
```

**What happens**:
```
BEFORE PUSH:
GitHub.com → Branch "main" exists
             Branch "fix/i2c-skill-output-mode" does NOT exist
Your computer → Branch "fix/i2c-skill-output-mode" exists (with your commits)

AFTER PUSH:
GitHub.com → Branch "main" exists
             Branch "fix/i2c-skill-output-mode" exists (your commits are here)
Your computer → Same as before
```

**Verification**: Go to GitHub.com → Code tab → Branch dropdown
You'll see `fix/i2c-skill-output-mode` listed.

---

## **STEP 8: CREATE A PULL REQUEST (Request to Merge)**

**Location**: GitHub.com

Go to GitHub. You'll see a notification:

```
🟡 Compare & pull request

fix/i2c-skill-output-mode had recent pushes a minute ago
[Create Pull Request] button
```

Click "Create Pull Request". You see:

```
Comparing:
  base: main (where changes will go)
  compare: fix/i2c-skill-output-mode (where changes come from)

Title: [pre-filled with your last commit message]
       docs: fix I2C skill output mode command format

Description: [pre-filled with commit message]
             Fixes #11

             - Updated skill file...
             [etc]
```

You can edit or leave as-is. Then click green "Create Pull Request" button.

**Result**: PR #XX is created

---

## **STEP 9: REVIEW YOUR OWN CODE (Self-Review)**

**Location**: GitHub.com → Pull Request #XX (the page that was just created)

Now you review your own work. This is THE crucial step.

### **What to do**:

**1. Read "Conversation" tab**
```
Title: docs: fix I2C skill output mode command format
Description: Shows what you changed and why
Status: OPEN (not merged yet)
```

Ask yourself: "Does the description explain the problem clearly?"

**2. Click "Files Changed" tab**

This shows EVERY line you changed:

```
.claude/skills/i2c-commands/SKILL.md
─────────────────────────────────────
Line 42:
- ## Output Mode (0x55/0x43/0x49) - SERIAL FORMAT
+ ## Output Mode (0x10/0x10/0x45) - I2C FORMAT

Line 45:
- [23-byte packet]
+ Packet structure: 18 bytes

Line 50:
+ Byte 0-1: 0x10, 0x10
+ Byte 2: 0x45

[... more changes ...]

docs/reference/I2C_PROTOCOL.md
──────────────────────────────
[NEW SECTION ADDED - green lines]
+ ## Output Mode Command (0x10/0x10/0x45)
+
+ Sets the thermal image color palette...
```

**3. Review each change**

For each line, ask:
- "Is this right?" ✓
- "Did I break anything?" ✗
- "Is the format correct?" ✓
- "Did I miss anything?" (thinking...)

If you find issues, you fix them:
```bash
# Back on your computer:
git checkout fix/i2c-skill-output-mode
# (edit the file)
git commit -m "fix: add missing mode values to output mode docs"
git push origin fix/i2c-skill-output-mode

# PR auto-updates with new commit
# Go back to GitHub PR page, review again
```

**4. Check the links**

In PR description, you wrote "Fixes #11". GitHub shows:
```
✓ This pull request closes issue #11
```

Click on issue #11 link to verify it's the right issue.

**5. Approve and merge**

Once satisfied, click green "Merge Pull Request" button.

You see:
```
✓ Merge pull request

Commit message: docs: fix I2C skill output mode command format

Merge button shows:
"Merge pull request #XX into main from fix/i2c-skill-output-mode"

[Confirm merge] button
```

Click "Confirm merge".

---

## **STEP 10: MERGED! - Automatic Updates**

**Location**: GitHub.com (automatic)

When you merge, GitHub does:

1. **Merges changes into main**
   ```
   Before: main branch has old skill file
   After: main branch has corrected skill file
   ```

2. **Closes PR #XX**
   ```
   PR status: MERGED ✓ (green checkmark)
   Shows: "Merged 1 commit into main"
   ```

3. **Closes issue #11**
   ```
   Issue status: CLOSED ✓ (purple checkmark)
   Shows: "closed by PR #XX"
   Click the PR link → see your changes
   ```

4. **Updates project board**
   ```
   Project board card #11 moves: Todo → Done
   Shows: "Completed"
   ```

5. **Deletes branch**
   ```
   GitHub suggests: "Delete branch fix/i2c-skill-output-mode?"
   Click "Delete branch" to clean up
   ```

**Result**: Issue #11 is completely resolved.

---

## **THE GITHUB COMPONENTS IN ACTION**

**Map each component to what happened:**

| Component | Used For | In Issue #11 |
|-----------|----------|-------------|
| **ISSUE #11** | "Here's work to do" | Starting point - problem statement |
| **ISSUE DESCRIPTION** | "Here's what needs to happen" | Added implementation plan |
| **LABELS** | "What type/priority" | `docs`, `priority: high` |
| **MILESTONE** | "What release" | `Beta v1.0` |
| **BRANCH** | "Safe workspace to edit" | `fix/i2c-skill-output-mode` |
| **COMMITS** | "Save work + explain why" | "docs: fix I2C skill..." |
| **PR** | "Request to merge, with review" | PR #XX showing all changes |
| **PR - Files Changed** | "See exactly what changed" | Shows old vs new skill file |
| **PR - Review** | "Check if changes are good" | You review your own code |
| **MERGE** | "Accept changes into main" | Click merge button |
| **PROJECT BOARD** | "Visual status dashboard" | Automatically moves #11 to Done |

---

## **COMPLETE TIMELINE FOR ISSUE #11**

```
Monday 9am:
  - You're working on something else
  - See issue #11 in project board (Todo column)

Monday 2pm:
  - Decide to work on #11
  - `git checkout -b fix/i2c-skill-output-mode`
  - Edit skill file: 0x55/0x43/0x49 → 0x10/0x10/0x45
  - Edit I2C_PROTOCOL.md: add output mode section
  - `git add ...` and `git commit`
  - `git push origin fix/i2c-skill-output-mode`

Monday 3pm:
  - Go to GitHub
  - Create PR #XX
  - Review your changes on "Files Changed" tab
  - Everything looks good
  - Click "Merge Pull Request"

Monday 3:05pm (automatic):
  - PR merges
  - Issue #11 closes
  - Project board updates
  - Branch deleted

Tuesday (next session):
  - Check project board
  - See #11 in Done column ✓
  - See issue #11: "Fixed by PR #XX"
  - Click PR to see what you did Monday
```

---

## **KEY LEARNINGS**

**1. Issues are the STARTING POINT**
- Issue = "Here's work to do"
- Everything else connects to it

**2. Plans go IN the issue**
- Not in separate files
- Visible to anyone looking at the work

**3. Branches are SAFE PLAYGROUNDS**
- Work on your copy, not the official one
- Easy to undo if needed

**4. Commits EXPLAIN your changes**
- Message explains WHY, not just WHAT
- "Fixes #11" links to the issue

**5. PRs are REVIEWS**
- Even working alone, review your code
- "Files Changed" shows exactly what changed
- Catch mistakes before they go to main

**6. GitHub AUTOMATES the closing**
- "Fixes #11" in PR → issue auto-closes
- One source of truth (no manual updates)

**7. Project board UPDATES AUTOMATICALLY**
- Merge PR → issue closes → board updates
- You never manually move cards (if PR is set up right)

---

## **READY FOR ISSUE #11?**

You now understand:
- What #11 is about
- How to create a branch
- How to make changes
- How to review your work
- How to merge
- What happens automatically

**Next time you work on a feature:**
1. Pick an issue from project board
2. Create a branch with descriptive name
3. Make changes
4. Create PR
5. Review on GitHub
6. Merge
7. Done - everything auto-closes

**This workflow is the same for:**
- Issue #11 (I2C Skill)
- Issue #5 (Driver Cleanup)
- Issue #6 (Kernel Logging)
- Any issue in your project

---

## **QUICK REFERENCE: Command Checklist**

```bash
# 1. CREATE BRANCH
git checkout -b fix/short-description

# 2. MAKE CHANGES
(edit files)

# 3. COMMIT
git add .
git commit -m "type: description. Fixes #XX

- Detail 1
- Detail 2"

# 4. PUSH
git push origin fix/short-description

# 5. CREATE PR
(Go to GitHub, click "Create Pull Request")

# 6. REVIEW
(Go to PR page, click "Files Changed", review)

# 7. MERGE
(Click "Merge Pull Request" button on GitHub)

# Done - issue auto-closes, board updates
```

---

