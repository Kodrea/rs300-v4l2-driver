#!/usr/bin/env python3
"""
CSV to Hierarchical JSON Transform
Converts I2C_Instructions.csv to hierarchical JSON structure
"""

import pandas as pd
import json

def transform_csv_to_hierarchical_json(input_csv, output_json):
    """Transform I2C instructions CSV to hierarchical JSON format"""

    # 1. READ
    print(f"Reading CSV: {input_csv}")
    df = pd.read_csv(input_csv, encoding='latin-1')
    print(f"Loaded {len(df)} rows with {len(df.columns)} columns")

    # 2. METADATA
    metadata = {
        "device": "RS300 Thermal Camera",
        "protocol": "I2C",
        "i2c_address": {
            "7bit": "0x3c",
            "write_address": "0x78",
            "read_address": "0x79"
        },
        "buffer_address": "0x1d00",
        "status_address": "0x0200",
        "total_commands": len(df),
        "total_categories": df['command category'].nunique()
    }

    print(f"Total commands: {metadata['total_commands']}")
    print(f"Total categories: {metadata['total_categories']}")
    print(f"Categories: {df['command category'].unique().tolist()}")

    # 3. GROUP BY CATEGORY
    categories = []
    for cat_name in df['command category'].unique():
        cat_df = df[df['command category'] == cat_name]
        commands = []

        for idx, row in cat_df.iterrows():
            commands.append({
                "name": row['command name'],
                "setting": row['command setting'] if pd.notna(row['command setting']) else None,
                "remarks": row['command remarks'] if pd.notna(row['command remarks']) else None,
                "command_class": row['Command Class'],
                "command_index": row['Module Command Index'],
                "subcmd": row['SubCmd'],
                "parameters": {
                    "para1": [
                        row['Para1[0]'],
                        row['Para1[1]'],
                        row['Para1[2]'],
                        row['Para1[3]']
                    ],
                    "para2": [
                        row['Para2[0]'],
                        row['Para2[1]'],
                        row['Para2[2]'],
                        row['Para2[3]']
                    ]
                },
                "response_length": [row['Len[0]'], row['Len[1]']],
                "crc16": [
                    row['CRC check result of instruction data part'],
                    row['CRC check result of instruction data part.1']
                ],
                "hex_command": row['Instruction data part (merged)']
            })

        categories.append({
            "name": cat_name,
            "command_count": len(commands),
            "commands": commands
        })

        print(f"Category '{cat_name}': {len(commands)} commands")

    # 4. ASSEMBLE
    output = {
        "metadata": metadata,
        "categories": categories
    }

    # 5. VALIDATION
    print("\n=== VALIDATION ===")
    total_cmd_count = sum(c['command_count'] for c in output['categories'])
    assert total_cmd_count == output['metadata']['total_commands'], \
        f"Command count mismatch: {total_cmd_count} != {output['metadata']['total_commands']}"
    print(f"✓ Total command count matches: {total_cmd_count}")

    assert len(output['categories']) == output['metadata']['total_categories'], \
        f"Category count mismatch: {len(output['categories'])} != {output['metadata']['total_categories']}"
    print(f"✓ Category count matches: {len(output['categories'])}")

    assert all(len(cmd['parameters']['para1']) == 4 for cat in output['categories'] for cmd in cat['commands']), \
        "Not all para1 arrays have 4 elements"
    print("✓ All para1 arrays have 4 elements")

    assert all(len(cmd['parameters']['para2']) == 4 for cat in output['categories'] for cmd in cat['commands']), \
        "Not all para2 arrays have 4 elements"
    print("✓ All para2 arrays have 4 elements")

    # 6. WRITE
    print(f"\n=== WRITING OUTPUT ===")
    with open(output_json, 'w', encoding='utf-8') as f:
        json.dump(output, f, indent=2, ensure_ascii=False)

    file_size = len(json.dumps(output, indent=2, ensure_ascii=False))
    print(f"✓ Written to: {output_json}")
    print(f"✓ File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")

    return output

if __name__ == "__main__":
    input_csv = "/home/cody/rs300-extra-documentation/I2C_Instructions.csv"
    output_json = "/home/cody/rs300-v4l2-driver/I2C_Instructions_hierarchical.json"

    print("=== CSV to Hierarchical JSON Transform ===\n")
    result = transform_csv_to_hierarchical_json(input_csv, output_json)
    print("\n=== TRANSFORMATION COMPLETE ===")
