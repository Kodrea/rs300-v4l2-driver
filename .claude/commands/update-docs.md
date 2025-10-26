---
requiredPermissions:
  - "Bash(find:*)"
  - "Bash(wc:*)"
  - "Bash(cat:*)"
  - "Bash(echo:*)"
  - "Bash(grep:*)"
  - "Bash(git branch:*)"
  - "Bash(git log:*)"
  - "Bash(git status:*)"
  - "Bash(git diff:*)"
  - "Bash(date:*)"
  - "Bash(ls:*)"
  - "Bash(cp:*)"
  - "Bash(cut:*)"
  - "Bash(sort:*)"
  - "Edit(.claude/SESSION_STATE.md)"
  - "Write(.claude/sessions/*.md)"
---

# Update Documentation System

Automatically maintain project documentation, validate links, update file counts, refresh session state, and generate session notes for seamless handoff between Claude Code sessions.

---

## What This Command Does

This command performs comprehensive documentation maintenance:

1. **File Statistics**: Count and update .md file counts in CLAUDE.md
2. **Git Status**: Update SESSION_STATE.md with current branch, commits, modifications
3. **Link Validation**: Check all documentation links point to existing files
4. **Staleness Detection**: Find outdated timestamps and information
5. **Session Note**: Generate brief summary of current session in .claude/sessions/
6. **Health Report**: Show overall documentation health status

---

## Execution Steps

Execute the following tasks sequentially and report results:

### 1. Count Documentation Files

```bash
echo "=== DOCUMENTATION FILE COUNTS ==="

# Count root .md files
find . -maxdepth 1 -name "*.md" -type f | wc -l > /tmp/root_count.txt
cat /tmp/root_count.txt

# Count docs/ hierarchy files
find docs/ -name "*.md" -type f 2>/dev/null | wc -l > /tmp/docs_count.txt
cat /tmp/docs_count.txt

# Count .claude/ files
find .claude/ -name "*.md" -type f 2>/dev/null | wc -l > /tmp/claude_count.txt
cat /tmp/claude_count.txt

# README.md line count (only .md file in root)
echo "README.md lines:"
wc -l < README.md 2>/dev/null || echo "0"
```

