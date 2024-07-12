
// SPDX-License-Identifier: MIT

#include "port/dtparse.h"

#include "port/dtb.h"
#include "rawprint.h"



// Parse the DTB and add found devices.
void dtparse(void *dtb_ptr) {
}

static void pname(char const *name) {
    if (name == NULL) {
        rawprint("(NULL)");
        return;
    }
    rawputc('"');
    while (*name) {
        if (*name == '"') {
            rawputc('\\');
            rawputc('"');
        } else if (*name < 0x20) {
            rawputc('\\');
            rawputc('x');
            rawprinthex(*name, 2);
        } else {
            rawputc(*name);
        }
        name++;
    }
    rawputc('"');
}

static void pindent(size_t count) {
    while (count--) {
        rawputc(' ');
        rawputc(' ');
    }
}

static void dtdump_r(dtb_handle_t *handle, dtb_entity_t node) {
    pindent(node.depth);
    pname(node.name);
    rawprint(": {\n");

    dtb_entity_t prop = dtb_first_prop(handle, node);
    while (prop.valid) {
        pindent(node.depth + 1);
        pname(prop.name);
        rawprint(": ");
        rawprintdec(prop.prop_len, 0);
        prop = dtb_next_prop(handle, prop);
        rawprint(" byte");
        if (prop.prop_len != 1) {
            rawputc('s');
        }
        rawputc('\n');
    }

    dtb_entity_t subnode = dtb_first_node(handle, node);
    while (subnode.valid) {
        dtdump_r(handle, subnode);
        subnode = dtb_next_node(handle, subnode);
    }

    rawprint("}\n");
}

// Dump the DTB.
void dtdump(void *dtb_ptr) {
    dtb_handle_t handle = dtb_open(dtb_ptr);
    if (handle.has_errors) {
        return;
    }
    dtb_entity_t root = dtb_root_node(&handle);
    if (!root.valid) {
        return;
    }
    dtdump_r(&handle, root);
}
