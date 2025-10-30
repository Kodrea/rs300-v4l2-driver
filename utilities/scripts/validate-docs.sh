#!/bin/bash
#
# validate-docs.sh - Validate documentation quality and integrity
#
# Usage: ./scripts/validate-docs.sh [--verbose]
#

set -e

VERBOSE=0
if [[ "$1" == "--verbose" ]]; then
    VERBOSE=1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
ERRORS=0
WARNINGS=0
PASSED=0

log_error() {
    echo -e "${RED}✗ ERROR:${NC} $1"
    ((ERRORS++))
}

log_warning() {
    echo -e "${YELLOW}⚠ WARNING:${NC} $1"
    ((WARNINGS++))
}

log_pass() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo -e "${GREEN}✓${NC} $1"
    fi
    ((PASSED++))
}

log_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

echo "================================"
echo "   Documentation Validation"
echo "================================"
echo ""

# Test 1: Check for broken internal links
echo "[1/7] Checking internal links..."
BROKEN_LINKS=0

# Find all markdown files
MD_FILES=$(find . -name "*.md" -type f | grep -v node_modules || true)

for file in $MD_FILES; do
    # Extract markdown links: [text](path)
    links=$(grep -oP '\[.*?\]\(\K[^)]+' "$file" 2>/dev/null || true)

    for link in $links; do
        # Skip external links (http/https)
        if [[ "$link" =~ ^https?:// ]]; then
            continue
        fi

        # Skip anchors only
        if [[ "$link" =~ ^# ]]; then
            continue
        fi

        # Strip anchors for file check
        file_path="${link%%#*}"

        # Skip empty paths
        if [[ -z "$file_path" ]]; then
            continue
        fi

        # Resolve relative path
        dir=$(dirname "$file")
        target="$dir/$file_path"

        # Check if target exists
        if [[ ! -e "$target" ]]; then
            log_error "Broken link in $file: $link"
            ((BROKEN_LINKS++))
        fi
    done
done

if [[ $BROKEN_LINKS -eq 0 ]]; then
    log_pass "No broken internal links found"
else
    log_error "Found $BROKEN_LINKS broken internal links"
fi

# Test 2: Check for YAML front matter syntax (in task cards)
echo "[2/7] Checking YAML front matter..."
YAML_ERRORS=0

# Check files that should have YAML (task cards, pattern docs)
YAML_FILES=$(find docs/implementation-tasks docs/implementation-patterns -name "*.md" -type f 2>/dev/null || true)

for file in $YAML_FILES; do
    # Check if file starts with ---
    if head -n 1 "$file" | grep -q "^---$"; then
        # Find end of YAML block
        yaml_end=$(grep -n "^---$" "$file" | head -2 | tail -1 | cut -d: -f1)

        if [[ -z "$yaml_end" ]]; then
            log_warning "YAML block not closed in $file"
            ((YAML_ERRORS++))
        else
            # Extract YAML and check basic syntax (key: value)
            yaml_block=$(sed -n "2,$((yaml_end-1))p" "$file")

            # Check for basic YAML syntax errors
            if ! echo "$yaml_block" | grep -q ":"; then
                log_warning "Invalid YAML syntax in $file"
                ((YAML_ERRORS++))
            fi
        fi

        log_pass "YAML valid in $file"
    fi
done

if [[ $YAML_ERRORS -eq 0 ]]; then
    log_pass "All YAML front matter valid"
fi

# Test 3: Check code blocks have language tags
echo "[3/7] Checking code block language tags..."
CODE_WARNINGS=0

for file in $MD_FILES; do
    # Find code blocks without language tag: ```\n (not ```lang\n)
    untagged=$(grep -n '```$' "$file" 2>/dev/null || true)

    if [[ -n "$untagged" ]]; then
        count=$(echo "$untagged" | wc -l)
        log_warning "$file has $count code blocks without language tags"
        ((CODE_WARNINGS++))
    fi
done

if [[ $CODE_WARNINGS -eq 0 ]]; then
    log_pass "All code blocks have language tags"
fi

# Test 4: Check for duplicate file names (different directories)
echo "[4/7] Checking for duplicate filenames..."
DUPES=0

all_basenames=$(find . -name "*.md" -type f -exec basename {} \; | sort)
duplicate_names=$(echo "$all_basenames" | uniq -d)

if [[ -n "$duplicate_names" ]]; then
    log_warning "Duplicate filenames found:"
    echo "$duplicate_names" | while read name; do
        echo "  - $name:"
        find . -name "$name" -type f | sed 's/^/    /'
    done
    ((DUPES++))
else
    log_pass "No duplicate filenames"
fi

# Test 5: Check file count matches audit
echo "[5/7] Verifying file count..."
EXPECTED_COUNT=63  # From audit
ACTUAL_COUNT=$(find . -name "*.md" -type f | wc -l)

if [[ $ACTUAL_COUNT -lt $EXPECTED_COUNT ]]; then
    log_error "File count decreased: expected >=$EXPECTED_COUNT, found $ACTUAL_COUNT"
elif [[ $ACTUAL_COUNT -gt $((EXPECTED_COUNT + 20)) ]]; then
    log_warning "File count significantly increased: $ACTUAL_COUNT (was $EXPECTED_COUNT)"
else
    log_pass "File count reasonable: $ACTUAL_COUNT files"
fi

# Test 6: Check for TODO markers
echo "[6/7] Checking for TODO markers..."
TODO_COUNT=$(grep -r "TODO\|FIXME\|XXX" --include="*.md" docs/ 2>/dev/null | wc -l || echo "0")

if [[ $TODO_COUNT -gt 10 ]]; then
    log_warning "Found $TODO_COUNT TODO markers in docs/"
else
    log_pass "TODO markers: $TODO_COUNT (acceptable)"
fi

# Test 7: Check for very large files (>100KB)
echo "[7/7] Checking for oversized files..."
LARGE_FILES=0

while IFS= read -r file; do
    size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null || echo "0")
    if [[ $size -gt 102400 ]]; then  # 100KB
        size_kb=$((size / 1024))
        log_warning "Large file: $file (${size_kb}KB)"
        ((LARGE_FILES++))
    fi
done < <(find . -name "*.md" -type f)

if [[ $LARGE_FILES -eq 0 ]]; then
    log_pass "No oversized files"
fi

# Summary
echo ""
echo "================================"
echo "        Validation Summary"
echo "================================"
echo -e "${GREEN}Passed:${NC}   $PASSED"
echo -e "${YELLOW}Warnings:${NC} $WARNINGS"
echo -e "${RED}Errors:${NC}   $ERRORS"
echo ""

if [[ $ERRORS -gt 0 ]]; then
    echo -e "${RED}Validation FAILED${NC}"
    exit 1
elif [[ $WARNINGS -gt 5 ]]; then
    echo -e "${YELLOW}Validation PASSED with warnings${NC}"
    exit 0
else
    echo -e "${GREEN}Validation PASSED${NC}"
    exit 0
fi
