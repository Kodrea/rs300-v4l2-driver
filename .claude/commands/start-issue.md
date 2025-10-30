# /start-issue

Start work on a GitHub issue with interactive questions, safety checks, branch setup, and context storage.

## EXECUTABLE COMMAND - Interactive Mode

This command:
1. Fetches and validates the GitHub issue
2. Shows the issue details clearly
3. Asks interactive questions (pull from remote, branch type, branch name)
4. Runs git pre-flight checks
5. Creates branch and stores issue context
6. Shows completion summary with next steps

## Usage

```
/start-issue 11
```

## Permissions

Required permissions in `.claude/settings.local.json` (all configured):
- `Bash(gh issue view:*)` - Fetch GitHub issue details
- `Bash(git rev-parse:*)` - Check git repo and branch state
- `Bash(git diff:*)` - Detect uncommitted changes
- `Bash(git fetch:*)` - Update remote tracking (non-destructive)
- `Bash(git checkout:*)` - Create and switch to new branch
- `Bash(git config:*)` - Store issue metadata in branch config

## EXECUTABLE STEPS

You are given an issue number as input. Follow these steps exactly:

### Step 1: Validate Issue and Get Details

Fetch the issue from GitHub. If it fails, show error and stop.

Once you have the issue details, show them in a clear format like:
```
═══════════════════════════════════════════════════════
ISSUE #{number}: {title}
═══════════════════════════════════════════════════════
State: {state}
Labels: {labels}
```

### Step 2: Pre-flight Git Checks

Before asking questions, run these checks:

1. Check inside git repo
2. Check on main branch
3. Check no uncommitted changes
4. Run `git fetch origin main` (non-destructive)

If any check fails, show error and stop. Do NOT proceed to interactive questions.

Show results clearly:
```
Git Pre-flight Checks:
  ✓ In git repository
  ✓ On main branch
  ✓ No uncommitted changes
  ✓ Main is up to date
```

### Step 3: Ask Interactive Questions

Once all pre-flight checks pass, use the AskUserQuestion tool to ask these questions in a single call:

**Question 1: Pull from Remote**
- Header: "Remote"
- Question: "Before creating branch, do you want to pull latest changes from remote?"
- Options:
  - "Yes, pull origin/main" - Run git pull origin main first
  - "No, skip" - Proceed without pulling
  - "Check status first" - Show git status before deciding

**Question 2: Branch Type**
- Header: "Branch Type"
- Question: "Select branch type for this issue"
- Options: Suggest based on issue labels (docs → docs/, bug → fix/, feature → feature/, refactor → refactor/)
  - Default to "Other" if no matching label
- Allow user to select "Other" for custom type

**Question 3: Branch Name**
- Header: "Branch Name"
- Question: "Suggest a descriptive branch name (lowercase, hyphens only)"
- Options:
  - Show suggested name based on issue title (e.g., "i2c-skill-output-mode")
  - Allow user to select "Other" to enter custom name

Process responses:
- If user wants to pull: `git pull origin main`
- Validate branch name format (lowercase, hyphens, numbers only, max 60 chars)
- Check branch doesn't already exist: `git branch -a | grep {name}`

### Step 4: Create Branch and Store Context

Once user responds to all questions:

```bash
git checkout -b {branch_type}/{branch_name}
git config branch.{branch_name}.issueNumber {issue_number}
```

Verify both commands succeeded.

### Step 5: Show Completion Summary

Display final summary:
```
═══════════════════════════════════════════════════════
✓ READY TO START
═══════════════════════════════════════════════════════

Issue: #{number} - {title}
Branch: {type}/{name}

Next Steps:
  1. Edit the files
  2. Commit: "Fixes #{number}"
  3. Push: git push origin {type}/{name}
  4. Create PR on GitHub

═══════════════════════════════════════════════════════
```

---

## Error Handling

| Situation | Action |
|-----------|--------|
| Issue doesn't exist | Show error, suggest checking issue number |
| Not authenticated | Show: "Run: gh auth login" |
| Not in git repo | Show: "Error: Not in git repository" |
| On wrong branch | Show: "Error: Checkout main first" |
| Uncommitted changes | Show: "Error: Commit or stash changes first" |
| Main behind origin | Show: "Error: Pull latest first" |
| Branch already exists | Show: "Error: Branch exists, use different name" |
| Invalid branch name | Show rules and ask again |

---

## Key Features

- **Pull Option**: User can opt to pull latest changes before creating branch
- **Label-based Suggestions**: Branch type automatically suggested from issue labels
- **Name Validation**: Ensures branch name follows convention (lowercase, hyphens, numbers only)
- **Context Storage**: Issue number stored in git config for later validation
- **Pre-flight Safety**: All git checks run before asking user questions

---

## System Requirements

- `gh` CLI installed and authenticated
- Inside git repository
- Git version 2.10+ (for branch config support)
