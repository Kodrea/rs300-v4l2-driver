#!/usr/bin/env python3
"""
Generate I2C Quick Reference from hierarchical JSON
Reads I2C_Instructions_hierarchical.json and produces I2C_QUICK_REFERENCE.md
"""

import json
import sys
from pathlib import Path


def format_param_info(cmd):
    """Extract and format parameter information from command."""
    params = cmd.get('parameters', {})
    para1 = params.get('para1', [])
    para2 = params.get('para2', [])
    setting = cmd.get('setting')
    remarks = cmd.get('remarks')

    # Build parameter description
    param_parts = []

    # Check if parameters are non-zero
    para1_nonzero = any(p != '0x00' for p in para1)
    para2_nonzero = any(p != '0x00' for p in para2)

    if setting:
        param_parts.append(f"Setting: {setting}")

    if para1_nonzero:
        para1_hex = ' '.join(para1)
        param_parts.append(f"para1=[{para1_hex}]")

    if para2_nonzero:
        para2_hex = ' '.join(para2)
        param_parts.append(f"para2=[{para2_hex}]")

    if remarks:
        param_parts.append(f"({remarks})")

    if not param_parts and not para1_nonzero and not para2_nonzero:
        return "None"

    return ', '.join(param_parts) if param_parts else "None"


def generate_markdown(json_path, output_path):
    """Generate markdown quick reference from JSON."""

    # Read JSON file
    print(f"Reading {json_path}...")
    with open(json_path, 'r') as f:
        data = json.load(f)

    metadata = data.get('metadata', {})
    categories = data.get('categories', [])

    # Start building markdown
    md_lines = []

    # Header
    md_lines.append("# RS300 I2C Command Quick Reference")
    md_lines.append("")
    md_lines.append("**Auto-generated from**: `I2C_Instructions_hierarchical.json`")
    md_lines.append("")
    md_lines.append("## Overview")
    md_lines.append("")
    md_lines.append(f"- **Device**: {metadata.get('device', 'Unknown')}")
    md_lines.append(f"- **Protocol**: {metadata.get('protocol', 'Unknown')}")

    i2c_addr = metadata.get('i2c_address', {})
    md_lines.append(f"- **I2C Address**: {i2c_addr.get('7bit', 'Unknown')} "
                   f"(Write: {i2c_addr.get('write_address', 'Unknown')}, "
                   f"Read: {i2c_addr.get('read_address', 'Unknown')})")

    md_lines.append(f"- **Buffer Address**: {metadata.get('buffer_address', 'Unknown')}")
    md_lines.append(f"- **Status Address**: {metadata.get('status_address', 'Unknown')}")
    md_lines.append(f"- **Total Commands**: {metadata.get('total_commands', 0)}")
    md_lines.append(f"- **Total Categories**: {metadata.get('total_categories', 0)}")
    md_lines.append("")
    md_lines.append("---")
    md_lines.append("")
    md_lines.append("## Quick Notes")
    md_lines.append("")
    md_lines.append("- **Command Format**: All commands are 18 bytes: "
                   "`[cmd_class] [cmd_index] [subcmd] [para1×4] [para2×4] [response_len×2] [reserved×2] [CRC16×2]`")
    md_lines.append("- **CRC**: CRC-16-CCITT calculated over first 16 bytes")
    md_lines.append("- **Usage**: See `I2C_PROTOCOL.md` for detailed protocol documentation")
    md_lines.append("- **Full Details**: Refer to `I2C_Instructions_hierarchical.json` for complete command specifications")
    md_lines.append("")
    md_lines.append("---")
    md_lines.append("")

    # Table of Contents
    md_lines.append("## Table of Contents")
    md_lines.append("")
    total_commands = 0
    for i, category in enumerate(categories, 1):
        cat_name = category.get('name', 'Unknown')
        cmd_count = category.get('command_count', 0)
        total_commands += cmd_count
        # Create anchor link
        anchor = cat_name.lower().replace(' ', '-').replace('(', '').replace(')', '')
        md_lines.append(f"{i}. [{cat_name}](#{anchor}) ({cmd_count} commands)")
    md_lines.append("")
    md_lines.append("---")
    md_lines.append("")

    # Process each category
    for category in categories:
        cat_name = category.get('name', 'Unknown Category')
        cmd_count = category.get('command_count', 0)
        commands = category.get('commands', [])

        # Category header
        md_lines.append(f"## {cat_name}")
        md_lines.append("")
        md_lines.append(f"**Command Count**: {cmd_count}")
        md_lines.append("")

        # Table header
        md_lines.append("| Command Name | Hex Command | Parameters |")
        md_lines.append("|--------------|-------------|------------|")

        # Table rows
        for cmd in commands:
            name = cmd.get('name', 'Unknown')
            hex_cmd = cmd.get('hex_command', 'N/A')
            params = format_param_info(cmd)

            # Escape pipe characters in text
            name = name.replace('|', '\\|')
            params = params.replace('|', '\\|')

            md_lines.append(f"| {name} | `{hex_cmd}` | {params} |")

        md_lines.append("")
        md_lines.append("---")
        md_lines.append("")

    # Footer
    md_lines.append("## Notes")
    md_lines.append("")
    md_lines.append("- **Parameter Format**: `para1` and `para2` are 4-byte arrays shown as hex values")
    md_lines.append("- **Settings**: Some commands have ON/OFF variants distinguished by parameter values")
    md_lines.append("- **Response Length**: Indicates expected response data length (bytes 12-13 in command)")
    md_lines.append("- **Reserved Bytes**: Bytes 14-15 are reserved (usually 0x00)")
    md_lines.append("")
    md_lines.append("---")
    md_lines.append("")
    md_lines.append(f"**Generated from**: `{json_path.name}`  ")
    md_lines.append(f"**Total Commands Documented**: {total_commands}")
    md_lines.append("")

    # Write output
    output_content = '\n'.join(md_lines)
    print(f"Writing {output_path}...")
    with open(output_path, 'w') as f:
        f.write(output_content)

    # Stats
    file_size = output_path.stat().st_size
    print(f"\n✓ Generated successfully!")
    print(f"  - Categories: {len(categories)}")
    print(f"  - Commands: {total_commands}")
    print(f"  - File size: {file_size:,} bytes")
    print(f"  - Lines: {len(md_lines)}")

    return len(categories), total_commands, file_size, md_lines[:30]


def main():
    """Main entry point."""
    # File paths
    script_dir = Path(__file__).parent
    json_path = script_dir / "I2C_Instructions_hierarchical.json"
    output_path = script_dir / "I2C_QUICK_REFERENCE.md"

    # Validate input exists
    if not json_path.exists():
        print(f"Error: {json_path} not found!", file=sys.stderr)
        return 1

    try:
        categories, commands, size, preview = generate_markdown(json_path, output_path)

        print("\n" + "="*60)
        print("Preview (first 30 lines):")
        print("="*60)
        for line in preview:
            print(line)
        print("="*60)

        return 0

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
