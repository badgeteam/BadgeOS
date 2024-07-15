
// SPDX-License-Identifier: MIT

#include "port/dtb.h"

#include "assertions.h"
#include "badge_strings.h"
#include "log.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe32(x) __builtin_bswap32(x)
#define be32toh(x) __builtin_bswap32(x)
#else
#define htobe32(x) (x)
#define be32toh(x) (x)
#endif


// Interpret the DTB header and prepare for reading.
dtb_handle_t dtb_open(void *dtb_ptr) {
    dtb_header_t *hdr    = dtb_ptr;
    dtb_handle_t  handle = {0};
    handle.dtb_hdr       = dtb_ptr;

    logkf_from_isr(LOG_DEBUG, "DTB pointer:   0x%{size;x}", dtb_ptr);

    // Magic check.
    if (hdr->magic != FDT_HEADER_MAGIC) {
        logk_from_isr(LOG_ERROR, "Invalid magic");
        handle.has_errors = true;
        return handle;
    }
    handle.string_blk = (char *)dtb_ptr + hdr->off_dt_strings;
    handle.struct_blk = (uint32_t *)((char *)dtb_ptr + hdr->off_dt_struct);

    logkf_from_isr(LOG_DEBUG, "String offset: 0x%{u32;x}", hdr->off_dt_strings);
    logkf_from_isr(LOG_DEBUG, "Struct offset: 0x%{u32;x}", hdr->off_dt_struct);
    logkf_from_isr(LOG_DEBUG, "String block:  0x%{size;x}", handle.string_blk);
    logkf_from_isr(LOG_DEBUG, "Struct block:  0x%{size;x}", handle.struct_blk);

    return handle;
}


// Go to the first node or prop in the DTB.
dtb_entity_t dtb_root_node(dtb_handle_t *handle) {
    uint32_t i = 0;
    while (handle->struct_blk[i] == htobe32(FDT_NOP)) {
        i++;
    }
    if (handle->struct_blk[i] != htobe32(FDT_BEGIN_NODE)) {
        return (dtb_entity_t){0};
    }
    size_t name_len = cstr_length((char *)(handle->struct_blk + i + 1));
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = 0,
        .content = i + name_len / 4 + 2,
        .name    = (char *)(handle->struct_blk + i + 1),
    };
}


// Go to the first subnode in a node.
dtb_entity_t dtb_first_node(dtb_handle_t *handle, dtb_entity_t parent_node) {
    uint32_t i = parent_node.content;
    // Skip NOPs.
    while (handle->struct_blk[i] == htobe32(FDT_NOP)) {
        i++;
    }
    // Skip props.
    while (handle->struct_blk[i] == htobe32(FDT_PROP)) {
        i += 3 + (be32toh(handle->struct_blk[i + 1]) + 3) / 4;
        while (handle->struct_blk[i] == htobe32(FDT_NOP)) {
            i++;
        }
    }
    // This should be a node.
    if (handle->struct_blk[i] != htobe32(FDT_BEGIN_NODE)) {
        return (dtb_entity_t){0};
    }
    size_t name_len = cstr_length((char *)(handle->struct_blk + i + 1));
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = parent_node.depth + 1,
        .content = i + name_len / 4 + 2,
        .name    = (char *)(handle->struct_blk + i + 1),
    };
}

// Go to the first prop in a node.
dtb_entity_t dtb_first_prop(dtb_handle_t *handle, dtb_entity_t parent_node) {
    uint32_t i = parent_node.content;
    while (handle->struct_blk[i] == htobe32(FDT_NOP)) {
        i++;
    }
    if (handle->struct_blk[i] != htobe32(FDT_PROP)) {
        return (dtb_entity_t){0};
    }
    return (dtb_entity_t){
        .valid    = true,
        .is_node  = false,
        .depth    = parent_node.depth + 1,
        .content  = i + 3,
        .prop_len = be32toh(handle->struct_blk[i + 1]),
        .name     = handle->string_blk + be32toh(handle->struct_blk[i + 2]),
    };
}


// Go to the next node on the same level of hierarchy.
dtb_entity_t dtb_next_node(dtb_handle_t *handle, dtb_entity_t from) {
    if (!from.valid || !from.is_node) {
        return (dtb_entity_t){0};
    }
    uint32_t i     = from.content;
    uint8_t  depth = from.depth;
    while (true) {
        while (handle->struct_blk[i] == htobe32(FDT_NOP)) {
            i++;
        }
        if (handle->struct_blk[i] == htobe32(FDT_PROP)) {
            i += 3 + (be32toh(handle->struct_blk[i + 1]) + 3) / 4;
        } else if (handle->struct_blk[i] == htobe32(FDT_BEGIN_NODE)) {
            if (depth == from.depth - 1) {
                break;
            }
            size_t len  = cstr_length((char *)(handle->struct_blk + i + 1));
            i          += len / 4 + 2;
            depth++;
        } else if (handle->struct_blk[i] == htobe32(FDT_END_NODE)) {
            if (depth == from.depth - 1) {
                return (dtb_entity_t){0};
            }
            depth--;
            i++;
        } else {
            return (dtb_entity_t){0};
        }
    }
    size_t name_len = cstr_length((char *)(handle->struct_blk + i + 1));
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = from.depth,
        .content = i + name_len / 4 + 2,
        .name    = (char *)(handle->struct_blk + i + 1),
    };
}

