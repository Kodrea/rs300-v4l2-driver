# RS300 I2C Command Quick Reference

**Auto-generated from**: `I2C_Instructions_hierarchical.json`

## Overview

- **Device**: RS300 Thermal Camera
- **Protocol**: I2C
- **I2C Address**: 0x3c (Write: 0x78, Read: 0x79)
- **Buffer Address**: 0x1d00
- **Status Address**: 0x0200
- **Total Commands**: 224
- **Total Categories**: 7

---

## Quick Notes

- **Command Format**: All commands are 18 bytes: `[cmd_class] [cmd_index] [subcmd] [para1×4] [para2×4] [response_len×2] [reserved×2] [CRC16×2]`
- **CRC**: CRC-16-CCITT calculated over first 16 bytes
- **Usage**: See `I2C_PROTOCOL.md` for detailed protocol documentation
- **Full Details**: Refer to `I2C_Instructions_hierarchical.json` for complete command specifications

---

## Table of Contents

1. [Shutter related](#shutter-related) (15 commands)
2. [Calibration related instructions](#calibration-related-instructions) (24 commands)
3. [System Features](#system-features) (15 commands)
4. [Get device information](#get-device-information) (10 commands)
5. [Video output related](#video-output-related) (60 commands)
6. [Image adjustment related instructions](#image-adjustment-related-instructions) (96 commands)
7. [Image Outline](#image-outline) (4 commands)

---

## Shutter related

**Command Count**: 15

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| Shutter Correction | `10 02 43 00 00 00 00 00 00 00 00 00 00 00 00 00 CF C8` | None |
| Background Correction | `01 10 52 00 00 00 00 00 00 00 00 00 00 00 00 00 24 FD` | None |
| Close the shutter | `01 0F 45 00 00 00 00 00 00 00 00 00 00 00 00 00 8D 5A` | None |
| Open shutter | `01 0F 45 00 01 00 00 00 00 00 00 00 00 00 00 00 F8 59` | para1=[0x01 0x00 0x00 0x00] |
| Auto Shutter | `10 02 41 00 00 00 00 00 00 00 00 00 00 00 00 00 0D 3E` | Setting: Settings - Off, ( Return when getting: BE AA 02 00 0000 AD FA EB AA: Indicates off BE AA 02 00 00 01 8C EA EB AA: indicates open) |
| Automatic shutter switch | `10 02 41 00 01 00 00 00 00 00 00 00 00 00 00 00 78 3D` | Setting: Setting-On, para1=[0x01 0x00 0x00 0x00], ( Return when getting: BE AA 02 00 0000 AD FA EB AA: Indicates off BE AA 02 00 00 01 8C EA EB AA: indicates open) |
| Automatic shutter switch | `10 02 81 00 00 00 00 00 00 00 00 00 01 00 00 00 78 34` | Setting: Get, ( Return when getting: BE AA 02 00 0000 AD FA EB AA: Indicates off BE AA 02 00 00 01 8C EA EB AA: indicates open) |
| Automatic shutter parameters | `10 02 42 00 00 0A 00 00 00 00 00 00 00 00 00 00 93 b1` | Setting: Setting - Temperature 10, para1=[0x00 0x0A 0x00 0x00], (Configuration values are: Para1[2], Para1[1] 0x0a 0x00 represents 10 temperature values, approximately 10/32=0.3125 degrees) |
| Automatic shutter parameters | `10 02 42 00 00 32 00 00 00 00 00 00 00 00 00 00 5A EC` | Setting: Setting - Temperature 50, para1=[0x00 0x32 0x00 0x00], (Configuration values are: Para1[2], Para1[1] 0x32 0x00 means 50 temperature values, about 50/32=1.56 degrees) |
| Automatic shutter parameters | `10 02 42 00 01 01 00 00 00 00 00 00 00 00 00 00 92 68` | Setting: Setting - minimum interval time 1s, para1=[0x01 0x01 0x00 0x00], (Configuration value: Para1[2], Para1[1] 0x01 0x00 means 1s) |
| Automatic shutter parameters | `10 02 42 00 02 78 00 00 00 00 00 00 00 00 00 00 58 ac` | Setting: Setting - Maximum interval time 120s, para1=[0x02 0x78 0x00 0x00], (Configuration value: Para1[2], Para1[1] 0x78 0x00 means 120s) |
| Automatic shutter parameters | `10 02 42 00 02 68 01 00 00 00 00 00 00 00 00 00 20 96` | Setting: Setting - Maximum interval time 360s, para1=[0x02 0x68 0x01 0x00], (Configuration value: Para1[2], Para1[1] 0x68 0x01 means 360s) |
| Automatic shutter parameters | `10 02 82 00 00 01 00 00 00 00 00 00 02 00 00 00 4E FA` | Setting: Get-Temperature, para1=[0x00 0x01 0x00 0x00], ( BE AA 03 00 005A 00 70 A8 EB AA: means 0x005A=90. Approximately 90/32=2.81 degrees) |
| Automatic shutter parameters | `10 02 82 00 01 01 00 00 00 00 00 00 02 00 00 00 3B F9` | Setting: Get-Minimum Interval Time, para1=[0x01 0x01 0x00 0x00], (BE AA 03 00 00 05 00 F1 B6 EB AA: means 0x0005=5s) |
| Automatic shutter parameters | `10 02 82 00 02 01 00 00 00 00 00 00 02 00 00 00 A4 FC` | Setting: Get-Maximum Interval Time, para1=[0x02 0x01 0x00 0x00], ( BE AA 03 00 002C 01 AE 1A EB AA: means 0x012C=300s) |

---

## Calibration related instructions

**Command Count**: 24

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| k value | `10 11 41 00 00 00 00 00 00 00 00 00 00 00 00 00 A2 93` | Setting: Calibrate k value - collect low temperature first, (Calibration step 1: Aim at a low-temperature black body, the black body temperature is 25? or below, and the picture flashes to complete) |
| k value | `10 11 41 00 01 00 00 00 00 00 00 00 00 00 00 00 D7 90` | Setting: Calibrate k value-collect high temperature again, para1=[0x01 0x00 0x00 0x00], (Calibration step 2: Aim at a high-temperature black body. The black body temperature is 60°C or above. The image flashes and the calibration is complete.) |
| k value | `10 11 41 00 02 00 00 00 00 00 00 00 00 00 00 00 48 95` | Setting: Calibrate k value-collection and calculation, para1=[0x02 0x00 0x00 0x00], (Calibration step 3: perform calculations and check the calibration results after completion) |
| k value | `10 11 43 00 00 00 00 00 00 00 00 00 00 00 00 00 60 65` | Setting: Save k value, (Calibration step 4: If the result is OK, save the result.) |
| k value | `10 11 42 00 00 00 00 00 00 00 00 00 00 00 00 00 01 1E` | Setting: Cancel calibration results, (If the effect is not OK, cancel the calibration and restore to the state before calibration.) |
| k value | `10 11 44 00 00 00 00 00 00 00 00 00 00 00 00 00 66 15` | Setting: Clear k value, (Used to test the effect of no K value calibration) |
| k value | `10 11 45 00 00 00 00 00 00 00 00 00 00 00 00 00 07 6E` | Setting: Restore factory K value data, (After restoring the factory data, save) |
| Blind Element | `10 11 51 00 00 00 00 00 00 00 00 00 00 00 00 00 55 55` | Setting: Automatic blind calibration |
| Blind Element | `10 11 57 00 01 00 00 00 00 00 00 00 00 00 00 00 47 5D` | Setting: Cursor switch setting-enable, para1=[0x01 0x00 0x00 0x00] |
| Blind Element | `10 11 57 00 00 00 00 00 00 00 00 00 00 00 00 00 32 5E` | Setting: Cursor switch setting - ing |
| Blind Element | `10 11 81 00 00 00 00 00 00 00 00 00 01 00 00 00 D7 99` | Setting: Cursor switch acquisition, ( BE AA 02 00 0001 8C EA EB AA: Indicates enabling BE AA 02 00 0000 AD FA EB AA: Indicates ing) |
| Blind Element | `10 11 58 00 2C 01 18 01 00 00 00 00 00 00 00 00 34 50` | Setting: Cursor position setting - (300, 280) - Example, para1=[0x2C 0x01 0x18 0x01], (Para1[1]+Para1[0]: x-axis coordinate 0x012C represents 300 Para1[3]+Para1[2]: y-axis coordinate 0x0118 represents 280) |
| Blind Element | `10 11 82 00 00 00 00 00 00 00 00 00 04 00 00 00 31 A8` | Setting: Get cursor position, ( BE AA 05 00 002C 01 18 01 42 83 EB AA) |
| Blind Element | `10 11 52 00 2C 01 18 01 00 00 00 00 00 00 00 00 9D 4D` | Setting: Set the cursor point to the blind pixel (manual calibration) - (300, 280) - Example, para1=[0x2C 0x01 0x18 0x01], (After setting the blind pixel, it will take effect immediately, that is, the blind pixel point disappears on the screen but will not be automatically saved. After all the blind pixels are calibrated, call the save blind pixel command.) |
| Blind Element | `10 11 52 01 2C 01 18 01 00 00 00 00 00 00 00 00 FE 08` | Setting: Set the cursor point to a non-blind element (manual calibration) - (300, 280) - Example, para1=[0x2C 0x01 0x18 0x01] |
| Blind Element | `10 11 53 00 00 00 00 00 00 00 00 00 00 00 00 00 97 A3` | Setting: Cancel this calibration data |
| Blind Element | `10 11 54 00 00 00 00 00 00 00 00 00 00 00 00 00 91 D3` | Setting: Saving blind data |
| Blind Element | `10 11 55 00 00 00 00 00 00 00 00 00 00 00 00 00 F0 A8` | Setting: Clear blind data |
| Blind Element | `10 11 56 00 00 00 00 00 00 00 00 00 00 00 00 00 53 25` | Setting: Restore factory blind data |
| Pot lid | `10 11 61 00 00 00 00 00 00 00 00 00 00 00 00 00 6D 0E` | Setting: Calibration lid, (Step 1: Aim at the uniform surface and issue this command. The screen will freeze and will be completed after thawing.) |
| Pot lid | `10 11 63 00 00 00 00 00 00 00 00 00 00 00 00 00 AF F8` | Setting: Save the lid, (Step 2: If the result is OK, save the file.) |
| Pot lid | `10 11 62 00 00 00 00 00 00 00 00 00 00 00 00 00 CE 83` | Setting: Cancel calibration results, (If the effect is not OK, cancel the calibration and restore to the state before calibration.) |
| Pot lid | `10 11 64 00 00 00 00 00 00 00 00 00 00 00 00 00 A9 88` | Setting: Empty the lid, (Used to test the effect of completely no pot lid calibration) |
| Pot lid | `10 11 65 00 00 00 00 00 00 00 00 00 00 00 00 00 C8 F3` | Setting: Restore the factory data of the pot cover, (After restoring the factory data, save) |

---

## System Features

**Command Count**: 15

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| Firmware Update | `01 01 42 00 00 00 00 00 00 00 00 00 00 00 00 00 A5 DB` | (Before updating the firmware, you need to send this command to put the module into update mode.) |
| Anti-burn protection switch | `10 03 4B 00 00 00 00 00 00 00 00 00 00 00 00 00 58 8D` | Setting: Settings - Off |
| Anti-burn protection switch | `10 03 4B 00 01 00 00 00 00 00 00 00 00 00 00 00 2D 8E` | Setting: Setting-On, para1=[0x01 0x00 0x00 0x00] |
| Anti-burn protection switch | `10 03 8B 00 00 00 00 00 00 00 00 00 01 00 00 00 2D 87` | Setting: Get, ( BE AA 02 00 0000 AD FA EB AA: Indicates off BE AA 02 00 0001 8C EA EB AA: indicates open) |
| Module sleep | `10 10 48 00 00 00 00 00 00 00 00 00 00 00 00 00 54 AD` | Setting: Settings-Wake-up |
| Module sleep | `10 10 48 00 01 00 00 00 00 00 00 00 00 00 00 00 21 AE` | Setting: Settings - Sleep, para1=[0x01 0x00 0x00 0x00], (After sleeping, the video is frozen and only responds to the wake-up command, and does not respond to other commands.) |
| Module sleep | `10 10 88 00 00 00 00 00 00 00 00 00 01 00 00 00 21 A7` | Setting: Get, ( BE AA 02 00 00 00 AD FA EB AA: Indicates working status BE AA 02 00 00 018C EA EB AA: Indicates sleep state) |
| Boot logo | `10 10 41 00 00 00 00 00 00 00 00 00 00 00 00 00 5E 3D` | Setting: Settings - Off |
| Boot logo | `10 10 41 00 01 00 00 00 00 00 00 00 00 00 00 00 2B 3E` | Setting: Setting-On, para1=[0x01 0x00 0x00 0x00] |
| Boot logo | `10 10 81 00 00 00 00 00 00 00 00 00 01 00 00 00 2B 37` | Setting: Get |
| DVP/I2C Voltage Switching | `10 10 47 00 00 00 00 00 00 00 00 00 00 00 00 00 39 36` | Setting: Setting -1.8V |
| DVP/I2C Voltage Switching | `10 10 47 00 01 00 00 00 00 00 00 00 00 00 00 00 4C 35` | Setting: Setup - 3.3V, para1=[0x01 0x00 0x00 0x00] |
| DVP/I2C Voltage Switching | `10 10 87 00 00 00 00 00 00 00 00 00 01 00 00 00 4C 3C` | Setting: Get |
| Parameter save-restore | `10 10 51 00 00 00 00 00 00 00 00 00 00 00 00 00 A9 FB` | Setting: Parameter preservation |
| Parameter save-restore | `10 10 52 00 00 00 00 00 00 00 00 00 00 00 00 00 0A 76` | Setting: Parameter recovery |

---

## Get device information

**Command Count**: 10

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| Get module information | `01 01 81 00 01 00 00 00 00 00 00 00 20 00 00 00 FC 1E` | Setting: Device Name, para1=[0x01 0x00 0x00 0x00], ( Receipt: BE AA 21 00 0043 61 6D 65 72 61 20 4D 49 4E 49 32 33 38 34 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 16 1A EB AA Device name: Camera MINI2384, the device name of mini2 series is 15 valid bytes) |
| Get module information | `01 01 81 00 02 00 00 00 00 00 00 00 0B 00 00 00 32 32` | Setting: Firmware version, para1=[0x02 0x00 0x00 0x00], ( Receipt: BE AA 0C 00 0030 30 2E 30 30 2E 30 31 2E 31 31 0A FE EB AA FW version:00.00.01.11) |
| Get module information | `01 01 81 00 04 00 00 00 00 00 00 00 02 00 00 00 7B CA` | Setting: VID, para1=[0x04 0x00 0x00 0x00], ( Receipt: BE AA 03 00 0074 34 4E FB EB AA vid: 0x3474) |
| Get module information | `01 01 81 00 05 00 00 00 00 00 00 00 02 00 00 00 0E C9` | Setting: PID, para1=[0x05 0x00 0x00 0x00], ( Receipt: BE AA 03 00 0020 00 E2 4F EB AA pid?0x0020) |
| Get module information | `01 01 81 00 06 00 00 00 00 00 00 00 20 00 00 00 B7 16` | Setting: PN, para1=[0x06 0x00 0x00 0x00], (Receipt: BE AA 21 00 00 4D 49 4E 49 32 33 38 34 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 30 68 EB AA pn: MINI2384xxxxxxxxxxxxxxxxxxxxxxxx) |
| Get module information | `01 01 81 00 07 00 00 00 00 00 00 00 20 00 00 00 C2 15` | Setting: SN, para1=[0x07 0x00 0x00 0x00], ( Receipt: BE AA 21 00 006D 69 6E 69 32 33 38 34 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 7855 EA EB AA sn: mini2384xxxxxxxxxxxxxxxxxxxxxxxx) |
| Get module temperature | `10 10 91 00 00 00 00 00 00 00 00 00 02 00 00 00 00 6A` | ( BE AA 03 00 00 0C3D 7A EB AA: means 0x0CAA=3242, i.e. 32.42 degrees Note that this temperature is for reference only and the accuracy error may be more than ±10 degrees Celsius) |
| Get the coordinates of the highest temperature point in the area of interest | `10 10 92 00 00 00 00 00 7f 01 1f 01 0e 00 00 00 e8 0c` | para2=[0x7f 0x01 0x1f 0x01], (Example: Select the range (0, 0) to (383, 287): 0x011f = 287; 0x017f = 383 1. When setting the coordinates, pay attention to the upper left (x1, y1) first and then the lower right (x2, y2). That is, x1< x2, y1<y2 2. When getting: min value, average value, max value, min x, min y, max x, max y. All are uint16 type) |
| Get the coordinates of the highest temperature point in the area of interest | `10 10 92 00 32 00 32 00 4d 01 ed 00 0e 00 00 00 34 59` | para1=[0x32 0x00 0x32 0x00], para2=[0x4d 0x01 0xed 0x00], (Example: Select the range (50, 50) to (333, 237): 0x0032=50; 0x014d=333; 0x00ed=237 Receipt: BE AA 0F 00 00D4 1F AE 6F E0 79 4C 00 D2 00 39 00 33 00 D3 F0 EB AA 0x1FD4 is the minimum value, 0x6FAE is the average value, and 0x79E0 is the maximum value. 0x004c, 0x00D2 are the minimum value coordinates, that is, (76, 210), 0x0039 and 0x0033 are the maximum value coordinates, that is, (51, 57)) |
| Get the power-on time | `10 10 93 00 00 00 00 00 00 00 00 00 04 00 00 00 5B BB` | ( BE AA 05 00 00D7 01 00 00 EB 98 EB AA: means 0x000001D7=471s) |

---

## Video output related

**Command Count**: 60

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| Digital video formats | `10 10 46 00 01 00 1E 00 00 00 00 00 00 00 00 00 4f e3` | Setting: Settings - USB - Progressive - 30hz, para1=[0x01 0x00 0x1E 0x00], (Para1[0]: 0x01, indicating digital output is enabled; Para1[1]: 0x00, indicating USB line-by-line output; Para1[2]: 0x1e, indicating 30 Hz output. The array size is consistent with the detector output. Note that the maximum frame rate of mini2 640 in USB output mode is 30 Hz.) |
| Digital video formats | `10 10 46 00 01 00 3C 00 00 00 00 00 00 00 00 00 c8 04` | Setting: Settings - USB - Progressive - 60hz, para1=[0x01 0x00 0x3C 0x00], (Para1[0]: 0x01, indicating digital output is turned on; Para1[1]: 0x00, indicating USB line-by-line output; Para1[2]: 0x3c, indicating 60 Hz output. The array size is consistent with the detector output. Note that mini2 640 will return an error if this command is set) |
| Digital video formats | `10 10 46 00 01 01 1E 00 00 00 00 00 00 00 00 00 06 3b` | Setting: settings-dvp-progressive-30hz, para1=[0x01 0x01 0x1E 0x00], (Para1[0]: 0x01, indicating digital output is turned on; Para1[1]: 0x01, indicating DVP line-by-line output; Para1[2]: 0x1e, indicating 30 Hz output; the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 01 3C 00 00 00 00 00 00 00 00 00 81 dc` | Setting: settings-dvp-progressive-60hz, para1=[0x01 0x01 0x3C 0x00], (Para1[0]: 0x01, indicating digital output is turned on; Para1[1]: 0x01, indicating DVP line-by-line output; Para1[2]: 0x3c, indicating 60 Hz output; the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 02 1E 00 00 00 00 00 00 00 00 00 fc 43` | Setting: settings -bt656 -progressive -30hz, para1=[0x01 0x02 0x1E 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x02, indicating bt656 line-by-line output, Para1[2]: 0x1e, indicating 30hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 02 3C 00 00 00 00 00 00 00 00 00 7b a4` | Setting: settings -bt656-progressive-60hz, para1=[0x01 0x02 0x3C 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x02, indicating bt656 line-by-line output, Para1[2]: 0x3c, indicating 60hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 12 00 00 00 00 00 00 00 00 00 00 a3 bb` | Setting: settings -bt656 -interlaced -25hz, para1=[0x01 0x12 0x00 0x00], (Para1[0]: 0x01, indicating digital output is enabled, Para1[1]: 0x12, indicating bt656 interlaced output, Para1[2]: 0x00, invalid, the array size is fixed720*576, the frame rate is fixed at 25hz, which is the standard BT656 interlaced timing) |
| Digital video formats | `10 10 46 00 01 03 1E 00 00 00 00 00 00 00 00 00 b5 9b` | Setting: settings-mipi-progressive-30hz, para1=[0x01 0x03 0x1E 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x03, indicating mipi line-by-line output, Para1[2]: 0x1e, indicating 30 Hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 03 3C 00 00 00 00 00 00 00 00 00 32 7c` | Setting: settings-mipi-progressive-60hz, para1=[0x01 0x03 0x3C 0x00], (Para1[0]: 0x01, indicating digital output is enabled; Para1[1]: 0x03, indicating mipi line-by-line output; Para1[2]: 0x3c, indicating 60 Hz output; the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 00 19 00 00 00 00 00 00 00 00 00 b5 ff` | Setting: Settings - USB - Progressive - 25hz (for mini2 256), para1=[0x01 0x00 0x19 0x00], ( Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x00, indicating USB line-by-line output, Para1[2]: 0x19, indicating 25 Hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 00 32 00 00 00 00 00 00 00 00 00 3c 3d` | Setting: Settings - USB - Progressive - 50hz (for mini2 256), para1=[0x01 0x00 0x32 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x00, indicating USB line-by-line output, Para1[2]: 0x32, indicating 50 Hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 01 19 00 00 00 00 00 00 00 00 00 fc 27` | Setting: Setting -dvp-progressive-25hz (for mini2 256), para1=[0x01 0x01 0x19 0x00], (Para1[0]: 0x01, indicating digital output is turned on; Para1[1]: 0x01, indicating DVP line-by-line output; Para1[2]: 0x19, indicating 25 Hz output; the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 01 32 00 00 00 00 00 00 00 00 00 75 e5` | Setting: Setting -dvp-progressive-50hz (for mini2 256), para1=[0x01 0x01 0x32 0x00], (Para1[0]: 0x01, indicating digital output is turned on; Para1[1]: 0x01, indicating DVP line-by-line output; Para1[2]: 0x32, indicating 50 Hz output; the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 02 19 00 00 00 00 00 00 00 00 00 06 5f` | Setting: Setting-bt656-progressive-25hz (for mini2 256), para1=[0x01 0x02 0x19 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x02, indicating bt656 line-by-line output, Para1[2]: 0x19, indicating 25hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 02 32 00 00 00 00 00 00 00 00 00 8f 9d` | Setting: Setting-bt656-progressive-50hz (for mini2 256), para1=[0x01 0x02 0x32 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x02, indicating bt656 line-by-line output, Para1[2]: 0x32, indicating 50hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 03 19 00 00 00 00 00 00 00 00 00 4f 87` | Setting: Setting-mipi-progressive-25hz (for mini2 256), para1=[0x01 0x03 0x19 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x03, indicating mipi line-by-line output, Para1[2]: 0x19, indicating 25 Hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 01 03 32 00 00 00 00 00 00 00 00 00 c6 45` | Setting: Setting-mipi-progressive-50hz (for mini2 256), para1=[0x01 0x03 0x32 0x00], (Para1[0]: 0x01, indicating digital output is turned on, Para1[1]: 0x03, indicating mipi line-by-line output, Para1[2]: 0x32, indicating 50 Hz output, the array size is consistent with the detector output) |
| Digital video formats | `10 10 46 00 00 00 00 00 00 00 00 00 00 00 00 00 58 4d` | Setting: Setting - Turning off digital output, (Para1[0]: 0x00, indicating that the digital output is turned off) |
| Digital video formats | `10 10 86 00 00 00 00 00 00 00 00 00 04 00 00 00 68 fb` | Setting: Get, ( BE AA 05 00 0000 00 00 00 f5 56 EB AA: 00 means digital output is off BE AA 05 00 00 01 00 1e00 3d 00 EB AA: 01 means digital output is on, 00 means USB output, 1e means the output frame rate is 30hz BE AA 05 00 00 01 00 3C 00 B9 60 EB AA: 01 means digital output is on, 00 means USB output, 3c means the output frame rate is 60hz BE AA 05 00 0001 01 1e 00 0d 37 EB AA: 01 means digital output is on, 01 means DVP output, 1e means the output frame rate is 30hz) |
| Analog video formats | `10 10 4A 00 01 00 00 00 00 00 00 00 00 00 00 00 e3 58` | Setting: Settings - Enable ntsc, para1=[0x01 0x00 0x00 0x00], (To switch from PAL to NTSC, you must first send the command to turn off simulation, and then send this command) |
| Analog video formats | `10 10 4A 00 01 01 00 00 00 00 00 00 00 00 00 00 aa 80` | Setting: Settings - Enable PAL, para1=[0x01 0x01 0x00 0x00], (To switch from NTSC to PAL, you must first send the command to turn off simulation, and then send this command) |
| Analog video formats | `10 10 4A 00 00 00 00 00 00 00 00 00 00 00 00 00 96 5b` | Setting: Settings - Disable Simulation |
| Analog video formats | `10 10 8A 00 00 00 00 00 00 00 00 00 02 00 00 00 3f ca` | Setting: Get, (BE AA 03 00 00 00 00 04 49 EB AA: Analog off BE AA 03 00 0001 00 35 7a EB AA: The first 01 means on, 00 means ntsc BE AA 03 00 0001 01 14 6a EB AA: The first 01 means open, 01 means pal) |
| Save digital-analog output format | `10 10 49 00 00 00 00 00 00 00 00 00 00 00 00 00 35 D6` | Setting: set up, (Send this command to save both the digital video output format and the analog video output format. This state cannot be reset by the "parameter restore" command after being saved, and can only be overwritten by the next save.) |
| Detector frame rate | `10 10 44 00 1E 00 00 00 00 00 00 00 00 00 00 00 5C 9C` | Setting: Setting -30hz, para1=[0x1E 0x00 0x00 0x00], (mini2 384/640 can be set, the detector output is cut to 30hz) |
| Detector frame rate | `10 10 44 00 3C 00 00 00 00 00 00 00 00 00 00 00 16 F4` | Setting: Setting -60hz, para1=[0x3C 0x00 0x00 0x00], (mini2 384/640 can be set, the detector output is cut to 60hz) |
| Detector frame rate | `10 10 44 00 19 00 00 00 00 00 00 00 00 00 00 00 17 94` | Setting: Setting -25Hz, para1=[0x19 0x00 0x00 0x00], (Mini2 256 can be set, the detector output is cut to 25hz) |
| Detector frame rate | `10 10 44 00 32 00 00 00 00 00 00 00 00 00 00 00 80 E4` | Setting: Setting -50hz, para1=[0x32 0x00 0x00 0x00], (Mini2 256 can be set, the detector output is cut to 50hz) |
| Detector frame rate | `10 10 84 00 00 00 00 00 00 00 00 00 01 00 00 00 EF B1` | Setting: Get, ( BE AA 02 00 001E 52 09 EB AA ?0x1E means 30hz BE AA 02 00 003C 72 0D EB AA: 0x3C means 60hz) |
| Image data source | `10 10 45 00 00 00 00 00 00 00 00 00 00 00 00 00 FB C0` | Setting: Setup - IR, (Detector raw data output) |
| Image data source | `10 10 45 00 01 00 00 00 00 00 00 00 00 00 00 00 8E C3` | Setting: Settings - KBC, para1=[0x01 0x00 0x00 0x00], (Data output after KB correction) |
| Image data source | `10 10 45 00 02 00 00 00 00 00 00 00 00 00 00 00 11 C6` | Setting: Setting - TNR, para1=[0x02 0x00 0x00 0x00], (Data output after removing time domain noise) |
| Image data source | `10 10 45 00 03 00 00 00 00 00 00 00 00 00 00 00 64 C5` | Setting: Setting - SNR, para1=[0x03 0x00 0x00 0x00], (Data output after spatial noise removal) |
| Image data source | `10 10 45 00 04 00 00 00 00 00 00 00 00 00 00 00 2F CD` | Setting: Setup - DDE, para1=[0x04 0x00 0x00 0x00], (Data output after image stretching and detail enhancement) |
| Image data source | `10 10 45 00 05 00 00 00 00 00 00 00 00 00 00 00 5A CE` | Setting: Settings - YUV, para1=[0x05 0x00 0x00 0x00], (Default final output) |
| Image data source | `10 10 85 00 00 00 00 00 00 00 00 00 01 00 00 00 8E CA` | Setting: Get, (BE AA 02 00 00 00 AD FA EB AA : Indicates IR output BE AA 02 00 00 01 8C EA EB AA : Indicates KBC output BE AA 02 00 00 02 EF DA EB AA : Indicates TNR output BE AA 02 00 00 03 CE CA EB AA : Indicates SNR output BE AA 02 00 00 04 29 BA EB AA : Indicates DDE output BE AA 02 00 00 05 08 AA EB AA : Indicates YUV output) |
| YUV format | `10 03 4D 00 00 00 00 00 00 00 00 00 00 00 00 00 3F 86` | Setting: Set-uyvy |
| YUV format | `10 03 4D 01 00 00 00 00 00 00 00 00 00 00 00 00 5C C3` | Setting: Setting-vyuy |
| YUV format | `10 03 4D 02 00 00 00 00 00 00 00 00 00 00 00 00 F9 0C` | Setting: Setting-yuyv |
| YUV format | `10 03 4D 03 00 00 00 00 00 00 00 00 00 00 00 00 9A 49` | Setting: Setting -yvyu |
| YUV format | `10 03 8C 00 00 00 00 00 00 00 00 00 01 00 00 00 2B F7` | Setting: Get |
| Screen freezes | `10 10 42 00 00 00 00 00 00 00 00 00 00 00 00 00 FD B0` | Setting: Set-disable, (Note: When the image is frozen, it will not respond to electronic zoom, mirror reversal, or scene switching commands.) |
| Screen freezes | `10 10 42 00 01 00 00 00 00 00 00 00 00 00 00 00 88 B3` | Setting: Set-enable, para1=[0x01 0x00 0x00 0x00] |
| Screen freezes | `10 10 82 00 00 00 00 00 00 00 00 00 01 00 00 00 88 BA` | Setting: Get, (BE AA 02 00 00 00 AD FA EB AA: indicates non-freezing state BE AA 02 00 00 01 8C EA EB AA: indicates freezing state) |
| Mirror flip | `10 10 43 00 00 00 00 00 00 00 00 00 00 00 00 00 9C CB` | Setting: Settings - No Flip, (Note: This function conflicts with the electronic zoom function, that is: The electronic zoom state cannot be flipped. The electronic zoom state cannot be flipped.) |
| Mirror flip | `10 10 43 00 01 00 00 00 00 00 00 00 00 00 00 00 E9 C8` | Setting: Settings - Flip left and right, para1=[0x01 0x00 0x00 0x00], (Note: This function conflicts with the electronic zoom function, that is: The electronic zoom state cannot be flipped. The electronic zoom state cannot be flipped.) |
| Mirror flip | `10 10 43 00 02 00 00 00 00 00 00 00 00 00 00 00 76 CD` | Setting: Settings - Flip upside down, para1=[0x02 0x00 0x00 0x00], (Note: This function conflicts with the electronic zoom function, that is: The electronic zoom state cannot be flipped. The electronic zoom state cannot be flipped.) |
| Mirror flip | `10 10 43 00 03 00 00 00 00 00 00 00 00 00 00 00 03 CE` | Setting: Settings - Left and Right + Up and Down, para1=[0x03 0x00 0x00 0x00], (Note: This function conflicts with the electronic zoom function, that is: The electronic zoom state cannot be flipped. The electronic zoom state cannot be flipped.) |
| Mirror flip | `10 10 83 00 00 00 00 00 00 00 00 00 01 00 00 00 E9 C1` | Setting: Get, (BE AA 02 00 00 00 AD FA EB AA: No flip BE AA 02 00 00 01 8C EA EB AA: Left-right flip BE AA 02 00 00 02 EF DA EB AA: Up-down flip BE AA 02 00 00 03 CE CA EB AA: Left-right + up-down flip) |
| External synchronization mode | `10 10 4B 00 01 00 00 00 00 00 00 00 00 00 00 00 82 23` | Setting: Setting-On, para1=[0x01 0x00 0x00 0x00], (Note: Only MIPI and DVP outputs support external synchronization mode. In external synchronization mode, electronic zoom and mirror flip are not supported.) |
| External synchronization mode | `10 10 4B 00 00 00 00 00 00 00 00 00 00 00 00 00 F7 20` | Setting: Settings - Off |
| External synchronization mode | `10 10 8B 00 00 00 00 00 00 00 00 00 01 00 00 00 82 2A` | Setting: Get |
| Electronic zoom-center | `01 31 42 00 00 0A 00 00 00 00 00 00 00 00 00 00 06 0A` | Setting: Setting -1x, para1=[0x00 0x0A 0x00 0x00], (Para1[1]: magnification, 0x0A=10, which means 1.0 times magnification, the highest is X8.0) |
| Electronic zoom-center | `01 31 42 00 00 14 00 00 00 00 00 00 00 00 00 00 41 0C` | Setting: Setting -2x, para1=[0x00 0x14 0x00 0x00], (Para1[1]: magnification, 0x0A=10, which means 1.0 times magnification, the highest is X8.0) |
| Electronic zoom-center | `01 31 42 00 00 1E 00 00 00 00 00 00 00 00 00 00 7C 0E` | Setting: Setting - 3x, para1=[0x00 0x1E 0x00 0x00], (Para1[1]: magnification, 0x0A=10, which means 1.0 times magnification, the highest is X8.0) |
| Electronic zoom-center | `01 31 42 00 00 28 00 00 00 00 00 00 00 00 00 00 CF 00` | Setting: Setting - 4x, para1=[0x00 0x28 0x00 0x00], (Para1[1]: magnification, 0x0A=10, which means 1.0 times magnification, the highest is X8.0) |
| Electronic zoom-center | `01 31 42 00 00 50 00 00 00 00 00 00 00 00 00 00 D3 19` | Setting: Setting -8x, para1=[0x00 0x50 0x00 0x00], (Para1[1]: magnification, 0x0A=10, which means 1.0 times magnification, the highest is X8.0) |
| Electronic zoom-center | `01 31 82 00 00 00 00 00 00 00 00 00 01 00 00 00 4E 02` | Setting: Get, ( BE AA 02 00 00 5058 A0 EB AA: means 0x50=80, which is 8.0 times. Note that this command is only used to obtain the center magnification information and has nothing to do with the coordinate magnification.) |
| Electronic zoom-coordinates | `01 31 51 00 00 15 00 00 2C 01 64 00 00 00 00 00 5B 14` | Setting: Setting -(300,100), 2.1 times, para1=[0x00 0x15 0x00 0x00], para2=[0x2C 0x01 0x64 0x00], (Zoom with the specified point as the center Para2[1]+Para2[0]: x-axis coordinate Para2[3]+Para2[2]: y-axis coordinate Para1[1]: zoom ratio) |
| Electronic zoom-coordinates | `01 31 91 00 00 00 00 00 00 00 00 00 05 00 00 00 EB 83` | Setting: Get, ( BE AA 06 0000 2C 01 64 00 1552 F7 EB AA: Indicates that the center coordinates of the zoom are (0x012C, 0x0064), i.e. (300, 100), and the magnification is 0x15=21, i.e. 2.1 times. Note that this command is only used to obtain coordinate zoom information and has nothing to do with center zoom.) |

---

## Image adjustment related instructions

**Command Count**: 96

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| Scene Mode | `10 04 42 00 00 00 00 00 00 00 00 00 00 00 00 00 C5 65` | Setting: Setting - Low Temperature Protrusion |
| Scene Mode | `10 04 42 00 01 00 00 00 00 00 00 00 00 00 00 00 B0 66` | Setting: Settings - Linear Stretch, para1=[0x01 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 02 00 00 00 00 00 00 00 00 00 00 00 2F 63` | Setting: Settings - Low Contrast, para1=[0x02 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 03 00 00 00 00 00 00 00 00 00 00 00 5A 60` | Setting: Settings - General Mode (Default), para1=[0x03 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 04 00 00 00 00 00 00 00 00 00 00 00 11 68` | Setting: Settings - High Contrast, para1=[0x04 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 05 00 00 00 00 00 00 00 00 00 00 00 64 6B` | Setting: Settings - Highlight, para1=[0x05 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 06 00 00 00 00 00 00 00 00 00 00 00 FB 6E` | Setting: Setting-Reserve 1, para1=[0x06 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 07 00 00 00 00 00 00 00 00 00 00 00 8E 6D` | Setting: Settings-Reserve 2, para1=[0x07 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 08 00 00 00 00 00 00 00 00 00 00 00 6D 7E` | Setting: Settings-Reserve 3, para1=[0x08 0x00 0x00 0x00] |
| Scene Mode | `10 04 42 00 09 00 00 00 00 00 00 00 00 00 00 00 18 7D` | Setting: Settings - Outline Mode, para1=[0x09 0x00 0x00 0x00] |
| Scene Mode | `10 04 89 00 01 00 00 00 00 00 00 00 01 00 00 00 0D 0A` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |
| False Color | `10 03 45 00 00 00 00 00 00 00 00 00 00 00 00 00 54 6D` | Setting: Settings - White Hot |
| False Color | `10 03 45 00 00 01 00 00 00 00 00 00 00 00 00 00 1D B5` | Setting: Set-reserved, para1=[0x00 0x01 0x00 0x00] |
| False Color | `10 03 45 00 00 02 00 00 00 00 00 00 00 00 00 00 E7 CD` | Setting: Settings-Gold Sepia, para1=[0x00 0x02 0x00 0x00] |
| False Color | `10 03 45 00 00 03 00 00 00 00 00 00 00 00 00 00 AE 15` | Setting: Settings - Ironbow, para1=[0x00 0x03 0x00 0x00] |
| False Color | `10 03 45 00 00 04 00 00 00 00 00 00 00 00 00 00 13 3C` | Setting: Setting-Rainbow, para1=[0x00 0x04 0x00 0x00] |
| False Color | `10 03 45 00 00 05 00 00 00 00 00 00 00 00 00 00 5A E4` | Setting: Settings-Night, para1=[0x00 0x05 0x00 0x00] |
| False Color | `10 03 45 00 00 06 00 00 00 00 00 00 00 00 00 00 A0 9C` | Setting: Settings - Aurora, para1=[0x00 0x06 0x00 0x00] |
| False Color | `10 03 45 00 00 07 00 00 00 00 00 00 00 00 00 00 E9 44` | Setting: Settings - Red_Hot, para1=[0x00 0x07 0x00 0x00] |
| False Color | `10 03 45 00 00 08 00 00 00 00 00 00 00 00 00 00 DA CF` | Setting: Setting-Jungle, para1=[0x00 0x08 0x00 0x00] |
| False Color | `10 03 45 00 00 09 00 00 00 00 00 00 00 00 00 00 93 17` | Setting: Settings-Medical, para1=[0x00 0x09 0x00 0x00] |
| False Color | `10 03 45 00 00 0A 00 00 00 00 00 00 00 00 00 00 69 6F` | Setting: Settings - Black_Hot, para1=[0x00 0x0A 0x00 0x00] |
| False Color | `10 03 45 00 00 0B 00 00 00 00 00 00 00 00 00 00 20 B7` | Setting: Setting-Golden Red Glory_Hot, para1=[0x00 0x0B 0x00 0x00] |
| False Color | `10 03 85 00 00 00 00 00 00 00 00 00 01 00 00 00 21 67` | Setting: Get |
| Detail Enhancement | `10 04 45 00 00 00 00 00 00 00 00 00 00 00 00 00 C3 15` | Setting: Setting - 0 |
| Detail Enhancement | `10 04 45 00 0A 00 00 00 00 00 00 00 00 00 00 00 81 08` | Setting: Settings - 10 levels, para1=[0x0A 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 14 00 00 00 00 00 00 00 00 00 00 00 47 2F` | Setting: Settings - 20 levels, para1=[0x14 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 1E 00 00 00 00 00 00 00 00 00 00 00 05 32` | Setting: Setting - 30 levels, para1=[0x1E 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 28 00 00 00 00 00 00 00 00 00 00 00 CB 60` | Setting: Settings - 40 levels, para1=[0x28 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 32 00 00 00 00 00 00 00 00 00 00 00 D9 4A` | Setting: Setting - 50 levels, para1=[0x32 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 3C 00 00 00 00 00 00 00 00 00 00 00 4F 5A` | Setting: Setting - 60 levels, para1=[0x3C 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 46 00 00 00 00 00 00 00 00 00 00 00 BD C3` | Setting: Setting - 70 levels, para1=[0x46 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 50 00 00 00 00 00 00 00 00 00 00 00 D3 FF` | Setting: Setting - 80, para1=[0x50 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 5A 00 00 00 00 00 00 00 00 00 00 00 91 E2` | Setting: Setting - 90 gears, para1=[0x5A 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 45 00 64 00 00 00 00 00 00 00 00 00 00 00 F7 AB` | Setting: Setting -100, para1=[0x64 0x00 0x00 0x00] |
| Detail Enhancement | `10 04 85 00 01 00 00 00 00 00 00 00 01 00 00 00 C3 1C` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |
| brightness | `10 04 47 00 00 00 00 00 00 00 00 00 00 00 00 00 01 E3` | Setting: Setting - 0 |
| brightness | `10 04 47 00 0A 00 00 00 00 00 00 00 00 00 00 00 43 FE` | Setting: Settings - 10 levels, para1=[0x0A 0x00 0x00 0x00] |
| brightness | `10 04 47 00 14 00 00 00 00 00 00 00 00 00 00 00 85 D9` | Setting: Settings - 20 levels, para1=[0x14 0x00 0x00 0x00] |
| brightness | `10 04 47 00 1E 00 00 00 00 00 00 00 00 00 00 00 C7 C4` | Setting: Setting - 30 levels, para1=[0x1E 0x00 0x00 0x00] |
| brightness | `10 04 47 00 28 00 00 00 00 00 00 00 00 00 00 00 09 96` | Setting: Settings - 40 levels, para1=[0x28 0x00 0x00 0x00] |
| brightness | `10 04 47 00 32 00 00 00 00 00 00 00 00 00 00 00 1B BC` | Setting: Setting - 50 levels, para1=[0x32 0x00 0x00 0x00] |
| brightness | `10 04 47 00 3C 00 00 00 00 00 00 00 00 00 00 00 8D AC` | Setting: Setting - 60 levels, para1=[0x3C 0x00 0x00 0x00] |
| brightness | `10 04 47 00 46 00 00 00 00 00 00 00 00 00 00 00 7F 35` | Setting: Setting - 70 levels, para1=[0x46 0x00 0x00 0x00] |
| brightness | `10 04 47 00 50 00 00 00 00 00 00 00 00 00 00 00 11 09` | Setting: Setting - 80, para1=[0x50 0x00 0x00 0x00] |
| brightness | `10 04 47 00 5A 00 00 00 00 00 00 00 00 00 00 00 53 14` | Setting: Setting - 90 gears, para1=[0x5A 0x00 0x00 0x00] |
| brightness | `10 04 47 00 64 00 00 00 00 00 00 00 00 00 00 00 35 5D` | Setting: Setting -100, para1=[0x64 0x00 0x00 0x00] |
| brightness | `10 04 87 00 01 00 00 00 00 00 00 00 01 00 00 00 01 EA` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 00 00 00 00 00 00 00 00 00 00 00 00 AE 8E` | Setting: Setting - 0 |
| Contrast | `10 04 4A 00 0A 00 00 00 00 00 00 00 00 00 00 00 EC 93` | Setting: Settings - 10 levels, para1=[0x0A 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 14 00 00 00 00 00 00 00 00 00 00 00 2A B4` | Setting: Settings - 20 levels, para1=[0x14 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 1E 00 00 00 00 00 00 00 00 00 00 00 68 A9` | Setting: Setting - 30 levels, para1=[0x1E 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 28 00 00 00 00 00 00 00 00 00 00 00 A6 FB` | Setting: Settings - 40 levels, para1=[0x28 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 32 00 00 00 00 00 00 00 00 00 00 00 B4 D1` | Setting: Setting - 50 levels, para1=[0x32 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 3C 00 00 00 00 00 00 00 00 00 00 00 22 C1` | Setting: Setting - 60 levels, para1=[0x3C 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 46 00 00 00 00 00 00 00 00 00 00 00 D0 58` | Setting: Setting - 70 levels, para1=[0x46 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 50 00 00 00 00 00 00 00 00 00 00 00 BE 64` | Setting: Setting - 80, para1=[0x50 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 5A 00 00 00 00 00 00 00 00 00 00 00 FC 79` | Setting: Setting - 90 gears, para1=[0x5A 0x00 0x00 0x00] |
| Contrast | `10 04 4A 00 64 00 00 00 00 00 00 00 00 00 00 00 9A 30` | Setting: Setting -100, para1=[0x64 0x00 0x00 0x00] |
| Contrast | `10 04 8A 00 01 00 00 00 00 00 00 00 01 00 00 00 AE 87` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 00 00 00 00 00 00 00 00 00 00 00 00 CF F5` | Setting: Setting - 0 |
| Airspace Noise Reduction | `10 04 4B 00 0A 00 00 00 00 00 00 00 00 00 00 00 8D E8` | Setting: Settings - 10 levels, para1=[0x0A 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 14 00 00 00 00 00 00 00 00 00 00 00 4B CF` | Setting: Settings - 20 levels, para1=[0x14 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 1E 00 00 00 00 00 00 00 00 00 00 00 09 D2` | Setting: Setting - 30 levels, para1=[0x1E 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 28 00 00 00 00 00 00 00 00 00 00 00 C7 80` | Setting: Settings - 40 levels, para1=[0x28 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 32 00 00 00 00 00 00 00 00 00 00 00 D5 AA` | Setting: Setting - 50 levels, para1=[0x32 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 3C 00 00 00 00 00 00 00 00 00 00 00 43 BA` | Setting: Setting - 60 levels, para1=[0x3C 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 46 00 00 00 00 00 00 00 00 00 00 00 B1 23` | Setting: Setting - 70 levels, para1=[0x46 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 50 00 00 00 00 00 00 00 00 00 00 00 DF 1F` | Setting: Setting - 80, para1=[0x50 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 5A 00 00 00 00 00 00 00 00 00 00 00 9D 02` | Setting: Setting - 90 gears, para1=[0x5A 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 4B 00 64 00 00 00 00 00 00 00 00 00 00 00 FB 4B` | Setting: Setting -100, para1=[0x64 0x00 0x00 0x00] |
| Airspace Noise Reduction | `10 04 8B 00 01 00 00 00 00 00 00 00 01 00 00 00 CF FC` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 00 00 00 00 00 00 00 00 00 00 00 00 C9 85` | Setting: Setting - 0 |
| Temporal noise reduction | `10 04 4C 00 0A 00 00 00 00 00 00 00 00 00 00 00 8B 98` | Setting: Settings - 10 levels, para1=[0x0A 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 14 00 00 00 00 00 00 00 00 00 00 00 4D BF` | Setting: Settings - 20 levels, para1=[0x14 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 1E 00 00 00 00 00 00 00 00 00 00 00 0F A2` | Setting: Setting - 30 levels, para1=[0x1E 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 28 00 00 00 00 00 00 00 00 00 00 00 C1 F0` | Setting: Settings - 40 levels, para1=[0x28 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 32 00 00 00 00 00 00 00 00 00 00 00 D3 DA` | Setting: Setting - 50 levels, para1=[0x32 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 3C 00 00 00 00 00 00 00 00 00 00 00 45 CA` | Setting: Setting - 60 levels, para1=[0x3C 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 46 00 00 00 00 00 00 00 00 00 00 00 B7 53` | Setting: Setting - 70 levels, para1=[0x46 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 50 00 00 00 00 00 00 00 00 00 00 00 D9 6F` | Setting: Setting - 80, para1=[0x50 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 5A 00 00 00 00 00 00 00 00 00 00 00 9B 72` | Setting: Setting - 90 gears, para1=[0x5A 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 4C 00 64 00 00 00 00 00 00 00 00 00 00 00 FD 3B` | Setting: Setting -100, para1=[0x64 0x00 0x00 0x00] |
| Temporal noise reduction | `10 04 8C 00 01 00 00 00 00 00 00 00 01 00 00 00 C9 8C` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 00 00 00 00 00 00 00 00 00 00 00 00 A8 FE` | Setting: Setting - 0 |
| Gamma intensity | `10 04 4D 00 0A 00 00 00 00 00 00 00 00 00 00 00 EA E3` | Setting: Settings - 10 levels, para1=[0x0A 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 14 00 00 00 00 00 00 00 00 00 00 00 2C C4` | Setting: Settings - 20 levels, para1=[0x14 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 1E 00 00 00 00 00 00 00 00 00 00 00 6E D9` | Setting: Setting - 30 levels, para1=[0x1E 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 28 00 00 00 00 00 00 00 00 00 00 00 A0 8B` | Setting: Settings - 40 levels, para1=[0x28 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 32 00 00 00 00 00 00 00 00 00 00 00 B2 A1` | Setting: Setting - 50 levels, para1=[0x32 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 3C 00 00 00 00 00 00 00 00 00 00 00 24 B1` | Setting: Setting - 60 levels, para1=[0x3C 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 46 00 00 00 00 00 00 00 00 00 00 00 D6 28` | Setting: Setting - 70 levels, para1=[0x46 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 50 00 00 00 00 00 00 00 00 00 00 00 B8 14` | Setting: Setting - 80, para1=[0x50 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 5A 00 00 00 00 00 00 00 00 00 00 00 FA 09` | Setting: Setting - 90 gears, para1=[0x5A 0x00 0x00 0x00] |
| Gamma intensity | `10 04 4D 00 64 00 00 00 00 00 00 00 00 00 00 00 9C 40` | Setting: Setting -100, para1=[0x64 0x00 0x00 0x00] |
| Gamma intensity | `10 04 8D 00 01 00 00 00 00 00 00 00 01 00 00 00 A8 F7` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |

---

## Image Outline

**Command Count**: 4

| Command Name | Hex Command | Parameters |
|--------------|-------------|------------|
| Hook edge position | `10 04 4E 00 00 00 00 00 00 00 00 00 00 00 00 00 0b 73` | Setting: Setting - 0 |
| Hook edge position | `10 04 4E 00 01 00 00 00 00 00 00 00 00 00 00 00 7e 70` | Setting: Setting - 1st gear, para1=[0x01 0x00 0x00 0x00] |
| Hook edge position | `10 04 4E 00 02 00 00 00 00 00 00 00 00 00 00 00 e1 75` | Setting: Setting - 2 levels, para1=[0x02 0x00 0x00 0x00] |
| Hook edge position | `10 04 8E 00 01 00 00 00 00 00 00 00 01 00 00 00 7e 79` | Setting: Get, para1=[0x01 0x00 0x00 0x00] |

---

## Notes

- **Parameter Format**: `para1` and `para2` are 4-byte arrays shown as hex values
- **Settings**: Some commands have ON/OFF variants distinguished by parameter values
- **Response Length**: Indicates expected response data length (bytes 12-13 in command)
- **Reserved Bytes**: Bytes 14-15 are reserved (usually 0x00)

---

**Generated from**: `I2C_Instructions_hierarchical.json`  
**Total Commands Documented**: 224
