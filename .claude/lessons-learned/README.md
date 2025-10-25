# Lessons Learned Repository

This directory contains documented mistakes and lessons learned during RS300 driver development.

## Purpose

Every mistake is an opportunity to:
1. Understand what went wrong
2. Document how to avoid it
3. Improve project processes
4. Share knowledge with future developers

## Format

Each lesson is documented in a numbered file: `NNN-short-description.md`

### Structure:
- **What Happened**: Clear description of the mistake
- **Root Cause Analysis**: Why it happened (multiple levels)
- **How to Prevent**: Specific, actionable guidelines
- **Action Items**: Changes to make to prevent recurrence

## Index

- [001: Bypassing Established Installation Tooling](001-bypass-setup-script.md) - 2024-10-24 - CRITICAL
- [002: Code Consolidation Kernel Crash](002-consolidation-kernel-crash.md) - 2025-10-24 - HIGH

## How to Use

### Before Starting Any Task
1. Read relevant lessons in this directory
2. Check if your approach could lead to documented mistakes
3. Follow prevention guidelines

### After Making a Mistake
1. Document it immediately (while fresh in memory)
2. Do root cause analysis (go deeper than surface cause)
3. Create specific prevention rules
4. Update project documentation to prevent recurrence

## Philosophy

**"Those who cannot remember the past are condemned to repeat it."** - George Santayana

Mistakes are inevitable. Repeating them is not.

---

## Categories

Lessons are categorized for easy reference:

- **Tooling**: Misuse of build tools, scripts, or development utilities
- **Testing Methodology**: Gaps in testing approach or verification
- **Process**: Workflow or procedure mistakes
- **Security**: Security vulnerabilities or unsafe practices
- **Performance**: Performance degradation or optimization mistakes
- **Kernel Driver Development**: Kernel-specific issues, ABI problems, compiler interactions

---

## Severity Levels

- **CRITICAL**: Caused system crash, data loss, or required emergency recovery
- **HIGH**: Caused significant delays or incorrect results
- **MEDIUM**: Caused minor delays or confusion
- **LOW**: Minor inconvenience, easily corrected

---

## Contributing

When documenting a new lesson:
1. Use next sequential number
2. Use descriptive filename (kebab-case)
3. Include all sections: What Happened, Root Cause, How to Prevent, Action Items
4. Update this README index
5. Update related documentation to prevent recurrence
