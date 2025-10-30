# /start-issue

Start work on a GitHub issue with safety checks, branch setup, and context storage.

Automatically executable after trigger with full permissions configured.

## Permissions

Required permissions in `.claude/settings.local.json` (all configured):
- `Bash(gh issue view:*)` - Fetch GitHub issue details
- `Bash(git rev-parse:*)` - Check git repo and branch state
- `Bash(git diff:*)` - Detect uncommitted changes
- `Bash(git fetch:*)` - Update remote tracking (non-destructive)
- `Bash(git checkout:*)` - Create and switch to new branch
- `Bash(git config:*)` - Store issue metadata in branch config

All permissions are safe: read-only operations and non-destructive git commands.

## Usage

```
/start-issue 11
```

## What It Does

1. **Validates issue** - Exists, open, not assigned to someone else
2. **Shows details** - Title, description, files to modify, implementation plan
3. **Confirms understanding** - Asks if you understand scope before proceeding
4. **Pre-flight checks** - Git repo clean, on main branch, up to date
5. **Creates branch** - Asks for branch type and name, follows conventions
6. **Stores context** - Records issue number in git config for later validation
7. **Shows checklist** - Lists files to edit and phases to complete

## Command Flow

### Step 1: Parse Issue Number

User provides issue number (e.g., 11). Extract and validate it's a number.

```bash
gh issue view {issue_number} --json state,title,body,labels,assignees \
  --template '{{json .}}'
```

If issue doesn't exist or fetch fails, show error and stop.

### Step 2: Validate Issue State

Check:
- **Is it open?** (state == "OPEN")
  - If closed: "Issue #X is CLOSED. Continue anyway? (y/n)"
- **Is it unassigned?** (assignees is empty)
  - If assigned: "Issue assigned to [person]. Continue anyway? (y/n)"
- **Does it have a type label?** (bug, docs, enhancement, refactor, etc.)
  - If missing: "Warning: No type label. Proceeding anyway."

If any checks fail, ask user to confirm before continuing.

### Step 3: Display Issue Details

Show clearly formatted:
```
═══════════════════════════════════════════════════════
ISSUE #11: I2C Skill Output Mode Command Mismatch
═══════════════════════════════════════════════════════

Labels: docs, priority: high
Milestone: Beta v1.0
State: OPEN

Files to Modify:
  - .claude/skills/i2c-commands/SKILL.md
  - docs/reference/I2C_PROTOCOL.md

Implementation Plan:

Phase 1: Verify Correct Command Format
  ☐ Find correct I2C output mode command in rs300.c
  ☐ Confirm command header is 0x10/0x10/0x45
  ☐ Confirm packet size is 18 bytes

Phase 2: Update Skill File
  ☐ Remove incorrect 0x55/0x43/0x49 entry
  ☐ Add correct 0x10/0x10/0x45 entry with fields
  ☐ Add note: serial vs I2C protocol difference

Phase 3: Update Documentation
  ☐ Add output mode section to I2C_PROTOCOL.md
  ☐ Cross-reference to I2C_QUICK_REFERENCE.md

Acceptance Criteria:
  ☐ Skill shows correct I2C command format
  ☐ Serial vs I2C documented
  ☐ No conflicting information
═══════════════════════════════════════════════════════
```

### Step 4: Confirm Understanding

Ask user:
```
Do you understand the scope and requirements? (y/n)
```

If no:
- Show each phase with more detail
- Ask clarifying questions
- Let user ask: "What is X?" and explain
- Don't proceed until confident

If yes:
- Move to Step 5

### Step 5: Pre-flight Git Checks

Run these checks in order. If any fail, ask to fix first:

```bash
# Check 1: Inside git repo
if ! git rev-parse --git-dir > /dev/null 2>&1; then
  "Error: Not in a git repository"
  exit 1
fi

# Check 2: On main branch
current_branch=$(git rev-parse --abbrev-ref HEAD)
if [ "$current_branch" != "main" ]; then
  "Error: Not on 'main' branch (currently on '$current_branch')"
  "Switch to main first: git checkout main"
  exit 1
fi

# Check 3: No uncommitted changes
if ! git diff --quiet; then
  "Error: Uncommitted changes detected"
  "Commit or stash changes first"
  exit 1
fi

# Check 4: Main is up to date with origin
git fetch origin main 2>/dev/null
if ! git diff main origin/main --quiet; then
  "Warning: Your main is behind origin/main"
  "Run: git pull origin main"
  exit 1
fi
```

Show results:
```
Git Pre-flight Checks:
  ✓ In git repository
  ✓ On main branch
  ✓ No uncommitted changes
  ✓ Main is up to date
```

### Step 6: Determine Branch Type

Use issue labels to suggest branch type prefix:

