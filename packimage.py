#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import os

# Use objcopy to extract the image from the ELF file.
os.system("riscv32-unknown-elf-objcopy -O binary build/main.elf build/main.bin")

# Open the generated file for appending a checksum.
fd = open("build/main.bin", "+ab")

# Determine the length of the file.
fd.seek(0, 2)
raw_size = fd.tell()

# Append padding bytes.
padd_size = ((raw_size + 15) & ~15) - raw_size
if padd_size == 0:
    fd.write(b"\0" * 15)
elif padd_size > 1:
    fd.write(b"\0" * (padd_size - 1))

# Initialise checksum.
xsum_state = 0xEF

fd.seek(1, 0)
seg_num = fd.read(1)[0]


def readword():
    raw = fd.read(4)
    return raw[0] + (raw[1] << 8) + (raw[2] << 16) + (raw[3] << 24)


# Compute checksum.
fd.seek(24)
for _ in range(seg_num):
    seg_laddr = readword()
    seg_len = readword()
    for _ in range(seg_len):
        xsum_state ^= fd.read(1)[0]

# Append checksum.
fd.seek(0, 2)
fd.write(bytes([xsum_state]))
