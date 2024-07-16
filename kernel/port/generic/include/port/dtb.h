
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_strings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Minimum supported FDT version.
#define FDT_VERSION_MIN  16
// Maximum supported FDT version.
#define FDT_VERSION_MAX  16
// Magic value for FDT headers.
#define FDT_HEADER_MAGIC 0xd00dfeed

// FDT node types.
typedef enum {
    FDT_BEGIN_NODE = 1,
    FDT_END_NODE,
    FDT_PROP,
    FDT_NOP,
    FDT_END = 9,
} fdt_node_t;

// Struct attributes for FDT.
#define FDT_ATTR __attribute__((scalar_storage_order("big-endian")))

// FDT header struct.
typedef struct FDT_ATTR {
    // This field shall contain 0xd00dfeed.
    uint32_t magic;
    // The size of the entire FDT including this header.
    uint32_t totalsize;
    // Offset in bytes of the structure block.
    uint32_t off_dt_struct;
    // Offset in bytes of the string block.
    uint32_t off_dt_strings;
    // Offset in bytes of the memory reservation block.
    uint32_t off_mem_rsvmap;
    // FDT version.
    uint32_t version;
    // The oldest version with which this FDT is backwards-compatible.
    uint32_t last_comp_version;
    // Booting CPU ID.
    uint32_t boot_cpuid_phys;
    // Size of the string block.
    uint32_t size_dt_strings;
    // Size of the structure block.
    uint32_t size_dt_struct;
} dtb_header_t;

// FDT reserved memory.
typedef struct FDT_ATTR {
    // Base physical address.
    uint64_t paddr;
    // Size in bytes.
    uint64_t size;
} dtb_rsvmem_t;



// DTB phandle map entry.
typedef struct {
    // Phandle reference number.
    uint32_t    phandle;
    // Node content offset.
    uint32_t    content;
    // Node depth.
    uint8_t     depth;
    // Node name.
    char const *name;
} dtb_phandle_t;

// DTB parent map entry.
typedef struct {
    // Node content offset.
    uint32_t    content;
    // Node content length.
    uint32_t    length;
    // Node depth.
    uint8_t     depth;
    // Node name.
    char const *name;
} dtb_parent_t;

// DTB reading handle.
typedef struct {
    // DTB pointer.
    dtb_header_t  *dtb_hdr;
    // Resolved structure block address.
    uint32_t      *struct_blk;
    // Resolved strings block address.
    char          *string_blk;
    // Whether any errors were found in the DTB.
    // The DTB should not be read if there are any.
    bool           has_errors;
    // Number of phandles found.
    size_t         phandles_len;
    // Phandles and nodes found.
    dtb_phandle_t *phandles;
    // Number of parent map entries.
    size_t         parents_len;
    // Parent map.
    dtb_parent_t  *parents;
} dtb_handle_t;

// DTB struct / property handle.
typedef struct {
    // Is valid.
    bool        valid;
    // Is a node (and not property).
    bool        is_node;
    // How deep in the hierarchy this is; 0 is root-level.
    uint8_t     depth;
    // Word offet of the entity's content.
    uint32_t    content;
    // Length of the prop.
    uint32_t    prop_len;
    // Name pointer of the entity.
    char const *name;
} dtb_entity_t;



// Interpret the DTB header and prepare for reading.
dtb_handle_t dtb_open(void *dtb_ptr);

// Go to the first node or prop in the DTB.
dtb_entity_t dtb_root_node(dtb_handle_t *handle);

// Go to the first subnode in a node.
dtb_entity_t dtb_first_node(dtb_handle_t *handle, dtb_entity_t parent_node);
// Go to the first prop in a node.
dtb_entity_t dtb_first_prop(dtb_handle_t *handle, dtb_entity_t parent_node);

// Go to the next node on the same level of hierarchy.
dtb_entity_t dtb_next_node(dtb_handle_t *handle, dtb_entity_t from);
// Go to the next prop in this node.
dtb_entity_t dtb_next_prop(dtb_handle_t *handle, dtb_entity_t from);
// Walk to the next node or prop in the DTB.
dtb_entity_t dtb_walk_next(dtb_handle_t *handle, dtb_entity_t from);

// Get a node with a specific name.
dtb_entity_t dtb_get_node_l(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len);
// Get a prop with a specific name.
dtb_entity_t dtb_get_prop_l(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len);

// Find a node in the DTB.
dtb_entity_t dtb_find_node(dtb_handle_t *handle, char const *path);
// Find the immediate parent node of a node or prop.
dtb_entity_t dtb_find_parent(dtb_handle_t *handle, dtb_entity_t ent);
// Get a DTB node by phandle.
dtb_entity_t dtb_phandle_node(dtb_handle_t *handle, uint32_t phandle);


// Read a prop as a single unsigned number.
uintmax_t   dtb_prop_read_uint(dtb_handle_t *handle, dtb_entity_t prop);
// Read a prop as an array of cells.
uint32_t    dtb_prop_read_cell(dtb_handle_t *handle, dtb_entity_t prop, uint32_t cell_idx);
// Read an unsigned number from a prop formatted as cells.
uintmax_t   dtb_prop_read_cells(dtb_handle_t *handle, dtb_entity_t prop, uint32_t cell_idx, uint32_t cell_count);
// Get raw prop contents.
void const *dtb_prop_content(dtb_handle_t *handle, dtb_entity_t prop, uint32_t *len_out);

// Read a prop as a single unsigned number.
uintmax_t dtb_read_uint_l(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len);
// Read a prop as an array of cells.
uint32_t  dtb_read_cell_l(
     dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, size_t name_len, uint32_t cell_idx
 );
// Read an unsigned number from a prop formatted as cells.
uintmax_t dtb_read_cells_l(
    dtb_handle_t *handle,
    dtb_entity_t  parent_node,
    char const   *name,
    size_t        name_len,
    uint32_t      cell_idx,
    uint32_t      cell_count
);



// Read a prop as a single unsigned number.
static inline uintmax_t dtb_read_uint(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name) {
    return dtb_read_uint_l(handle, parent_node, name, cstr_length(name));
}

// Read a prop as an array of cells.
static inline uint32_t
    dtb_read_cell(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, uint32_t cell_idx) {
    return dtb_read_cell_l(handle, parent_node, name, cstr_length(name), cell_idx);
}

// Read an unsigned number from a prop formatted as cells.
static inline uintmax_t dtb_read_cells(
    dtb_handle_t *handle, dtb_entity_t parent_node, char const *name, uint32_t cell_idx, uint32_t cell_count
) {
    return dtb_read_cells_l(handle, parent_node, name, cstr_length(name), cell_idx, cell_count);
}


// Get a node with a specific name.
static inline dtb_entity_t dtb_get_node(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name) {
    return dtb_get_node_l(handle, parent_node, name, __builtin_strlen(name));
}

// Get a prop with a specific name.
static inline dtb_entity_t dtb_get_prop(dtb_handle_t *handle, dtb_entity_t parent_node, char const *name) {
    return dtb_get_prop_l(handle, parent_node, name, __builtin_strlen(name));
}
