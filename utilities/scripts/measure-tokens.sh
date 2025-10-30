#!/bin/bash
#
# measure-tokens.sh - Estimate token counts for documentation files
#
# Usage: ./scripts/measure-tokens.sh [--file FILE] [--compare]
#

# Simple token estimation: ~4 characters = 1 token (conservative)
# This is approximate - actual tokenization varies by model

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

COMPARE_MODE=0
SINGLE_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --compare)
            COMPARE_MODE=1
            shift
            ;;
        --file)
            SINGLE_FILE="$2"
            shift 2
            ;;
        *)
            echo "Usage: $0 [--file FILE] [--compare]"
            exit 1
            ;;
    esac
done

estimate_tokens() {
    local file="$1"
    local char_count=$(wc -c < "$file" 2>/dev/null || echo "0")
    # Conservative estimate: 4 chars per token
    echo $((char_count / 4))
}

if [[ -n "$SINGLE_FILE" ]]; then
    # Single file mode
    if [[ ! -f "$SINGLE_FILE" ]]; then
        echo -e "${RED}Error: File not found: $SINGLE_FILE${NC}"
        exit 1
    fi

    tokens=$(estimate_tokens "$SINGLE_FILE")
    size=$(du -h "$SINGLE_FILE" | cut -f1)
    lines=$(wc -l < "$SINGLE_FILE")

    echo "File: $SINGLE_FILE"
    echo "  Size: $size"
    echo "  Lines: $lines"
    echo "  Estimated tokens: ~$tokens"
    exit 0
fi

echo "================================"
echo "   Token Count Estimation"
echo "================================"
echo ""

# Key files for implementation tasks
echo "📋 Implementation Planning Docs:"
echo "--------------------------------"

files=(
    "I2C_COMMANDS_TO_IMPLEMENT.md"
    "MODERNIZATION_PLAN.md"
    "docs/contributing/ROADMAP.md"
)

total_planning=0
for file in "${files[@]}"; do
    if [[ -f "$file" ]]; then
        tokens=$(estimate_tokens "$file")
        total_planning=$((total_planning + tokens))
        printf "  %-40s ~%6d tokens\n" "$file" "$tokens"
    fi
done

echo "  ─────────────────────────────────────────────────"
printf "  %-40s ~%6d tokens\n" "Total Planning Docs:" "$total_planning"
echo ""

# Reference documentation
echo "📖 Reference Documentation:"
echo "--------------------------------"

ref_files=(
    "DRIVER_ANALYSIS.md"
    "I2C_PROTOCOL.md"
    "DEV_QUICK_REFERENCE.md"
    "I2C_QUICK_REFERENCE.md"
)

total_ref=0
for file in "${ref_files[@]}"; do
    if [[ -f "$file" ]]; then
        tokens=$(estimate_tokens "$file")
        total_ref=$((total_ref + tokens))
        printf "  %-40s ~%6d tokens\n" "$file" "$tokens"
    fi
done

echo "  ─────────────────────────────────────────────────"
printf "  %-40s ~%6d tokens\n" "Total Reference Docs:" "$total_ref"
echo ""

# Pattern library (if exists)
if [[ -d "docs/implementation-patterns" ]]; then
    echo "🔧 Pattern Library:"
    echo "--------------------------------"

    total_patterns=0
    pattern_count=0

    while IFS= read -r file; do
        tokens=$(estimate_tokens "$file")
        total_patterns=$((total_patterns + tokens))
        ((pattern_count++))
        printf "  %-40s ~%6d tokens\n" "$(basename "$file")" "$tokens"
    done < <(find docs/implementation-patterns -name "*.md" -type f 2>/dev/null || true)

    if [[ $pattern_count -gt 0 ]]; then
        echo "  ─────────────────────────────────────────────────"
        printf "  %-40s ~%6d tokens\n" "Total Pattern Library:" "$total_patterns"
        echo ""
    fi
fi

# Task cards (if exist)
if [[ -d "docs/implementation-tasks" ]]; then
    echo "📦 Task Cards:"
    echo "--------------------------------"

    total_tasks=0
    task_count=0
    avg_task=0

    while IFS= read -r file; do
        tokens=$(estimate_tokens "$file")
        total_tasks=$((total_tasks + tokens))
        ((task_count++))

        # Show only index and a few examples
        if [[ $(basename "$file") == "README.md" ]] || [[ $task_count -le 3 ]]; then
            printf "  %-40s ~%6d tokens\n" "$(basename "$file")" "$tokens"
        fi
    done < <(find docs/implementation-tasks -name "*.md" -type f 2>/dev/null || true)

    if [[ $task_count -gt 0 ]]; then
        avg_task=$((total_tasks / task_count))

        if [[ $task_count -gt 3 ]]; then
            echo "  ... and $((task_count - 3)) more task cards"
        fi

        echo "  ─────────────────────────────────────────────────"
        printf "  %-40s ~%6d tokens\n" "Total Task Cards:" "$total_tasks"
        printf "  %-40s ~%6d tokens\n" "Average per Card:" "$avg_task"
        echo ""
    fi
fi

# Comparison mode
if [[ $COMPARE_MODE -eq 1 ]]; then
    echo ""
    echo "================================"
    echo "   Efficiency Comparison"
    echo "================================"
    echo ""

    # Old workflow
    old_workflow=$((total_planning + 8000))  # Planning docs + code search
    echo "📊 Old Workflow (read planning docs + search code):"
    printf "   %d tokens\n" "$old_workflow"
    echo ""

    # New workflow (if available)
    if [[ -d "docs/implementation-tasks" ]] && [[ $task_count -gt 0 ]]; then
        # Index + average task card
        index_tokens=$(estimate_tokens "docs/implementation-tasks/README.md" 2>/dev/null || echo "800")
        new_workflow=$((index_tokens + avg_task))

        echo "🚀 New Workflow (read index + task card):"
        printf "   %d tokens\n" "$new_workflow"
        echo ""

        reduction=$(((old_workflow - new_workflow) * 100 / old_workflow))
        echo -e "${GREEN}Token Reduction: $reduction%${NC}"
        echo ""
    fi
fi

# Summary
echo "================================"
echo "        Summary"
echo "================================"

grand_total=$((total_planning + total_ref))
if [[ -d "docs/implementation-patterns" ]]; then
    grand_total=$((grand_total + total_patterns))
fi
if [[ -d "docs/implementation-tasks" ]]; then
    grand_total=$((grand_total + total_tasks))
fi

echo "  Implementation Planning: ~$total_planning tokens"
echo "  Reference Documentation: ~$total_ref tokens"
if [[ ${total_patterns:-0} -gt 0 ]]; then
    echo "  Pattern Library:         ~$total_patterns tokens"
fi
if [[ ${total_tasks:-0} -gt 0 ]]; then
    echo "  Task Cards:              ~$total_tasks tokens"
fi
echo "  ─────────────────────────────────────────────────"
echo "  Grand Total:             ~$grand_total tokens"
echo ""

# Estimate cost (very rough)
cost_per_1m_tokens=3.00  # Typical for Claude Sonnet
cost_estimate=$(echo "scale=4; $grand_total * $cost_per_1m_tokens / 1000000" | bc)
echo "  Estimated API cost for full read: \$$cost_estimate"
echo ""

exit 0
