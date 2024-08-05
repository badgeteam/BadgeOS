#!/usr/bin/env python3

# SPDX-License-Identifier: MIT

from argparse import *
import os, re

assert __name__ == "__main__"

parser = ArgumentParser()

float_spec = [
    "none",
    "single",
    "double",
]

default_options = {
    "float_spec": (["none"], 0),
    "vec_spec":   (["none"], 0),
}

targets = {
    "esp32c6": {
        "cpu":       ["riscv32"],
        "cc-match":  "^riscv.*-linux-",
        "cc-prefer": ["^riscv32-badgeros-", "^riscv32-"],
        "port":      "esp32c6",
        "options":   {},
    },
    "esp32p4": {
        "cpu":       ["riscv32"],
        "cc-match":  "^riscv.*-linux-",
        "cc-prefer": ["^riscv32-badgeros-", "^riscv32-"],
        "float":     True,
        "port":      "esp32p4",
        "options":   {
            "float_spec": (["single"], 0),
        },
    },
    "generic": {
        "cpu":       ["riscv64"],
        "cc-match":  "^riscv64.*-linux-",
        "cc-prefer": ["^riscv64-linux-"],
        "float":     True,
        "port":      "generic",
        "options":   {
            "float_spec": (float_spec, 2),
            "vec_spec":   (["none", "rvv_1"], 1),
        },
    }
}

parser.add_argument("--target", 
        action="store", default=None, choices=list(targets.keys()),
        help="Target chip, one of: "+", ".join(targets.keys()))

parser.add_argument("--use-default",
        action="store_true",
        help="Use the default option values instead of prompting")

parser.add_argument("--cpu", "--arch", 
        action="store", default=None,
        help="CPU architecture, one of: riscv32, riscv64")

parser.add_argument("--compiler", "--cc", 
        action="store", default=None,
        help="C compiler, toolchain prefix is derived from this")

parser.add_argument("--fp-spec", "--float-spec", "--fp", "--float", 
        action="store", default=None,
        help="Floating-point type to enable")

parser.add_argument("--vec-spec", "--vector-spec", "--vec", "--vector", 
        action="store", default=None,
        help="Vector type to enable")

args = parser.parse_args()
use_default = args.use_default


def option_select(prompt: str, options: list, prefer=0):
    global use_default
    prefer += 1
    if len(options) == 0:
        print(f"Error: No valid {prompt}s found")
        exit(1)
    elif len(options) == 1:
        return options[0]
    elif use_default:
        return options[prefer-1]
    else:
        print(f"Available {prompt}s:")
    for i in range(len(options)):
        print(f"[{i+1:d}] {options[i]}")
    sel = input(f"Select a {prompt} [{prefer}] ")
    try:
        i = int(sel) if len(sel) else prefer
        if i < 1 or i > len(options):
            exit(1)
        return options[i-1]
    except ValueError:
        exit(1)

def find_compilers():
    global target
    candidates  = []
    prefer_idx  = 1
    prefer_prio = 99999999
    for path in os.getenv("PATH").split(":"):
        path = os.path.abspath(path)
        try:
            for bin in os.listdir(path):
                if not bin.endswith("gcc"): continue
                if not re.findall(targets[target]["cc-match"], bin): continue
                for i in range(len(targets[target]["cc-prefer"])):
                    prefer = targets[target]["cc-prefer"][i]
                    if i >= prefer_prio: break
                    if re.findall(prefer, bin):
                        prefer_idx  = len(candidates)
                        prefer_prio = i
                        break
                candidates.append(path + "/" + bin)
        except FileNotFoundError:
            pass
    return candidates, prefer_idx

def handle_option_arg(arg: str|None, id: str, name: str) -> str:
    global target, target_options
    if not arg: return option_select(name, *target_options[id])
    if arg not in target_options[id][0]:
        print(f"ERROR: Chosen {name} `{arg}` not supported by target `{target}`")
        exit(1)
    return arg


target = args.target or option_select("target", list(targets.keys()))
config = {}
target_options = default_options.copy()
for opt in targets[target]["options"]:
    target_options[opt] = targets[target]["options"][opt]

if args.cpu and args.cpu not in targets[target]["cpu"]:
    print(f"ERROR: Chosen CPU architecture `{args.cpu}` not supported by target `{target}`")
    exit(1)
config["cpu"] = args.cpu or option_select("CPU", targets[target]["cpu"])

if args.compiler:
    if not re.findall(targets[target]["cc-match"], args.compiler):
        print(f"WARNING: Chosen compiler `{args.compiler}` does not match /{targets[target]['cc-match']}/")
    config["compiler"] = args.compiler
else:
    config["compiler"] = option_select("compiler", *find_compilers())

config["fp_spec"]  = handle_option_arg(args.fp_spec,  "float_spec", "float spec")
config["vec_spec"] = handle_option_arg(args.vec_spec, "vec_spec",   "vector spec")

config["target"]    = target
cc_re = re.match("^(.+?)\\w+$", config["compiler"])
if not cc_re:
    print("ERROR: Cannot determine toolchain prefix")
    exit(1)
config["tc_prefix"] = cc_re.group(1)


os.makedirs(".config", exist_ok=True)

with open(".config/config.mk", "w") as fd:
    fd.write(f'# WARNING: This is a generated file, do not edit it!\n')
    for opt in config:
        fd.write(f'CONFIG_{opt.upper()} = {config[opt]}\n')

with open(".config/config.cmake", "w") as fd:
    fd.write(f'# WARNING: This is a generated file, do not edit it!\n')
    for opt in config:
        fd.write(f'set(CONFIG_{opt.upper()} {config[opt]})\n')

with open(".config/config.h", "w") as fd:
    fd.write(f'// WARNING: This is a generated file, do not edit it!\n')
    fd.write(f'// clang-format off\n')
    fd.write(f'#pragma once\n')
    for opt in config:
        fd.write(f'#define CONFIG_{opt.upper()} "{config[opt]}"\n')
        if re.match('^\\w+$', config[opt]):
            fd.write(f'#define CONFIG_{opt.upper()}_{config[opt]}\n')
