
// SPDX-License-Identifier: MIT

#include "list.h"

#include "assertions.h"

void dlist_append(dlist_t *const list, dlist_node_t *const node) {
    assert_dev_drop(list != NULL);
    assert_dev_drop(node != NULL);
    assert_dev_drop(!dlist_contains(list, node));

    *node = (dlist_node_t){
        .next     = NULL,
        .previous = list->tail,
    };

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        assert_dev_drop(list->head == NULL);
        assert_dev_drop(list->len == 0);
        list->head = node;
    }
    list->tail  = node;
    list->len  += 1;
}

void dlist_prepend(dlist_t *const list, dlist_node_t *const node) {
    assert_dev_drop(list != NULL);
    assert_dev_drop(node != NULL);
    assert_dev_drop(!dlist_contains(list, node));

    *node = (dlist_node_t){
        .next     = list->head,
        .previous = NULL,
    };

    if (list->head != NULL) {
        list->head->previous = node;
    } else {
        assert_dev_drop(list->tail == NULL);
        assert_dev_drop(list->len == 0);
        list->tail = node;
    }
    list->head  = node;
    list->len  += 1;
}

#include "rawprint.h"

dlist_node_t *dlist_pop_front(dlist_t *const list) {
    assert_dev_drop(list != NULL);

    if (list->head != NULL) {
        assert_dev_drop(list->tail != NULL);
        assert_dev_drop(list->len > 0);

        dlist_node_t *const node = list->head;

        if (node->next != NULL) {
            node->next->previous = node->previous;
        }

        list->len  -= 1;
        list->head  = node->next;
        if (list->head == NULL) {
            list->tail = NULL;
        }

        assert_dev_drop((list->head != NULL) == (list->tail != NULL));
        assert_dev_drop((list->head != NULL) == (list->len > 0));

        *node = DLIST_NODE_EMPTY;
        return node;
    } else {
        assert_dev_drop(list->tail == NULL);
        assert_dev_drop(list->len == 0);
        return NULL;
    }
}

dlist_node_t *dlist_pop_back(dlist_t *const list) {
    assert_dev_drop(list != NULL);

    if (list->tail != NULL) {
        assert_dev_drop(list->head != NULL);
        assert_dev_drop(list->len > 0);

        dlist_node_t *const node = list->tail;

        if (node->previous != NULL) {
            node->previous->next = node->next;
        }

        list->len  -= 1;
        list->tail  = node->previous;
        if (list->tail == NULL) {
            list->head = NULL;
        }

        assert_dev_drop((list->head != NULL) == (list->tail != NULL));
        assert_dev_drop((list->head != NULL) == (list->len > 0));

        *node = DLIST_NODE_EMPTY;
        return node;
    } else {
        assert_dev_drop(list->head == NULL);
        assert_dev_drop(list->len == 0);
        return NULL;
    }
}

bool dlist_contains(dlist_t const *const list, dlist_node_t const *const node) {
    assert_dev_drop(list != NULL);
    assert_dev_drop(node != NULL);

    dlist_node_t const *iter = list->head;
    while (iter != NULL) {
        if (iter == node) {
            return true;
        }
        iter = iter->next;
    }

    return false;
}

void dlist_remove(dlist_t *const list, dlist_node_t *const node) {
    assert_dev_drop(dlist_contains(list, node) || ((node->next == NULL) && (node->previous == NULL)));

    bool decrement = false;
    if (node->previous != NULL) {
        node->previous->next = node->next;
        decrement            = true;
    }
    if (node->next != NULL) {
        node->next->previous = node->previous;
        decrement            = true;
    }

    if (node == list->head) {
        list->head = node->next;
        decrement  = true;
    }
    if (node == list->tail) {
        list->tail = node->previous;
        decrement  = true;
    }

    if (decrement) {
        list->len -= 1;
    }
    *node = DLIST_NODE_EMPTY;
}
