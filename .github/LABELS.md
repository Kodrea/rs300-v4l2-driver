# GitHub Labels Configuration

These labels should be created in the repository for organizing issues.

## Type Labels
- **type: bug** (color: #d73a49) - Bug or defect
- **type: enhancement** (color: #a2eeef) - New feature or improvement
- **type: docs** (color: #0075ca) - Documentation
- **type: refactor** (color: #fbca04) - Code refactoring
- **type: test** (color: #cccccc) - Testing or validation
- **type: chore** (color: #cfd3d7) - Maintenance tasks
- **type: task** (color: #7057ff) - Task or to-do item

## Platform Labels
- **platform: pi5** (color: #1f6feb) - Raspberry Pi 5 specific
- **platform: pi4** (color: #1f6feb) - Raspberry Pi 4 specific

## Priority Labels
- **priority: critical** (color: #ff0000) - Must fix immediately
- **priority: high** (color: #ff9800) - Important, address soon
- **priority: medium** (color: #ffeb3b) - Normal priority
- **priority: low** (color: #4caf50) - Nice to have

## Status Labels
- **status: blocked** (color: #ff6b6b) - Blocked on something
- **status: help-wanted** (color: #128a0c) - Help needed
- **status: question** (color: #d876e3) - Need clarification
- **status: duplicate** (color: #e4e669) - Duplicate of another issue
- **status: wontfix** (color: #ffffff) - Won't be fixed

## Special Labels
- **security** (color: #ef5350) - Security issue
- **performance** (color: #f06292) - Performance optimization
- **known-issue** (color: #ffc107) - Known limitation or issue

## How to Create

1. Go to repository → **Settings** → **Labels**
2. Click **New label**
3. Enter name, description, and color from above
4. Click **Create label**

Alternatively, use GitHub CLI:
```bash
gh label create "type: bug" --description "Bug or defect" --color "d73a49"
```

## Using Labels

- Issues should have **exactly one** `type:` label
- Add `platform:` label if specific to Pi 4 or Pi 5
- Add `priority:` label if work-related (default: medium)
- Add `status:` label if blocked or needs attention
- Use `security` and `performance` sparingly
