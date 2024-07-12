
// SPDX-License-Identifier: MIT

#include "port/dtb.h"

#include "badge_strings.h"
#include "log.h"



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
    while (handle->struct_blk[i] == FDT_NOP) {
        i++;
    }
    if (handle->struct_blk[i] != FDT_BEGIN_NODE) {
        return (dtb_entity_t){0};
    }
    size_t name_len = cstr_length((char *)(handle->struct_blk + i + 1));
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = 0,
        .content = i + 1 + (name_len + 3) / 4,
        .name    = (char *)(handle->struct_blk + i + 1),
    };
}


// Go to the first subnode in a node.
dtb_entity_t dtb_first_node(dtb_handle_t *handle, dtb_entity_t parent_node) {
    uint32_t i = parent_node.content;
    // Skip NOPs.
    while (handle->struct_blk[i] == FDT_NOP) {
        i++;
    }
    // Skip props.
    while (handle->struct_blk[i] == FDT_PROP) {
        i += 3 + (handle->struct_blk[i] + 3) / 4;
        while (handle->struct_blk[i] == FDT_NOP) {
            i++;
        }
    }
    // This should be a node.
    if (handle->struct_blk[i] != FDT_BEGIN_NODE) {
        return (dtb_entity_t){0};
    }
    size_t name_len = cstr_length((char *)(handle->struct_blk + i + 1));
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = parent_node.depth + 1,
        .content = i + 1 + (name_len + 3) / 4,
        .name    = (char *)(handle->struct_blk + i + 1),
    };
}

// Go to the first prop in a node.
dtb_entity_t dtb_first_prop(dtb_handle_t *handle, dtb_entity_t parent_node) {
    uint32_t i = 0;
    while (handle->struct_blk[i] == FDT_NOP) {
        i++;
    }
    if (handle->struct_blk[i] != FDT_PROP) {
        return (dtb_entity_t){0};
    }
    return (dtb_entity_t){
        .valid    = true,
        .is_node  = false,
        .depth    = parent_node.depth + 1,
        .content  = i + 3,
        .prop_len = handle->struct_blk[i + 1],
        .name     = handle->string_blk + handle->struct_blk[i + 2],
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
        while (handle->struct_blk[i] == FDT_NOP) {
            i++;
        }
        if (handle->struct_blk[i] == FDT_PROP) {
            i += 3 + (handle->struct_blk[i + 1] + 3) / 4;
        } else if (handle->struct_blk[i] == FDT_BEGIN_NODE) {
            size_t len  = cstr_length((char *)(handle->struct_blk + i + 1));
            i          += 1 + (len + 3) / 4;
        } else {
            return (dtb_entity_t){0};
        }
    }
    if (handle->struct_blk[i] != FDT_BEGIN_NODE) {
        return (dtb_entity_t){0};
    }
    size_t name_len = cstr_length((char *)(handle->struct_blk + i));
    return (dtb_entity_t){
        .valid   = true,
        .is_node = true,
        .depth   = from.depth,
        .content = i + (name_len + 3) / 4,
        .name    = (char *)(handle->struct_blk + i),
    };
}

// Go to the next prop in this node.
dtb_entity_t dtb_next_prop(dtb_handle_t *handle, dtb_entity_t from) {
    if (!from.valid || from.is_node) {
        return (dtb_entity_t){0};
    }
    uint32_t i = from.content + (from.prop_len + 3) / 4;
    while (handle->struct_blk[i] == FDT_NOP) {
        i++;
    }
    if (handle->struct_blk[i] != FDT_PROP) {
        return (dtb_entity_t){0};
    }
    return (dtb_entity_t){
        .valid    = true,
        .is_node  = false,
        .depth    = from.depth,
        .content  = i + 3,
        .prop_len = handle->struct_blk[i + 1],
        .name     = handle->string_blk + handle->struct_blk[i + 2],
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
