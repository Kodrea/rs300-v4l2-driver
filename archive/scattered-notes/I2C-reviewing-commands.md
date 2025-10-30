

# pulled from dmesg -wH
55 43 49 00 00 10 10 45 00 00 00 00 00 00 00 00 00 00 00 00 00 fb c0

# pulled from I2C instructions table
10 10 45 00 00 00 00 00 00 00 00 00 00 00 00 00  


# ISSUE
The command used in the driver matches the SERIAL command not the I2C command required.
- Serial, do not use:  55 43 49 12 00 10 10 45 00 00 00 00 00 00 00 00 00 00 00 00 00 FB C0