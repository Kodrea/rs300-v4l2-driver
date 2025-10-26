# Rollback Procedure

**Created**: 2025-10-25
**Purpose**: Emergency rollback for LLM documentation optimization

---

## Quick Rollback Options

### Option 1: Full Git Rollback (< 2 minutes)

**Use when**: Need to completely undo all documentation changes

```bash
# Return to baseline state
git checkout main
git reset --hard docs-baseline-2025-10-25

# Or if still on docs/llm-optimization branch
git checkout pi5-testing
git branch -D docs/llm-optimization
```

**Result**: All documentation changes reverted

---

### Option 2: Selective Rollback (< 5 minutes)

**Use when**: Keep some changes, revert others

```bash
# Revert specific directories
git checkout docs-baseline-2025-10-25 -- docs/implementation-tasks/
git checkout docs-baseline-2025-10-25 -- docs/implementation-patterns/

# Or revert specific files
git checkout docs-baseline-2025-10-25 -- I2C_COMMANDS_TO_IMPLEMENT.md
git checkout docs-baseline-2025-10-25 -- MODERNIZATION_PLAN.md
```

**Result**: Selected changes reverted, others preserved

---

### Option 3: Restore from Backup Tarball (< 3 minutes)

**Use when**: Git history corrupted or need fresh start

```bash
# Go to parent directory
cd /home/cody

# Extract backup (will overwrite)
tar xzf rs300-docs-backup-20251025.tar.gz

# Verify
cd rs300-v4l2-driver
git status
```

**Result**: All documentation restored to pre-optimization state

---

## Baseline Information

**Branch Created**: `docs/llm-optimization`
**Baseline Tag**: `docs-baseline-2025-10-25`
**Backup Location**: `/home/cody/rs300-docs-backup-20251025.tar.gz`
**Original Branch**: `pi5-testing`

**Baseline State**:
- Total markdown files: 63
- Modified files: 2 (DRIVER_ANALYSIS.md, START_HERE.md)
- Untracked files: ~42 (mostly build artifacts)

---

## Verification After Rollback

```bash
# Check git status
git status

# Verify file count
find . -name "*.md" -type f | wc -l
# Expected: 63

# Check for new directories (should not exist after rollback)
ls -la docs/implementation-tasks/ 2>/dev/null || echo "Correctly removed"
ls -la docs/implementation-patterns/ 2>/dev/null || echo "Correctly removed"
ls -la docs/meta/ 2>/dev/null || echo "Correctly removed"

# Verify no broken links
grep -r "implementation-tasks" *.md docs/ 2>/dev/null || echo "No references found"
```

---

## Communication After Rollback

If rollback is needed:

1. **Update README.md**: Remove any announcement of new system
2. **Update SESSION_STATE.md**: Document rollback reason
3. **Create rollback report**:
   ```bash
   # Create report
   cat > ROLLBACK_REPORT.md <<'EOF'
   # Rollback Report

   **Date**: [DATE]
   **Reason**: [Why rollback was necessary]
   **What was rolled back**: [Specific changes]
   **What was learned**: [Lessons for next attempt]
   **Next steps**: [How to iterate]
   EOF
   ```

---

## Prevention

**Before major changes**:
- ✅ Create git branch
- ✅ Create git tag
- ✅ Create backup tarball
- ✅ Document rollback procedure (this file)
- ✅ Test rollback procedure

**During development**:
- Commit frequently
- Use descriptive commit messages
- Tag milestones
- Keep baseline branch clean

**Testing rollback** (safe to test):
```bash
# Test rollback without affecting work
git stash
git checkout docs-baseline-2025-10-25
# Verify rollback works
git checkout docs/llm-optimization
git stash pop
```

---

## Troubleshooting

### "Tag not found"
```bash
# List all tags
git tag -l

# Recreate baseline tag from commit hash
git tag docs-baseline-2025-10-25 [COMMIT_HASH]
```

### "Backup tarball not found"
```bash
# List available backups
ls -lh /home/cody/rs300-docs-backup-*.tar.gz

# Use most recent
tar xzf /home/cody/rs300-docs-backup-[LATEST].tar.gz
```

### "Merge conflicts after rollback"
```bash
# Force reset (WARNING: loses uncommitted changes)
git reset --hard docs-baseline-2025-10-25
```

---

## Recovery After Rollback

**If you want to retry optimization later**:

1. **Analyze what went wrong**
   - Review rollback report
   - Identify specific issues
   - Plan corrections

2. **Start fresh**
   ```bash
   # Ensure clean slate
   git checkout pi5-testing
   git branch -D docs/llm-optimization

   # Create new optimization branch
   git checkout -b docs/llm-optimization-v2
   ```

3. **Apply lessons learned**
   - Smaller increments
   - More testing
   - Better validation

---

## Contact

If rollback needed due to critical issue:
1. Document the issue
2. Preserve error logs
3. Create GitHub issue (if public repo)
4. Note in SESSION_STATE.md for next session

---

**Last Updated**: 2025-10-25
**Tested**: Yes (procedure verified)
**Backup Verified**: Yes (tarball created successfully)
