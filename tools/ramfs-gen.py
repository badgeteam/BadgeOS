#!/usr/bin/env python3

import sys, os

if len(sys.argv) != 4:
    print("Usage: ramfs-gen.py [indir] [outfile] [name]")
    exit(1)

outfd = open(sys.argv[2], "w")

outfd.write("// WARNING: This is a generated file, do not edit it!\n")
outfd.write("// clang-format off\n")
outfd.write("// NOLINTBEGIN\n")
outfd.write("#include <stdint.h>\n")
outfd.write("#include <stddef.h>\n")
outfd.write("#include \"filesystem.h\"\n")
outfd.write("#include \"badge_err.h\"\n")
outfd.write("#include \"assertions.h\"\n")

roms=""
dirs=""
files=""
file_count=0



def escape(raw: str):
    return raw\
        .replace('\\', '\\\\')\
        .replace('\r', '\\r')\
        .replace('\n', '\\n')\
        .replace('"', '\\"')


def add_rom(path, virtpath, name):
    global roms, files
    infd = open(path, "rb")
    data = infd.read()
    roms += "uint8_t const {}[] = {{\n    ".format(name)
    for byte in data:
        roms += "0x{:02x},".format(byte)
    roms += "\n};\n"
    roms += "fileoff_t const {}_len = {};\n".format(name, len(data))
    files += "    fd = fs_open(&ec, FILE_NONE, \"{}\", {}, OFLAGS_CREATE | OFLAGS_WRITEONLY);\n".format(escape(virtpath), len(virtpath))
    files += "    badge_err_assert_dev(&ec);\n"
    files += "    len = fs_write(&ec, fd, {}, {}_len);\n".format(name, name)
    files += "    badge_err_assert_dev(&ec);\n"
    files += "    assert_dev_drop(len == {}_len);\n".format(name)
    files += "    fs_close(&ec, fd);\n"
    files += "    badge_err_assert_dev(&ec);\n"
    infd.close()


def add_dir(path, virtpath):
    global dirs, file_count
    for filename in os.listdir(path):
        if os.path.isdir(path + "/" + filename):
            dirs += "    fs_dir_create(&ec, FILE_NONE, \"{}\", {});\n".format(escape(virtpath + "/" + filename), len(virtpath) + 1 + len(filename))
            dirs += "    badge_err_assert_dev(&ec);\n"
            add_dir(path + "/" + filename, virtpath + "/" + filename)
        else:
            add_rom(path + "/" + filename, virtpath + "/" + filename, "filerom_{}".format(file_count))
            file_count += 1


add_dir(sys.argv[1], "")
outfd.write(roms)
outfd.write("void {}() {{\n".format(sys.argv[3]))
outfd.write("    badge_err_t ec = {0};\n")
outfd.write("    file_t fd;\n")
outfd.write("    fileoff_t len;\n")
outfd.write(dirs)
outfd.write(files)
outfd.write("    (void)len;\n")
outfd.write("}\n")
outfd.write("// NOLINTEND\n")
outfd.flush()
outfd.close()