```
Labels: docs, priority: high

Suggested branch type: docs/
Options: docs / fix / feature / refactor / other

Select branch type (or press Enter for suggested): docs/
```

Map labels to types:
- `type: docs` or `docs` → suggest `docs/`
- `type: bug` or `bug` → suggest `fix/`
- `type: enhancement` or `type: feature` → suggest `feature/`
- `type: refactor` → suggest `refactor/`
- Others → ask user

### Step 7: Branch Name

Ask user for descriptive branch name:

```
Issue title: I2C Skill Output Mode Command Mismatch
Suggested name: i2c-skill-output-mode

Enter branch name (or press Enter for suggested):
(Branch will be: docs/i2c-skill-output-mode)
```

Validate branch name:
- Only lowercase, hyphens, numbers (no spaces, underscores, etc.)
- Not too long (max 60 chars)
- Doesn't already exist

If invalid, ask again.

### Step 8: Create Branch

Run:
```bash
git checkout -b {branch_type}/{branch_name}
```

Verify:
```bash
current=$(git rev-parse --abbrev-ref HEAD)
if [ "$current" == "{branch_type}/{branch_name}" ]; then
  "✓ Branch created and checked out"
else
  "Error: Failed to create branch"
  exit 1
fi
```

### Step 9: Store Issue Context

Store issue number in git config so `/pr-ready` can validate later:

```bash
git config branch.{branch_name}.issueNumber {issue_number}
```

Verify:
```bash
stored=$(git config branch.{branch_name}.issueNumber)
if [ "$stored" == "{issue_number}" ]; then
  "✓ Issue #$stored linked to this branch"
else
  "Warning: Could not store issue number in git config"
fi
```

### Step 10: Show Completion Summary

```
═══════════════════════════════════════════════════════
✓ READY TO START
═══════════════════════════════════════════════════════

Issue: #11 - I2C Skill Output Mode Command Mismatch
Branch: docs/i2c-skill-output-mode
Files to edit:
  1. .claude/skills/i2c-commands/SKILL.md
  2. docs/reference/I2C_PROTOCOL.md

Next Steps:
  1. Edit the files above
  2. Test your changes
  3. Commit with message: "Fixes #11"
  4. Push: git push origin docs/i2c-skill-output-mode
  5. Create PR on GitHub
  6. Review on PR page (Files Changed tab)
  7. Merge when satisfied

Checklist for Phase 1:
  ☐ Find correct command in rs300.c
  ☐ Confirm 0x10/0x10/0x45 format
  ☐ Confirm 18-byte packet size

Ready to edit.
═══════════════════════════════════════════════════════
```

---

## Error Handling

| Situation | Action |
|-----------|--------|
| Issue doesn't exist | Show error, suggest checking issue number |
| User not authenticated | Show: "Run: gh auth login" |
| Not in git repo | Show: "Not in git directory" |
| On wrong branch | Show: "Checkout main first: git checkout main" |
| Uncommitted changes | Show: "Commit or stash changes first" |
| Main behind origin | Show: "Pull latest: git pull origin main" |
| Branch already exists | Ask: "Branch exists. Use different name?" |
| Invalid branch name | Show rules, ask again |
| Git config fails | Warn but continue (not critical) |

---

## Design Decisions

**Why ask "do you understand?" before creating branch?**
- Separates reading/understanding from doing/acting
- If user realizes they misunderstood, they haven't created branch yet
- Prevents throwaway branches

**Why store issue number in git config?**
- `/pr-ready` can later validate that commit has "Fixes #X" for right issue
- If user forgets "Fixes #11", we can catch it before merge
- Provides context without file-based state

**Why pre-flight checks?**
- Prevents creating branch on wrong base
- Prevents branch with mixed changes from other work
- Catches common mistakes early

**Why validate issue state (open/unassigned)?**
- Even solo dev: prevents wasted work on closed issues
- Good practice for potential future collaborators
- User can still force through if they want

**Why ask for branch type and name separately?**
- Respects your branch conventions (type prefix)
- User decides name (understands what they're working on)
- Both are validated
- Follows git best practices

---

## System Requirements

- `gh` CLI installed and authenticated
- Inside git repository
- Git version 2.10+ (for branch config support)

## Testing

Test with issue #11 (known good state):
```
/start-issue 11
→ Validate issue open ✓
→ Show plan ✓
→ Confirm understanding ✓
→ Check git state ✓
→ Ask branch type ✓
→ Ask branch name ✓
→ Create docs/i2c-skill-output-mode ✓
→ Store issue context ✓
→ Show summary ✓
```

Then `git log --oneline` should show you're on the branch.
And `git config branch.i2c-skill-output-mode.issueNumber` should output `11`.
