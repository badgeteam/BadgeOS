
// SPDX-License-Identifier: MIT

#include "port/dtparse.h"

#include "assertions.h"
#include "badge_strings.h"
#include "driver.h"
#include "port/dtb.h"
#include "rawprint.h"
#include "smp.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe32(x) __builtin_bswap32(x)
#define be32toh(x) __builtin_bswap32(x)
#else
#define htobe32(x) (x)
#define be32toh(x) (x)
#endif



// Check if we have a driver for some compat string.
static bool check_drivers(
    dtb_handle_t *handle, dtb_entity_t node, uint32_t addr_cells, uint32_t size_cells, char const *compat_str
) {
    for (driver_t const *driver = start_drivers; driver != stop_drivers; driver++) {
        for (size_t j = 0; j < driver->dtb_supports_len; j++) {
            if (cstr_equals(compat_str, driver->dtb_supports[j])) {
                driver->dtbinit(handle, node, addr_cells, size_cells);
                return true;
            }
        }
    }
    return false;
}

// Parse the DTB and add found devices.
void dtparse(void *dtb_ptr) {
    // Open the DTB for reading.
    dtb_handle_t handle = dtb_open(dtb_ptr);
    assert_always(!handle.has_errors);
    dtb_entity_t root = dtb_root_node(&handle);

    // The SOC node contains devices for which we may have drivers.
    dtb_entity_t soc      = dtb_get_node(&handle, root, "soc");
    uint32_t     soc_alen = dtb_read_uint(&handle, soc, "#address-cells");
    uint32_t     soc_slen = dtb_read_uint(&handle, soc, "#size-cells");

    // Initialise SMP.
    smp_init(&handle);

    // Walk the SOC node to detect devices and install drivers.
    dtb_entity_t node = dtb_first_node(&handle, soc);
    while (node.valid) {
        // // Debug log.
        // logkf_from_isr(LOG_DEBUG, "Node %{cs}", node.name);
        // dtb_entity_t reg = dtb_get_prop(&handle, node, "reg");
        // for (uint32_t i = 0; i < reg.prop_len / 4 / (soc_alen + soc_slen); i++) {
        //     size_t base = dtb_prop_read_cells(&handle, reg, i * (soc_alen + soc_slen), soc_alen);
        //     size_t size = dtb_prop_read_cells(&handle, reg, i * (soc_alen + soc_slen) + soc_alen, soc_slen);
        //     logkf_from_isr(LOG_DEBUG, "  Reg[%{u32;d}]:  base=0x%{size;x}  size=0x%{size;x}", i, base, size);
        // }

        // Read which drivers the device is compatible with.
        dtb_entity_t compatible = dtb_get_prop(&handle, node, "compatible");
        char const  *compat_str = (char *)(handle.struct_blk + compatible.content);
        size_t       compat_len = compatible.prop_len;

        // Check all compatible options.
        while (compat_len) {
            size_t len = cstr_length(compat_str);
            if (check_drivers(&handle, node, soc_alen, soc_slen, compat_str)) {
                break;
            }
            compat_str += len + 1;
            compat_len -= len + 1;
        }

        // Next device.
        node = dtb_next_node(&handle, node);
    }
}



static void pindent(size_t count) {
    while (count--) {
        rawputc(' ');
        rawputc(' ');
    }
}

static bool isbin(uint8_t const *mem, uint32_t len) {
    if (mem[0] == 0 || mem[len - 1] != 0) {
        return true;
    }
    while (len--) {
        if (*mem && (*mem < 0x20 || *mem > 0x7e)) {
            return true;
        }
        mem++;
    }
    return false;
}

static void escprint(char const *str, uint32_t len) {
    while (len--) {
        if (len == 0 && *str == 0) {
            return;
        } else if (*str == 0) {
            rawputc('\\');
            rawputc('0');
        } else {
            rawputc(*str);
        }
        str++;
    }
}

static void hexprint4(uint32_t const *bin, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        rawprint("0x");
        rawprinthex(be32toh(bin[i]), 8);
        if (i < len - 1) {
            rawputc(' ');
        }
    }
}

static void hexprint1(uint8_t const *bin, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        rawprint("0x");
        rawprinthex(bin[i], 2);
        if (i < len - 1) {
            rawputc(' ');
        }
    }
}

static void dtdump_r(dtb_handle_t *handle, dtb_entity_t node) {
    pindent(node.depth);
    rawprint(node.name);
    rawprint(" {\n");

    dtb_entity_t prop = dtb_first_prop(handle, node);
    while (prop.valid) {
        pindent(node.depth + 1);
        rawprint(prop.name);
        if (prop.prop_len) {
            rawputc(' ');
            if (isbin((uint8_t *)(handle->struct_blk + prop.content), prop.prop_len)) {
                rawputc('<');
                if (prop.prop_len % 4) {
                    hexprint1((uint8_t *)(handle->struct_blk + prop.content), prop.prop_len);
                } else {
                    hexprint4((uint32_t *)(handle->struct_blk + prop.content), prop.prop_len / 4);
                }
                rawputc('>');
            } else {
                rawputc('"');
                escprint((char *)(handle->struct_blk + prop.content), prop.prop_len);
                rawputc('"');
            }
        }
        rawprint(";\n");
        prop = dtb_next_prop(handle, prop);
    }

    dtb_entity_t subnode = dtb_first_node(handle, node);
    while (subnode.valid) {
        dtdump_r(handle, subnode);
        subnode = dtb_next_node(handle, subnode);
    }

    pindent(node.depth);
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
        rawprint("Invalid root node\n");
        return;
    }
    root.name = "/";
    dtdump_r(&handle, root);
}
