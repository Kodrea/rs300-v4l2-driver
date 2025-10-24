# CSV→Hierarchical JSON Transform

## INPUT
- File: `I2C_Instructions.csv`
- Encoding: `latin-1`
- Rows: 224
- Columns: 24
- Key: `command category` (7 unique values)

## OUTPUT SCHEMA
```json
{
  "metadata": {
    "device": "RS300 Thermal Camera",
    "protocol": "I2C",
    "i2c_address": {"7bit": "0x3c", "write_address": "0x78", "read_address": "0x79"},
    "buffer_address": "0x1d00",
    "status_address": "0x0200",
    "total_commands": INT,
    "total_categories": INT
  },
  "categories": [
    {
      "name": STR,
      "command_count": INT,
      "commands": [
        {
          "name": STR,
          "setting": STR|NULL,
          "remarks": STR|NULL,
          "command_class": STR,
          "command_index": STR,
          "subcmd": STR,
          "parameters": {
            "para1": [STR, STR, STR, STR],
            "para2": [STR, STR, STR, STR]
          },
          "response_length": [STR, STR],
          "crc16": [STR, STR],
          "hex_command": STR
        }
      ]
    }
  ]
}
```

## FIELD MAPPING
```
CSV Column                              → JSON Field
─────────────────────────────────────────────────────────
command category                        → category.name
command name                            → name
command setting                         → setting
command remarks                         → remarks
Command Class                           → command_class
Module Command Index                    → command_index
SubCmd                                  → subcmd
Para1[0], Para1[1], Para1[2], Para1[3] → parameters.para1[]
Para2[0], Para2[1], Para2[2], Para2[3] → parameters.para2[]
Len[0], Len[1]                          → response_length[]
CRC check result... (2 columns)         → crc16[]
Instruction data part (merged)          → hex_command
```

## TRANSFORM LOGIC

### 1. READ
```python
df = pd.read_csv('I2C_Instructions.csv', encoding='latin-1')
```

### 2. METADATA
```python
metadata = {
    "device": "RS300 Thermal Camera",
    "protocol": "I2C",
    "i2c_address": {"7bit": "0x3c", "write_address": "0x78", "read_address": "0x79"},
    "buffer_address": "0x1d00",
    "status_address": "0x0200",
    "total_commands": len(df),
    "total_categories": df['command category'].nunique()
}
```

### 3. GROUP BY CATEGORY
```python
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
                "para1": [row['Para1[0]'], row['Para1[1]'], row['Para1[2]'], row['Para1[3]']],
                "para2": [row['Para2[0]'], row['Para2[1]'], row['Para2[2]'], row['Para2[3]']]
            },
            "response_length": [row['Len[0]'], row['Len[1]']],
            "crc16": [row['CRC check result of instruction data part'],
                      row['CRC check result of instruction data part.1']],
            "hex_command": row['Instruction data part (merged)']
        })

    categories.append({
        "name": cat_name,
        "command_count": len(commands),
        "commands": commands
    })
```

### 4. ASSEMBLE
```python
output = {"metadata": metadata, "categories": categories}
```

### 5. WRITE
```python
with open('I2C_Instructions_hierarchical.json', 'w', encoding='utf-8') as f:
    json.dump(output, f, indent=2, ensure_ascii=False)
```

## VALIDATION
```python
assert sum(c['command_count'] for c in output['categories']) == output['metadata']['total_commands']
assert len(output['categories']) == output['metadata']['total_categories']
assert all(len(cmd['parameters']['para1']) == 4 for cat in output['categories'] for cmd in cat['commands'])
assert all(len(cmd['parameters']['para2']) == 4 for cat in output['categories'] for cmd in cat['commands'])
```

## OPTIMIZATION RATIONALE
- **3-level hierarchy**: Root→Categories→Commands = O(1) category lookup vs O(n) flat scan
- **Array consolidation**: 8 param fields → 2 arrays = -6 fields per command = -1344 fields total
- **Category metadata**: `command_count` = no iteration for stats
- **Null handling**: `if pd.notna()` = consistent structure, predictable parsing
- **Preserved hex**: Redundant but direct-use convenience

## QUERY EFFICIENCY
| Query | Flat JSON | Hierarchical | Speedup |
|-------|-----------|--------------|---------|
| List categories | O(n)=224 | O(c)=7 | 32x |
| Commands in category | O(n) scan | O(k) direct | n/k avg |
| Category count | Full scan | Metadata read | ∞ |
| Command by name | O(n) | O(k) within cat | ~n/c avg |

## OUTPUT FILE
`I2C_Instructions_hierarchical.json` (estimated 180-200KB)
