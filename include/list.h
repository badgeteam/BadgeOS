
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A node of a doubly linked list structure. It contains no data, use
// `field_parent_ptr` from `meta.h` to obtain the containing structure.
typedef struct dlist_node_t {
    // Pointer to the next item in the linked list.
    struct dlist_node_t *next;

    // Pointer to the previous item in the linked list.
    struct dlist_node_t *previous;
} dlist_node_t;

// A doubly linekd list.
typedef struct dlist_t {
    // Current number of elements in the list.
    size_t len;
    // Pointer to the first node in the list or `NULL` if the list is empty.
    struct dlist_node_t *head;
    // Pointer to the last node in the list or `NULL` if the list is empty.
    struct dlist_node_t *tail;
} dlist_t;

// Initializer value for an empty list. Convenience macro for
// zero-initialization.
#define DLIST_EMPTY ((dlist_t){.len = 0, .head = NULL, .tail = NULL})

// Initializer value for a list node. Convenience macro for zero-initialization.
#define DLIST_NODE_EMPTY ((dlist_node_t){.next = NULL, .previous = NULL})

// Appends an item after the `tail` of the `list`.
// Both `list` and `node` must be non-`NULL`
void dlist_append(dlist_t *list, dlist_node_t *node);

// Prepends an item before the `head` of the `list`.
// Both `list` and `node` must be non-`NULL`
void dlist_prepend(dlist_t *list, dlist_node_t *node);

// Removes the `head` of the given `list`. Will return `NULL`
// if the list was empty.
//
// `list` must be non-`NULL`
dlist_node_t *dlist_pop_front(dlist_t *list);

// Removes the `tail` of the given `list`. Will return `NULL`
// if the list was empty.
//
// `list` must be non-`NULL`
dlist_node_t *dlist_pop_back(dlist_t *list);

// Checks if `list` contains the given `node`.
//
// Both `list` and `node` must be non-`NULL`
bool dlist_contains(const dlist_t *list, const dlist_node_t *node);
