#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import os, argparse, shutil
from pathlib import Path

def patchElf(fd):
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

def main():

    parser = argparse.ArgumentParser()

    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)

    args = parser.parse_args()

    input_file: Path = args.input
    output_file: Path = args.output

    try:
        shutil.copyfile(input_file, output_file)

        # Open the generated file for appending a checksum.
        with output_file.open("ab+") as fd:
            patchElf(fd)
    
    except:
        try:
            output_file.unlink()
        except:
            pass 
        raise 

if __name__ == "__main__":
    main()