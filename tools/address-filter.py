#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

import sys, re, subprocess, io

file_name = "build/badger-os.elf"

if len(sys.argv) > 1:
    file_name = sys.argv[1]

matcher = re.compile("(0x[0-9A-Za-z]{8})")


def runAndCapture(cmd):
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)

    result = process.stdout.read()

    process.wait()

    string = result.decode().strip()

    if process.returncode == 0 and string != "??:0":
        return string
    else:
        return None


def addr2line(x):
    global file_name

    addr = int(x.group(1), 16)

    addr_string = "0x{:0>8x}".format(addr)

    result = runAndCapture(
        ["riscv32-unknown-elf-addr2line", "-e", file_name, addr_string]
    )

    if result != None:
        return f"{addr_string} ({result})"
    else:
        return addr_string


# https://stackoverflow.com/questions/73335410/how-to-read-sys-stdin-containing-binary-data-in-python-ignore-errors
sys.stdin = io.TextIOWrapper(sys.stdin.buffer, errors="ignore")

for line in sys.stdin:
    line = matcher.sub(addr2line, line.rstrip())

    sys.stdout.write(line + "\r\n")

sys.stdout.write("\n")
