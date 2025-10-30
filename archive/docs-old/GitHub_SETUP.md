# GitHub Setup Guide for RS300 Driver

This guide shows how to set up GitHub features to replace scattered .md documentation with a GitHub-centric workflow.

---

## Project Directory Structure

After recent consolidations, the project is organized as:

```
rs300-v4l2-driver/
├── .claude/                    # AI session documentation
│   ├── SESSION_STATE.md       # Session status (will link to GitHub)
│   └── lessons-learned/       # Design decisions & solutions
├── docs/                       # USER DOCUMENTATION (minimal & focused)
│   ├── getting-started/
│   │   ├── installation.md    # Pi 4 & 5 combined installation
│   │   └── first-capture.md   # Quick start guide
│   └── reference/             # Technical reference (4 files)
│       ├── SETUP_AND_TROUBLESHOOTING.md   # Setup & troubleshooting
│       ├── DRIVER_ANALYSIS.md             # Driver internals
│       ├── DEV_QUICK_REFERENCE.md         # Command cheat sheet
│       ├── RS300_Media_Pipeline_Guide.md # Pipeline architecture
│       └── SECURITY_AUDIT.md              # Security analysis
├── utilities/                  # Helper tools & configurations
│   ├── config/                # Configuration files
│   ├── scripts/               # Helper scripts (udev, systemd)
│   └── examples/              # Example usage & test code
├── archive/                    # Preserved older documentation
│   └── docs-old/              # Archived documentation
├── CLAUDE.md                   # Project guide (authority)
├── README.md                   # User-facing project page
└── rs300.c, setup.sh, etc.    # Core driver code
```

**Key Points:**
- `docs/` contains only essential, actively maintained documentation
- `utilities/` organizes helper tools cleanly separate from code
- `CLAUDE.md` is the single source of truth for project metadata
- `archive/` preserves old docs for reference

---

## Overview

Instead of maintaining 70+ .md files for tracking issues, decisions, and status, we use GitHub's built-in features as the system of record:

- **Issues** → Track bugs, enhancements, tasks
- **Project Board** → Visual task status (Kanban)
- **Discussions** → Architecture decisions, design docs
- **Milestones** → Release phases and planning
- **Wiki** → Shared knowledge (optional)

---

## 1. Create GitHub Project Board

Create a new **Project** to track work visually.

### Setup Steps

1. Go to repository → **Projects** tab
2. Click **New project**
3. **Name**: "RS300 Driver Development"
4. **Description**: "Kanban board for RS300 thermal camera driver development"
5. **Template**: Choose **Board** (visual Kanban layout)
6. Click **Create project**

### Configure Status Field

Board template includes default **Status** field (Single Select type).

Edit the Status field values to create Kanban columns:
- **Backlog** - Not yet prioritized
- **Todo** - Ready to start
- **In Progress** - Currently being worked on
- **Review** - Waiting for review/approval
- **Done** - Completed

**Result**: Visual board with 5 columns, drag issues between them

### Automation Rules

Link to GitHub Actions to auto-update:
- Move to "In Progress" when PR opened
- Move to "Review" when PR ready
- Move to "Done" when PR merged

---

## 2. Create Standard Labels

Labels help organize issues by type and priority.

### Type Labels
```
type: bug          - 🐛 Bug/defect
type: enhancement  - ✨ New feature/improvement
type: docs         - 📖 Documentation
type: refactor     - 🔄 Code refactoring
type: test         - ✅ Testing/validation
type: chore        - 🧹 Maintenance
```

### Platform Labels
```
platform: pi5      - Raspberry Pi 5 specific
platform: pi4      - Raspberry Pi 4 specific
security           - 🔒 Security issue
performance        - 📊 Performance optimization
```

### Priority Labels
```
priority: critical - 🔴 Must fix immediately
priority: high     - 🟠 Important, address soon
priority: medium   - 🟡 Normal priority
priority: low      - 🟢 Nice to have
```

### Status Labels
```
status: blocked    - ⚠️ Blocked on something
status: help       - 🆘 Help needed
status: question   - ❓ Need clarification
```

---

## 3. Create Issue Templates

### Bug Report Template

```markdown
## Description
Brief description of the bug

## Steps to Reproduce
1. ...
2. ...
3. ...

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Environment
- Platform: (Pi 5/Pi 4)
- Driver version: (commit hash)
- Camera model: RS300

## Logs
dmesg output, etc.
```

### Enhancement Template

```markdown
## Description
What feature/improvement?

## Use Case
Why is this needed?

## Proposed Solution
How should it work?

## Alternative Solutions
Any alternatives considered?

## Additional Context
Links, related issues, etc.
```

### Task Template

```markdown
## Description
What needs to be done?

## Acceptance Criteria
- [ ] Criterion 1
- [ ] Criterion 2
- [ ] Criterion 3

## Related
Links to related issues

## Notes
Any important context
```

---

## 4. Create Milestones

Milestones represent release phases or major goals.

**Suggested Milestones:**

- **Beta v1.0** - Initial beta release (current)
- **Production v1.0** - Production-ready release
- **v1.1** - Planned enhancements
- **Experimental** - Long-term research features

Each milestone should list:
- Target completion date
- Key deliverables
- Linked issues

---

## 5. Migration Strategy

### Move Existing Knowledge to GitHub

**From archived docs → GitHub Issues/Discussions:**

1. **Known Issues** → GitHub Issues with labels
   - From SETUP_AND_TROUBLESHOOTING.md → `type: bug` issues
   - From known issues list → `status: blocked` issues

