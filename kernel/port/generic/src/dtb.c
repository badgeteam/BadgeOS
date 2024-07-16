
// SPDX-License-Identifier: MIT

#include "port/dtb.h"

#include "arrays.h"
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



// Sort phandles by index.
static int phandle_cmp(void const *a, void const *b) {
    uint32_t const *phandle_a = a;
    uint32_t const *phandle_b = b;
    return *phandle_a - *phandle_b;
}

// Interpret the DTB header and prepare for reading.
dtb_handle_t dtb_open(void *dtb_ptr) {
    dtb_header_t *hdr    = (dtb_header_t *)dtb_ptr;
    dtb_handle_t  handle = {0};
    handle.dtb_hdr       = hdr;

    logkf_from_isr(LOG_DEBUG, "DTB pointer:   0x%{size;x}", dtb_ptr);

    // Magic check.
    if (hdr->magic != FDT_HEADER_MAGIC) {
        logk_from_isr(LOG_ERROR, "Invalid magic");
        handle.has_errors = true;
        return handle;
    }
    handle.string_blk = (char *)dtb_ptr + hdr->off_dt_strings;
    handle.struct_blk = (uint32_t *)((char *)dtb_ptr + hdr->off_dt_struct);

    // Walk the DTB for phandles.
    size_t       phandles_cap = 0;
    size_t       parents_cap  = 0;
    dtb_entity_t ent          = dtb_root_node(&handle);
    do {
        if (ent.is_node) {
            dtb_entity_t phandle = dtb_get_prop(&handle, ent, "phandle");
            if (phandle.valid && phandle.prop_len == 4) {
                // Write phandle entry.
                dtb_phandle_t new_ent = {
                    .phandle = dtb_prop_read_cell(&handle, phandle, 0),
                    .depth   = ent.depth,
                    .name    = ent.name,
                    .content = ent.content,
                };
                array_lencap_sorted_insert(
                    &handle.phandles,
                    sizeof(dtb_phandle_t),
                    &handle.phandles_len,
                    &phandles_cap,
                    &new_ent,
                    phandle_cmp
                );
            }

            // Write beginning of parent entry.
            dtb_parent_t new_ent = {
                .depth   = ent.depth,
                .name    = ent.name,
                .content = ent.content,
            };
            array_lencap_insert(
                &handle.parents,
                sizeof(dtb_parent_t),
                &handle.parents_len,
                &parents_cap,
                &new_ent,
                handle.parents_len
            );
        }
        ent = dtb_walk_next(&handle, ent);
    } while (ent.valid);

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

// Walk to the next node or prop in the DTB.
dtb_entity_t dtb_walk_next(dtb_handle_t *handle, dtb_entity_t from) {
    if (!from.valid) {
        return (dtb_entity_t){0};
    }
    uint8_t  depth = from.depth;
    uint32_t i     = from.is_node ? from.content : from.content + (from.prop_len + 3) / 4;
    while (true) {
        uint32_t token = be32toh(handle->struct_blk[i]);
        if (token == FDT_PROP) {
            return (dtb_entity_t){
                .valid    = true,
                .is_node  = false,
                .depth    = depth + from.is_node,
                .content  = i + 3,
                .prop_len = be32toh(handle->struct_blk[i + 1]),
                .name     = handle->string_blk + be32toh(handle->struct_blk[i + 2]),
            };
        } else if (token == FDT_BEGIN_NODE) {
            size_t name_len = cstr_length((char *)(handle->struct_blk + i + 1));
            return (dtb_entity_t){
                .valid   = true,
                .is_node = true,
                .depth   = depth + from.is_node,
                .content = i + name_len / 4 + 2,
                .name    = (char *)(handle->struct_blk + i + 1),
            };
        } else if (token == FDT_END_NODE) {
            depth--;
        } else if (token != FDT_NOP) {
            return (dtb_entity_t){0};
        }
        i++;
    }
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
    // TODO.
    return (dtb_entity_t){0};
}


// Find the immediate parent node of a node or prop.
dtb_entity_t dtb_find_parent(dtb_handle_t *handle, dtb_entity_t ent) {
    if (!ent.valid) {
        return (dtb_entity_t){0};
    }
    dtb_entity_t parent = dtb_root_node(handle);
    dtb_entity_t cur    = dtb_walk_next(handle, parent);
    while (cur.valid) {
        if (cur.content == ent.content) {
            return parent;
        } else if (cur.is_node) {
            parent = cur;
        }
        cur = dtb_walk_next(handle, cur);
    }
    return (dtb_entity_t){0};
}

// Get a DTB node by phandle.
dtb_entity_t dtb_phandle_node(dtb_handle_t *handle, uint32_t phandle) {
    array_binsearch_t res =
        array_binsearch(handle->phandles, sizeof(dtb_phandle_t), handle->phandles_len, &phandle, phandle_cmp);
    if (!res.found) {
        return (dtb_entity_t){0};
    }
    dtb_phandle_t ent = handle->phandles[res.index];
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = ent.depth,
        .content = ent.content,
        .name    = ent.name,
    };
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

// Get raw prop contents.
void const *dtb_prop_content(dtb_handle_t *handle, dtb_entity_t prop, uint32_t *len_out) {
    if (!prop.valid) {
        if (len_out) {
            *len_out = 0;
        }
        return 0;
    } else {
        if (len_out) {
            *len_out = prop.prop_len;
        }
        return (void const *)(handle->struct_blk + prop.content);
    }
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
