
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
    dtb_handle_t *handle, dtb_node_t *node, uint32_t addr_cells, uint32_t size_cells, char const *compat_str
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
    dtb_handle_t *handle = dtb_open(dtb_ptr);
    assert_always(handle);
    dtb_node_t *root = dtb_root_node(handle);

    // The SOC node contains devices for which we may have drivers.
    dtb_node_t *soc      = dtb_get_node(handle, root, "soc");
    uint32_t    soc_alen = dtb_read_uint(handle, soc, "#address-cells");
    uint32_t    soc_slen = dtb_read_uint(handle, soc, "#size-cells");

    // Initialise timers.
    time_init_dtb(handle);
    // Initialise SMP.
    smp_init_dtb(handle);

    // Walk the SOC node to detect devices and install drivers.
    dtb_node_t *node = soc->nodes;
    while (node) {
        // // Debug log.
        // logkf_from_isr(LOG_DEBUG, "Node %{cs}", node.name);
        // dtb_entity_t reg = dtb_get_prop(handle, node, "reg");
        // for (uint32_t i = 0; i < reg.prop_len / 4 / (soc_alen + soc_slen); i++) {
        //     size_t base = dtb_prop_read_cells(handle, reg, i * (soc_alen + soc_slen), soc_alen);
        //     size_t size = dtb_prop_read_cells(handle, reg, i * (soc_alen + soc_slen) + soc_alen, soc_slen);
        //     logkf_from_isr(LOG_DEBUG, "  Reg[%{u32;d}]:  base=0x%{size;x}  size=0x%{size;x}", i, base, size);
        // }

        // Read which drivers the device is compatible with.
        dtb_prop_t *compatible = dtb_get_prop(handle, node, "compatible");
        uint32_t    compat_len = 0;
        char const *compat_str = (char *)dtb_prop_content(handle, compatible, &compat_len);

        // Check all compatible options.
        while (compat_len) {
            size_t len = cstr_length(compat_str);
            if (check_drivers(handle, node, soc_alen, soc_slen, compat_str)) {
                break;
            }
            compat_str += len + 1;
            compat_len -= len + 1;
        }

        // Next device.
        node = node->next;
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

static void dtdump_r(dtb_handle_t *handle, dtb_node_t *node) {
    pindent(node->depth);
    rawprint(node->name);
    rawprint(" {\n");

    dtb_prop_t *prop = node->props;
    while (prop) {
        pindent(node->depth + 1);
        rawprint(prop->name);
        if (prop->content_len) {
            rawprint(" = ");
            if (isbin((uint8_t *)prop->content, prop->content_len)) {
                rawputc('<');
                if (prop->content_len % 4) {
                    hexprint1((uint8_t *)prop->content, prop->content_len);
                } else {
                    hexprint4((uint32_t *)prop->content, prop->content_len / 4);
                }
                rawputc('>');
            } else {
                rawputc('"');
                escprint((char *)prop->content, prop->content_len);
                rawputc('"');
            }
        }
        rawprint(";\n");
        prop = prop->next;
    }

    dtb_node_t *subnode = node->nodes;
    while (subnode) {
        dtdump_r(handle, subnode);
        subnode = subnode->next;
    }

    pindent(node->depth);
    rawprint("}\n");
}

// Dump the DTB.
void dtdump(void *dtb_ptr) {
    dtb_handle_t *handle = dtb_open(dtb_ptr);
    if (!handle) {
        return;
    }
    dtb_node_t *root = dtb_root_node(handle);
    if (!root) {
        rawprint("Invalid root node\n");
        return;
    }
    dtdump_r(handle, root);
}
