#!/usr/bin/env python3

# SPDX-License-Identifier: MIT

from argparse import *
import os, re, typing, subprocess

assert __name__ == "__main__"

T = typing.TypeVar('T')
parser  = ArgumentParser()
options = {}
known_compiler_names = ["cc", "gcc", "clang"]



def prompt_option(prompt: str, options: dict[str,T]|list[T], default: str|int|T = None) -> T:
    if type(options) != dict:
        options = [(str(x), x) for x in options]
    else:
        options = [(k, options[k]) for k in options]
    defidx = None
    if type(default) != int:
        for i in range(len(options)):
            if options[i][0] == default or options[i][1] == default:
                defidx = i
                break
    print(f"Available {prompt} options:")
    for i in range(len(options)):
        print(f"[{i+1}] {options[i][0]}")
    while True:
        if defidx != None:
            idx = input(f"Select {prompt} [{defidx+1}] ")
            if not idx: return options[defidx][1]
        else:
            idx = input(f"Select {prompt}: ")
        try:
            idx = int(idx)
            if idx >= 1 and idx <= len(options):
                return options[idx-1][1]
        except:
            pass


class Desc:
    def __init__(self, id: str, name: str, help: str):
        self.id   = id
        self.name = name
        self.help = help
    
    def argument(self, parser: ArgumentParser):
        parser.add_argument(f"--{self.id}", action="store", required=False, help=self.help)


class Option(typing.Generic[T]):
    def __init__(self):
        self.defval = None
    
    def use_default(self) -> T: return self.defval
    def select(self, desc: Desc) -> T: raise NotImplementedError()
    def parse(self, desc: Desc, value: str) -> T: raise NotImplementedError()


class OptConst(Option[T]):
    def __init__(self, value: T):
        self.defval = value
    def select(self, desc: Desc) -> T:
        print(f"Using {desc.name} `{self.defval}`")
        return self.defval
    def parse(self, desc: Desc, val: str) -> int:
        if val != str(self.defval):
            print(f"Error: Unsupported {desc.name} `{val}`")
            exit(1)
        return self.defval


class OptInt(Option[int]):
    def __init__(self, min: int, max: int, inc: int = 1, defval: int = None):
        assert inc >= 1
        assert (max - min) % inc == 0
        assert defval == None or (defval - min) % inc == 0
        self.min    = min
        self.max    = max
        self.inc    = inc
        self.defval = min if defval == None else defval
    
    def select(self, desc: Desc) -> int:
        prompt = f"{self.min} - {self.max}"
        if self.inc != 1: prompt += f" step {self.inc}"
        prompt = f"{desc.name}: {prompt} [{self.defval}] "
        while True:
            val = input(prompt)
            if len(val) == 0:
                return self.defval
            try:
                val = int(val)
                if (val - self.min) % self.inc != 0 or val < self.min or val > self.max:
                    continue
                return val
            except ValueError:
                continue
    
    def parse(self, desc: Desc, val: str) -> int:
        try:
            val = int(val)
            if (val - self.min) % self.inc != 0 or val < self.min or val > self.max:
                print(f"{desc.name} {val} out of range {self.xhelp}")
                exit(1)
            return val
        except ValueError:
            print(f"{desc.name} invalid integer {val}")
            exit(1)


class OptEnum(Option[T]):
    def __init__(self, options: dict[str, T]|list[str], defval: T = None):
        self.options: dict[str, T]
        if type(options) != dict:
            self.options = {x: x for x in options}
        else:
            self.options = options
        if defval == None:
            self.defval  = self.options.values()[0]
        else:
            assert defval in self.options.values()
            self.defval  = defval
    
    def select(self, desc: Desc) -> T:
        keys = list(self.options.keys())
        if len(keys) == 1:
            print(f"Using {desc.name} `{self.defval}`")
            return self.defval
        while True:
            print(f"Available {desc.name} options:")
            for i in range(len(keys)):
                print(f"[{i+1}] {self.options[keys[i]]}")
            defidx = keys.index(self.defval)+1
            idx = input(f"Select {desc.name} [{defidx}] ") or str(defidx)
            try:
                idx = int(idx)
                if idx >= 1 and idx <= len(keys):
                    return self.options[keys[idx-1]]
            except ValueError:
                continue
    
    def parse(self, desc: Desc, val: str) -> T:
        try:
            return self.options[val]
        except KeyError:
            print(f"Invalid {desc.name} `{val}`")
            exit(1)