// Go to the next prop in this node.
dtb_entity_t dtb_next_prop(dtb_handle_t *handle, dtb_entity_t from) {
    if (!from.valid || from.is_node) {
        return (dtb_entity_t){0};
    }
    uint32_t i = from.content + (from.prop_len + 3) / 4;
    while (handle->struct_blk[i] == htobe32(FDT_NOP)) {
        i++;
    }
    if (handle->struct_blk[i] != htobe32(FDT_PROP)) {
        return (dtb_entity_t){0};
    }
    return (dtb_entity_t){
        .valid    = true,
        .is_node  = false,
        .depth    = from.depth,
        .content  = i + 3,
        .prop_len = be32toh(handle->struct_blk[i + 1]),
        .name     = handle->string_blk + be32toh(handle->struct_blk[i + 2]),
    };
}


// Get a node with a specific name.
dtb_entity_t dtb_get_node_l(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len) {
    dtb_entity_t node = dtb_first_node(handle, parent_node);
    while (node.valid && (!cstr_prefix_equals(node.name, name, name_len) || node.name[name_len])) {
        node = dtb_next_node(handle, node);
    }
    return node;
}

// Get a prop with a specific name.
dtb_entity_t dtb_get_prop_l(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len) {
    dtb_entity_t prop = dtb_first_prop(handle, parent_node);
    while (prop.valid && (!cstr_prefix_equals(prop.name, name, name_len) || prop.name[name_len])) {
        prop = dtb_next_prop(handle, prop);
    }
    return prop;
}


// Find a node in the DTB.
dtb_entity_t dtb_find_node(dtb_handle_t *handle, char const *path) {
    dtb_entity_t node = dtb_root_node(handle);
    while (true) {
        while (path[0] == '/') {
            path++;
        }
    }
}


// Read a prop as a single unsigned number.
uintmax_t dtb_prop_read_uint(dtb_handle_t *handle, dtb_entity_t prop) {
    assert_dev_drop(prop.prop_len > 0 && prop.prop_len <= sizeof(uintmax_t));
    uintmax_t      data = 0;
    uint8_t const *ptr  = (uint8_t *)(handle->struct_blk + prop.content);
    for (size_t i = 0; i < prop.prop_len; i++) {
        data <<= 8;
        data  |= (uintmax_t)ptr[i];
    }
    return data;
}

// Read a prop as an array of cells.
uint32_t dtb_prop_read_cell(dtb_handle_t *handle, dtb_entity_t prop, uint32_t cell_idx) {
    assert_dev_drop(prop.prop_len % 4 == 0);
    assert_dev_drop(prop.prop_len / 4 > cell_idx);
    return be32toh(handle->struct_blk[prop.content + cell_idx]);
}

// Read an unsigned number from a prop formatted as cells.
uintmax_t dtb_prop_read_cells(dtb_handle_t *handle, dtb_entity_t prop, uint32_t cell_idx, uint32_t cell_count) {
    assert_dev_drop(prop.prop_len % 4 == 0);
    assert_dev_drop(prop.prop_len / 4 >= cell_count + cell_idx);
    uintmax_t data = 0;
    for (uint32_t i = 0; i < cell_count; i++) {
        data <<= 32;
        data  |= be32toh(handle->struct_blk[prop.content + cell_idx + i]);
    }
    return data;
}


// Read a prop as a single unsigned number.
uintmax_t dtb_read_uint_l(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len) {
    dtb_entity_t prop = dtb_get_prop_l(handle, parent_node, name, name_len);
    return prop.valid ? dtb_prop_read_uint(handle, prop) : 0;
}

// Read a prop as an array of cells.
uint32_t dtb_read_cell_l(
    dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len, uint32_t cell_idx
) {
    dtb_entity_t prop = dtb_get_prop_l(handle, parent_node, name, name_len);
    return prop.valid ? dtb_prop_read_cell(handle, prop, cell_idx) : 0;
}

// Read an unsigned number from a prop formatted as cells.
uintmax_t dtb_read_cells_l(
    dtb_handle_t *handle,
    dtb_entity_t  parent_node,
    char const   *name,
    size_t        name_len,
    uint32_t      cell_idx,
    uint32_t      cell_count
) {
    dtb_entity_t prop = dtb_get_prop_l(handle, parent_node, name, name_len);
    return prop.valid ? dtb_prop_read_cells(handle, prop, cell_idx, cell_count) : 0;
}
