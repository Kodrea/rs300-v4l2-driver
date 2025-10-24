# Sprint Prompts

This directory contains ready-to-use prompts for starting specific development sprints after clearing Claude Code context.

## Purpose

When starting a new Claude Code session (or after clearing context), you can use these prompts to quickly resume work on a specific sprint/batch without needing to re-explain the entire project context.

## Usage

### Option 1: Quick Start (One-Liner)
```bash
cat docs/prompts/SPRINT1_BATCH1_PROMPT.txt
```
Copy and paste the content into a new Claude Code session.

### Option 2: Detailed Instructions
```bash
cat docs/prompts/start-sprint1-batch1.md
```
Send this as your initial prompt for full context and checklist.

## Available Prompts

### Sprint 1

- **SPRINT1_BATCH1_PROMPT.txt** - Quick one-liner prompt for Autoshutter implementation
- **start-sprint1-batch1.md** - Detailed prompt with checklist for Autoshutter (Days 1-2)

*(More sprint prompts will be added as work progresses)*

## How These Work

Each prompt:
1. References the existing documentation structure
2. Points to specific sections in I2C_COMMANDS_TO_IMPLEMENT.md, MODERNIZATION_PLAN.md, etc.
3. Provides a clear task description
4. Includes success criteria
5. Leverages the comprehensive documentation already in the repository

## Creating New Sprint Prompts

When completing a sprint/batch, consider creating a prompt for the next one:

```markdown
# Sprint X, Batch Y: [Feature Name]

[Brief description]

## Documentation References
- Link to relevant sections in existing docs

## Implementation Checklist
- [ ] Step 1
- [ ] Step 2

## Success Criteria
- [ ] Criteria 1
```

## Integration with Documentation System

These prompts are designed to work with:
- **I2C_COMMANDS_TO_IMPLEMENT.md** - Complete implementation details
- **MODERNIZATION_PLAN.md** - Sprint timelines and context
- **DRIVER_ANALYSIS.md** - Code structure and patterns
- **SESSION_STATE.md** - Current project state

This approach ensures documentation is the single source of truth, and prompts just point to the right sections.
