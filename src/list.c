#include "list.h"

#include "assertions.h"

void dlist_append(dlist_t *const list, dlist_node_t *const node)
{
    assert_dev_drop(list != NULL);
    assert_dev_drop(node != NULL);

    if (list->tail == NULL)
    {
        assert_dev_drop(list->head == NULL);
        assert_dev_drop(list->len == 0);
        list->head = node;
    }

    *node = (dlist_node_t){
        .next = NULL,
        .previous = list->tail,
    };

    list->tail = node;
    list->len += 1;
}

void dlist_prepend(dlist_t *const list, dlist_node_t *const node)
{
    assert_dev_drop(list != NULL);
    assert_dev_drop(node != NULL);

    if (list->head == NULL)
    {
        assert_dev_drop(list->tail == NULL);
        assert_dev_drop(list->len == 0);
        list->tail = node;
    }

    *node = (dlist_node_t){
        .next = list->head,
        .previous = NULL,
    };

    list->head = node;
    list->len += 1;
}

dlist_node_t *dlist_pop_front(dlist_t *const list)
{
    assert_dev_drop(list != NULL);

    if (list->head != NULL)
    {
        assert_dev_drop(list->tail != NULL);
        assert_dev_drop(list->len > 0);

        if (list->head == list->tail)
        {
            list->tail = NULL;
        }
        dlist_node_t *const node = list->head;
        list->head = node->next;
        list->len -= 1;
        *node = DLIST_NODE_EMPTY;
        assert_dev_drop((list->head != NULL) == (list->tail != NULL));
        assert_dev_drop((list->head != NULL) == (list->len > 0));
        return node;
    }
    else
    {
        assert_dev_drop(list->tail == NULL);
        assert_dev_drop(list->len == 0);
        return NULL;
    }
}

dlist_node_t *dlist_pop_back(dlist_t *const list)
{
    assert_dev_drop(list != NULL);

    if (list->tail != NULL)
    {
        assert_dev_drop(list->head != NULL);
        assert_dev_drop(list->len > 0);

        if (list->tail == list->head)
        {
            list->head = NULL;
        }
        dlist_node_t *const node = list->tail;
        list->tail = node->previous;
        list->len -= 1;
        *node = DLIST_NODE_EMPTY;
        assert_dev_drop((list->head != NULL) == (list->tail != NULL));
        assert_dev_drop((list->head != NULL) == (list->len > 0));
        return node;
    }
    else
    {
        assert_dev_drop(list->head == NULL);
        assert_dev_drop(list->len == 0);
        return NULL;
    }
}

bool dlist_contains(dlist_t const *const list, dlist_node_t const *const node)
{
    assert_dev_drop(list != NULL);
    assert_dev_drop(node != NULL);

    dlist_node_t const *iter = list->head;
    while (iter != NULL)
    {
        if (iter == node)
        {
            return true;
        }
        iter = iter->next;
    }

    return false;
}