class OptCompiler(Option[str]):
    def __init__(self, match: str, prefer: list[str]):
        self.match   = re.compile(match)
        self.prefer  = [re.compile(x) for x in prefer]
        self.options = []
        self.defidx  = 0
        self.defval  = None
    
    def _prio(self, cc: str) -> int|None:
        for i in range(len(self.prefer)):
            if self.prefer[i].match(cc):
                return i
        return None
    
    def _query_compiler_arch(self, path):
        res = subprocess.run([path, '-dumpmachine'], capture_output=True)
        if res.returncode != 0: return None
        return res.stdout.decode().strip()
    
    def _search(self):
        self.options = []
        self.defidx  = 0
        priority     = None
        for dir in os.getenv("PATH").split(os.pathsep):
            try:
                for bin in os.listdir(dir):
                    # Check for RE-matched compilers.
                    if not bin.split('-')[-1] in known_compiler_names: continue
                    arch = self._query_compiler_arch(bin)
                    if arch == None: continue
                    if not self.match.match(arch): continue
                    self.options.append(dir + os.path.sep + bin)
                    opt_prio = self._prio(bin)
                    if opt_prio != None and priority == None:
                        priority = opt_prio
                        self.defidx = len(self.options)-1
                    elif opt_prio != None and priority != None and opt_prio < priority:
                        priority = opt_prio
                        self.defidx = len(self.options)-1
            except FileNotFoundError:
                continue
        self.defval = self.options[self.defidx]
    
    def use_default(self) -> str:
        self._search()
        return self.defval
    
    def select(self, desc: Desc) -> str:
        self._search()
        if len(self.options) == 0:
            print("Warning: No suitable compilers found!")
            return input("Select compiler: ")
        while True:
            print("Available compilers:")
            for i in range(len(self.options)):
                print(f"[{i+1}] {self.options[i]}")
            try:
                val = input(f"Select compiler [{self.defidx+1}] ")
                if len(val) == 0:
                    return self.options[self.defidx]
                else:
                    return self.options[int(val)-1]
            except ValueError:
                print(f"Warning: Selected unsupported compiler: {val}")
                return val
            except IndexError:
                continue
    
    def parse(self, desc: Desc, val: str) -> T:
        self._search()
        if len(self.options) == 0:
            print("Warning: No suitable compilers found!")
        elif val not in self.options:
            print(f"Warning: Selected unsupported compiler: {val}")
        return val


class OptStr(Option[str]):
    def __init__(self, defval: str):
        self.defval = defval
    
    def select(self, desc: Desc) -> str:
        return input(f"{desc.name} [{self.defval}] ") or self.defval
            
    def parse(self, desc: Desc, value: str) -> T:
        print(f"Selected {desc.name} `{value}`")
        return value



option_desc = {x.id: x for x in [
    Desc("compiler",   "compiler",         "C compiler to use for building BadgerOS and apps."),
    Desc("cpu",        "CPU architecture", "CPU architecture to build for."),
    Desc("float_spec", "floating-point",   "Largest floating-point type to support."),
    Desc("vec_spec",   "vector",           "Largest vector type to support."),
    Desc("stack_size", "stack size",       "Stack size to use for kernel threads."),
]}

default_options = {
    "stack_size": OptInt(8192, 65536, 4096, 16384),
    "float_spec": OptEnum(["none", "single", "double"], "double"),
}

arch_default_options = {
    "riscv64": {
        "compiler": OptCompiler("^riscv64.*-linux-", ["^riscv64-badgeros-", "^riscv64-linux-"]),
        "vec_spec": OptEnum(["none", "rvv_1"], "rvv_1"),
    },
    "amd64": {
        "compiler": OptCompiler("^x86_64.*-linux-", ["^x86_64-badgeros-", "^x86_64-linux-"]),
        "vec_spec": OptEnum(["none", "sse", "avx", "avx2"], "avx"),
    }
}
arch_default_options["riscv32"] = arch_default_options["riscv64"]

default_target = "esp32p4"
targets = {
    "esp32c6": {
        "compiler":   OptCompiler("^riscv32.*-linux-", ["^riscv32-badgeros-", "^riscv32-linux-"]),
        "cpu":        OptConst("riscv32"),
        "float_spec": OptConst("none"),
        "vec_spec":   OptConst("none"),
    },
    "esp32p4": {
        "compiler":   OptCompiler("^riscv32.*-linux-", ["^riscv32-badgeros-", "^riscv32-linux-"]),
        "cpu":        OptConst("riscv32"),
        "float_spec": OptConst("single"),
        "vec_spec":   OptConst("none"),
    },
    "generic": {
        "cpu":        OptEnum(["riscv64", "amd64"], "riscv64"),
    }
}



parser.add_argument("--target", 
        action="store", default=None, choices=list(targets.keys()),
        help="Target platform, one of: "+", ".join(targets.keys()))

parser.add_argument("--use-default",
        action="store_true",
        help="Use the default option values instead of prompting")

for desc in option_desc.values():
    desc.argument(parser)

args = vars(parser.parse_args())

try:
    if args["target"] == None:
        args["target"] = prompt_option("target", targets.keys(), "generic")

    config = {}
    config["target"] = args["target"]
    options: dict[str, Option] = {}
    for k in targets[args["target"]]:
        options[k] = targets[args["target"]][k]

    if args["cpu"] == None:
        if args["use_default"]:
            config["cpu"] = options["cpu"].use_default()
        else:
            config["cpu"] = options["cpu"].select(option_desc["cpu"])
    else:
        config["cpu"] = args["cpu"]
    del options["cpu"]
    cpu = args["cpu"]
    
    for k in arch_default_options[config["cpu"]]:
        if k not in options:
            options[k] = arch_default_options[config["cpu"]][k]
    
    for k in default_options:
        if k not in options:
            options[k] = default_options[k]

    for k in options:
        if args[k] != None:
            config[k] = options[k].parse(option_desc[k], args[k])
        elif args["use_default"]:
            config[k] = options[k].use_default()
            print(f"Using {option_desc[k].name} `{config[k]}`")
        else:
            config[k] = options[k].select(option_desc[k])

except (KeyboardInterrupt, EOFError):
    print()
    print("Cancelled")
    exit(1)

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
        if type(config[opt]) == int:
            fd.write(f'#define CONFIG_{opt.upper()} {config[opt]}\n')
        else:
            fd.write(f'#define CONFIG_{opt.upper()} "{config[opt]}"\n')
            if re.match('^\\w+$', config[opt]):
                fd.write(f'#define CONFIG_{opt.upper()}_{config[opt]}\n')