2. **Design Decisions** → GitHub Discussions
   - Why certain approaches were taken
   - Lessons learned from mistakes
   - Architecture rationale

3. **Future Work** → GitHub Issues
   - From roadmaps/plans → `type: enhancement` issues
   - Break into smaller deliverables
   - Link to milestones

4. **Lessons Learned** → Wiki (optional)
   - Keep `.claude/lessons-learned/` as quick reference
   - Link from relevant GitHub issues

### Update Session State

`.claude/SESSION_STATE.md` becomes minimal (2-3 lines per section):
- **Current Phase**: Brief description
- **Blockers**: Link to GitHub Issues
- **Next Steps**: Brief summary
- **Key Context**: Link to CLAUDE.md for project metadata and essential documentation

---

## 6. Commit Message Integration

Use conventional commit format to link to GitHub:

```
type(scope): brief description

Longer explanation if needed.

Fixes: #123
Related: #456, #789
```

GitHub automatically:
- Links commits to issues
- Closes issues when PR merged with "Fixes:"
- Shows commit history in issue timeline

---

## 7. PR Workflow

### Branch Naming
```
feature/add-xyz
fix/issue-123
docs/update-xyz
```

### PR Template (in `.github/pull_request_template.md`)

```markdown
## Description
What does this PR do?

## Related Issues
Closes #123
Related to #456

## Changes
- Change 1
- Change 2

## Testing
How to verify the changes?

## Checklist
- [ ] Code compiles without errors
- [ ] Tests pass (if applicable)
- [ ] Documentation updated
- [ ] No new security issues
```

---

## 8. Discussions vs Issues

### Use GitHub Discussions for:
- Architecture decisions ("Should we refactor XYZ?")
- Design questions ("What's the best way to handle ABC?")
- Knowledge sharing ("Best practices for V4L2")
- Long-form documentation

### Use GitHub Issues for:
- Bugs (trackable, assignable, closeable)
- Features (with acceptance criteria)
- Tasks (with checklists)
- Any work item with clear completion

---

## Benefits of This Approach

✅ **Centralized** - Everything in GitHub, single source of truth
✅ **Visual** - Project board shows status at a glance
✅ **Linked** - Commits, PRs, issues all connected
✅ **Searchable** - Find discussions and decisions quickly
✅ **Automated** - Integrates with CI/CD and workflows
✅ **Less friction** - No manual doc updates needed
✅ **Session handoff** - New AI session sees current state immediately

---

## Recent Consolidations (October 2025)

**Documentation Restructuring:**
- ✅ Combined `installation-pi4.md` + `installation-pi5.md` → `installation.md` (single guide with platform sections)
- ✅ Merged `BOOT_CONFIGURATION.md` + `TROUBLESHOOTING.md` → `SETUP_AND_TROUBLESHOOTING.md` (operational guide)
- ✅ Deleted redundant `docs/README.md` (CLAUDE.md is authoritative source)
- ✅ Removed `.claude/implementation-notes/` (incomplete CLI feature, not shipped)
- ✅ Organized utilities → `utilities/config/`, `utilities/scripts/`, `utilities/examples/`

**Result:** 70+ scattered docs → 7 focused documentation files + GitHub as system of record

**GitHub Migration Implications:**
- When creating issues, link to current consolidated docs
- SETUP_AND_TROUBLESHOOTING.md is now the single source for troubleshooting (not TROUBLESHOOTING.md)
- All setup/boot issues reference SETUP_AND_TROUBLESHOOTING.md
- Installation issues reference docs/getting-started/installation.md (not platform-specific files)

---

## Implementation Checklist

- [ ] Create GitHub Project board with Kanban columns
- [ ] Set up labels (type, platform, priority, status)
- [ ] Create issue and PR templates
- [ ] Create milestones (v1.0, v1.1, etc.)
- [ ] Create GitHub Discussions categories
  - [ ] Architecture & Design
  - [ ] Lessons Learned
  - [ ] Feature Ideas
- [ ] Migrate known issues to GitHub Issues
- [ ] Migrate design decisions to Discussions
- [ ] Update `.claude/settings.local.json` to allow GitHub creation
- [ ] Archive this guide in `/archive/docs-old/` once setup complete

---

## Current Essential Documentation

When migrating to GitHub and creating issues/discussions, reference these files:

**Setup & Troubleshooting:**
- `docs/getting-started/installation.md` - Installation for Pi 4 & 5
- `docs/getting-started/first-capture.md` - Quick start guide
- `docs/reference/SETUP_AND_TROUBLESHOOTING.md` - Configuration & troubleshooting

**Technical Reference:**
- `docs/reference/DRIVER_ANALYSIS.md` - Driver architecture & internals
- `docs/reference/DEV_QUICK_REFERENCE.md` - Command cheat sheet
- `docs/reference/RS300_Media_Pipeline_Guide.md` - Pipeline architecture
- `docs/reference/SECURITY_AUDIT.md` - Security analysis

**Project Metadata:**
- `CLAUDE.md` - Project guide (authority for everything)
- `.claude/SESSION_STATE.md` - Current session status (will link to GitHub)
- `.claude/lessons-learned/` - Design decisions & solutions

---

## Reference

- GitHub Projects: https://docs.github.com/en/issues/planning-and-tracking-with-projects
- GitHub Issues: https://docs.github.com/en/issues
- GitHub Discussions: https://docs.github.com/en/discussions
- Conventional Commits: https://www.conventionalcommits.org/