**Expected**: Clear count of documentation files (counts saved to /tmp/*.txt for later use)

### 2. Check Git Status

```bash
echo ""
echo "=== GIT STATUS ==="

# Current branch
git branch --show-current > /tmp/git_branch.txt
cat /tmp/git_branch.txt

# Last commit
git log --oneline -1 > /tmp/git_commit.txt
cat /tmp/git_commit.txt

# Uncommitted changes
git status --short | grep "^ M" | wc -l > /tmp/git_modified.txt
git status --short | grep "^??" | wc -l > /tmp/git_untracked.txt
echo "Modified files:"
cat /tmp/git_modified.txt
echo "Untracked files:"
cat /tmp/git_untracked.txt
```

**Expected**: Current git state summary (saved to /tmp/git_*.txt files)

### 3. Validate Documentation Links

```bash
echo ""
echo "=== LINK VALIDATION ==="

# Extract .md file references and save to file
grep -roP '\[[^\]]+\]\(\K[^)]+\.md' README.md docs/ .claude/ 2>/dev/null | cut -d':' -f2 | sort -u > /tmp/doc_links.txt

# Note: Link validation can produce false positives for relative paths
# (e.g., ../README.md from subdirectories are valid but reported as broken)
echo "Checking documentation links (note: relative paths may show as broken)..."
echo "Link validation is informational - manual review recommended"
```

**Expected**: Link extraction complete (full validation requires manual review due to relative path complexity)

### 4. Check for Staleness

```bash
echo ""
echo "=== STALENESS CHECK ==="

# Check SESSION_STATE.md last updated
grep "Last Updated:" .claude/SESSION_STATE.md | head -1

# Check for TODO or FIXME in recent git changes
git diff HEAD~5..HEAD 2>/dev/null | grep -c "TODO\|FIXME" > /tmp/todo_count.txt || echo "0" > /tmp/todo_count.txt
echo "TODO/FIXME in recent commits:"
cat /tmp/todo_count.txt
```

**Expected**: Staleness check results (timestamp and TODO count)

### 5. Update SESSION_STATE.md

```bash
echo ""
echo "=== UPDATING SESSION_STATE.md ==="

# Backup current SESSION_STATE.md
cp .claude/SESSION_STATE.md .claude/SESSION_STATE.md.bak
echo "✅ Backed up to .claude/SESSION_STATE.md.bak"

# Note: SESSION_STATE.md should be updated using Edit tool for reliability
# The slash command will prompt Claude to update it with current git status
echo "Claude will update .claude/SESSION_STATE.md with:"
echo "  - Current timestamp"
echo "  - Git branch from /tmp/git_branch.txt"
echo "  - Git status from /tmp/git_modified.txt and /tmp/git_untracked.txt"
echo "  - Last commit from /tmp/git_commit.txt"
```

**Expected**: SESSION_STATE.md backup created (Claude will update main file with Edit tool)

### 6. Generate Session Note

```bash
echo ""
echo "=== GENERATING SESSION NOTE ==="

# Check if a note already exists for this hour
date +%Y-%m-%d_%H > /tmp/session_hour.txt
ls .claude/sessions/*.md 2>/dev/null | grep -f /tmp/session_hour.txt > /tmp/existing_session.txt

if [ -s /tmp/existing_session.txt ]; then
    echo "⚠️  Session note already exists for this hour:"
    cat /tmp/existing_session.txt
    echo "Skipping generation to avoid duplicates"
else
    # Note: Claude will create the session note using Write tool
    # This ensures proper formatting and variable expansion
    date +%Y-%m-%d_%H%M > /tmp/session_timestamp.txt
    echo "✅ Claude will create session note at:"
    echo ".claude/sessions/"
    cat /tmp/session_timestamp.txt
    echo ".md"
fi
```

**Expected**: Check for existing session note (Claude will create new one with Write tool if needed)

### 7. Check CLAUDE.md Accuracy

```bash
echo ""
echo "=== CHECKING CLAUDE.md ACCURACY ==="

# Extract file count from CLAUDE.md
grep "Files.*files" docs/CLAUDE.md | head -1 > /tmp/claude_filecount.txt
cat /tmp/claude_filecount.txt

echo ""
echo "Actual counts:"
echo "  Root .md:"
cat /tmp/root_count.txt
echo "  docs/:"
cat /tmp/docs_count.txt
echo "  .claude/:"
cat /tmp/claude_count.txt
```

**Expected**: Comparison of documented vs actual file counts

### 8. Generate Health Report

```bash
echo ""
echo "=== DOCUMENTATION HEALTH REPORT ==="
echo ""
echo "✅ Update checks complete!"
echo ""
echo "Summary:"
echo "  - File counts saved to /tmp/*_count.txt"
echo "  - Git status saved to /tmp/git_*.txt"
echo "  - SESSION_STATE.md backed up"
echo "  - Claude will complete SESSION_STATE.md and session note updates"
echo ""
echo "Next: Claude will use Edit/Write tools to update:"
echo "  1. .claude/SESSION_STATE.md (with current git status)"
echo "  2. Create session note (if needed)"
echo "  3. Report final health status"
```

**Expected**: Summary of update process completion

---

## Output Format

Provide output in this format:

```
=== DOCUMENTATION UPDATE REPORT ===

File Counts:
  Root .md: 1 file (README.md only)
  docs/: 37 files (includes CLAUDE.md, START_HERE.md, reference/)
  .claude/: 30 files (includes SESSION_STATE.md)
  Total: 68 files

Git Status:
  Branch: pi5-testing
  Last: faa1141 - docs: Optimize I2C lookups
  Status: DIRTY (3 modified, 15 untracked)

Validation:
  ✅ All links valid
  ✅ No staleness detected
  ✅ CLAUDE.md counts accurate

Updates:
  ✅ .claude/SESSION_STATE.md updated
  ✅ Session note created: .claude/sessions/2025-10-24_1615.md

Health: ✅ GREEN (All systems good)
```

---

## Notes

- This command is **safe to run multiple times** - it won't create duplicate session notes within the same hour
- Manual review of generated session note recommended to add specific details
- Some validations are approximate and may need manual verification
- If HEALTH is RED, review and fix issues before committing
- Run this command **before making git commits** for best session handoff

---

## Integration

This command should be run:
- ✅ Before ending a Claude Code session
- ✅ Before making git commits (ideally via pre-commit hook)
- ✅ After major changes (new features, bug fixes, refactoring)
- ✅ Periodically during long sessions (every 1-2 hours)

**Tip**: The more frequently you run /update-docs, the easier session handoffs become!
