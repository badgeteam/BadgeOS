#!/usr/bin/env python3

# SPDX-License-Identifier: MIT

import sys, subprocess, io
from argparse import *

assert __name__ == "__main__"

parser = ArgumentParser(description="Reads stdin for kernel addresses and injects their linenumbers")
parser.add_argument("-L", "--long", "--64bit", action="store_true", help="Use 64-bit addresses")
parser.add_argument("-A", "--addr2line", action="store", help="addr2line program")
parser.add_argument("file", action="store", help="Executable file to read symbols from")
args = parser.parse_args()

hextab = "0123456789abcdef"
buf    = ''
hexits = 16 if args.long else 8

state = -2

def addr2line():
    global buf
    cmd = [
        args.addr2line, '-e', args.file, buf
    ]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    result  = process.stdout.read()
    process.wait()
    string  = result.decode().strip()
    if process.returncode: return
    if string == '??:0': return
    if string.endswith('?'): return
    sys.stdout.write(" (")
    sys.stdout.write(string)
    sys.stdout.write(")")

while True:
    try:
        msg_raw = sys.stdin.buffer.read(1)
    except (KeyboardInterrupt, EOFError):
        break
    if not len(msg_raw): break
    msg = chr(msg_raw[0])
    for char in msg:
        if state == -2 and char == '0':
            state = -1
            sys.stdout.write(char)
        elif state == -1 and char in 'xX':
            state = 0
            buf   = ''
            sys.stdout.write(char)
        elif state == hexits:
            if char.lower() not in hextab:
                addr2line()
            sys.stdout.write(char)
            state = -2
        elif char.lower() in hextab:
            buf   += char
            state += 1
            sys.stdout.write(char)
        else:
            state = -2
            sys.stdout.write(char)
    sys.stdout.flush()
