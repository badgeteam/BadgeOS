#!/usr/bin/env python3

import sys

if len(sys.argv) != 4:
    print("Usage: bin2c.py [infile] [outfile] [name]")
    exit(1)

infd  = open(sys.argv[1], "rb")
outfd = open(sys.argv[2], "w")
name  = sys.argv[3]

data = infd.read()

outfd.write("// WARNING: This is a generated file, do not edit it!\n")
outfd.write("#include <stdint.h>\n")
outfd.write("#include <stddef.h>\n")
outfd.write("uint8_t const {}[] = {{\n".format(name))
for byte in data:
    outfd.write("0x{:02x},".format(byte))
outfd.write("\n};\n")
outfd.write("size_t const {}_len = {};\n".format(name, len(data)))

infd.close()
outfd.flush()
outfd.close()
