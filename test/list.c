#include "list.h"

#include "assertions.h"
#include "unittest.h"

static void expect_list_eql(dlist_t list, dlist_node_t const *const *nodes, size_t nodes_size) {
    size_t node_count = nodes_size / sizeof(dlist_node_t *);

    for (size_t i = 0; i < node_count; i++) {
        EXPECT_TRUE(dlist_contains(&list, nodes[i]));
    }

    EXPECT_EQL(node_count, list.len);
    switch (node_count) {
        case 0:
            EXPECT_EQL(NULL, list.head);
            EXPECT_EQL(NULL, list.tail);
            break;

        default: {
            EXPECT_EQL(nodes[0], list.head);
            EXPECT_EQL(nodes[node_count - 1], list.tail);

            dlist_node_t const *iter = list.head;
            for (size_t i = 0; i < node_count; i++, iter = iter->next) {
                bool first = (i == 0);
                bool last  = (i == (node_count - 1));

                EXPECT(iter != NULL);

                EXPECT_EQL(first ? NULL : nodes[i - 1], iter->previous);
                EXPECT_EQL(last ? NULL : nodes[i + 1], iter->next);
            }
            break;
        }
    }
}
#define EXPECT_LIST_EQL(list, ...)                                                                                     \
    expect_list_eql(list, (dlist_node_t const *const[]){__VA_ARGS__}, sizeof((dlist_node_t *[]){__VA_ARGS__}));

TEST(DLIST_EMPTY) {

    dlist_t list = DLIST_EMPTY;

    EXPECT_EQL(0, list.len);
    EXPECT_EQL(NULL, list.head);
    EXPECT_EQL(NULL, list.tail);
}

TEST(dlist_append) {
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;

    EXPECT_EQL(0, list.len);
    EXPECT_EQL(NULL, list.head);
    EXPECT_EQL(NULL, list.tail);

    EXPECT_EQL(NULL, node1.next);
    EXPECT_EQL(NULL, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(NULL, node1.previous);
    EXPECT_EQL(NULL, node2.previous);
    EXPECT_EQL(NULL, node3.previous);

    dlist_append(&list, &node1);

    EXPECT_EQL(1, list.len);
    EXPECT_EQL(&node1, list.head);
    EXPECT_EQL(&node1, list.tail);

    EXPECT_EQL(NULL, node1.next);
    EXPECT_EQL(NULL, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(NULL, node1.previous);
    EXPECT_EQL(NULL, node2.previous);
    EXPECT_EQL(NULL, node3.previous);

    dlist_append(&list, &node2);

    EXPECT_EQL(2, list.len);
    EXPECT_EQL(&node1, list.head);
    EXPECT_EQL(&node2, list.tail);

    EXPECT_EQL(&node2, node1.next);
    EXPECT_EQL(NULL, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(NULL, node1.previous);
    EXPECT_EQL(&node1, node2.previous);
    EXPECT_EQL(NULL, node3.previous);

    dlist_append(&list, &node3);

    EXPECT_EQL(3, list.len);
    EXPECT_EQL(&node1, list.head);
    EXPECT_EQL(&node3, list.tail);

    EXPECT_EQL(&node2, node1.next);
    EXPECT_EQL(&node3, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(NULL, node1.previous);
    EXPECT_EQL(&node1, node2.previous);
    EXPECT_EQL(&node2, node3.previous);
}

TEST(dlist_prepend) {
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;

    EXPECT_EQL(0, list.len);
    EXPECT_EQL(NULL, list.head);
    EXPECT_EQL(NULL, list.tail);

    EXPECT_EQL(NULL, node1.next);
    EXPECT_EQL(NULL, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(NULL, node1.previous);
    EXPECT_EQL(NULL, node2.previous);
    EXPECT_EQL(NULL, node3.previous);

    dlist_prepend(&list, &node1);

    EXPECT_EQL(1, list.len);
    EXPECT_EQL(&node1, list.head);
    EXPECT_EQL(&node1, list.tail);

    EXPECT_EQL(NULL, node1.next);
    EXPECT_EQL(NULL, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(NULL, node1.previous);
    EXPECT_EQL(NULL, node2.previous);
    EXPECT_EQL(NULL, node3.previous);

    dlist_prepend(&list, &node2);

    EXPECT_EQL(2, list.len);
    EXPECT_EQL(&node2, list.head);
    EXPECT_EQL(&node1, list.tail);

    EXPECT_EQL(NULL, node1.next);
    EXPECT_EQL(&node1, node2.next);
    EXPECT_EQL(NULL, node3.next);

    EXPECT_EQL(&node2, node1.previous);
    EXPECT_EQL(NULL, node2.previous);
    EXPECT_EQL(NULL, node3.previous);

    dlist_prepend(&list, &node3);

    EXPECT_EQL(3, list.len);
    EXPECT_EQL(&node3, list.head);
    EXPECT_EQL(&node1, list.tail);

    EXPECT_EQL(NULL, node1.next);
    EXPECT_EQL(&node1, node2.next);
    EXPECT_EQL(&node2, node3.next);

    EXPECT_EQL(&node2, node1.previous);
    EXPECT_EQL(&node3, node2.previous);
    EXPECT_EQL(NULL, node3.previous);
}

TEST(dlist_contains) {
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;

    EXPECT_FALSE(dlist_contains(&list, &node1));
    EXPECT_FALSE(dlist_contains(&list, &node2));
    EXPECT_FALSE(dlist_contains(&list, &node3));

    dlist_append(&list, &node1);

    EXPECT_TRUE(dlist_contains(&list, &node1));
    EXPECT_FALSE(dlist_contains(&list, &node2));
    EXPECT_FALSE(dlist_contains(&list, &node3));

    dlist_append(&list, &node2);

    EXPECT_TRUE(dlist_contains(&list, &node1));
    EXPECT_TRUE(dlist_contains(&list, &node2));
    EXPECT_FALSE(dlist_contains(&list, &node3));

    dlist_append(&list, &node3);

    EXPECT_TRUE(dlist_contains(&list, &node1));
    EXPECT_TRUE(dlist_contains(&list, &node2));
    EXPECT_TRUE(dlist_contains(&list, &node3));
}

TEST(dlist_remove) {
    // Tests for removing list mid
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;

    dlist_append(&list, &node1);
    dlist_append(&list, &node2);
    dlist_append(&list, &node3);

    EXPECT_LIST_EQL(list, &node1, &node2, &node3);

    dlist_remove(&list, &node2);

    EXPECT_LIST_EQL(list, &node1, &node3);

    dlist_remove(&list, &node1);

    EXPECT_LIST_EQL(list, &node3);

    dlist_remove(&list, &node3);

    EXPECT_LIST_EQL(list, );
}


TEST(dlist_remove) {
    // Tests for removing list HEAD
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;

    dlist_append(&list, &node1);
    dlist_append(&list, &node2);
    dlist_append(&list, &node3);

    EXPECT_LIST_EQL(list, &node1, &node2, &node3);

    dlist_remove(&list, &node1);

    EXPECT_LIST_EQL(list, &node2, &node3);
}

TEST(dlist_remove) {
    // Tests for removing list TAIL
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;

    dlist_append(&list, &node1);
    dlist_append(&list, &node2);
    dlist_append(&list, &node3);

    EXPECT_LIST_EQL(list, &node1, &node2, &node3);

    dlist_remove(&list, &node3);

    EXPECT_LIST_EQL(list, &node1, &node2);
}

TEST(dlist_pop_front) {
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;
    dlist_node_t node4 = DLIST_NODE_EMPTY;

    dlist_append(&list, &node1);
    dlist_append(&list, &node2);
    dlist_append(&list, &node3);
    dlist_append(&list, &node4);

    EXPECT_LIST_EQL(list, &node1, &node2, &node3, &node4);

    EXPECT_EQL(&node1, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, &node2, &node3, &node4);

    EXPECT_EQL(&node2, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, &node3, &node4);

    EXPECT_EQL(&node3, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, &node4);

    EXPECT_EQL(&node4, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, );

    EXPECT_EQL(NULL, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, );

    EXPECT_EQL(NULL, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, );
}

TEST(dlist_pop_front) {
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;
    dlist_node_t node4 = DLIST_NODE_EMPTY;

    dlist_append(&list, &node1);
    dlist_append(&list, &node2);
    dlist_append(&list, &node3);
    dlist_append(&list, &node4);

    EXPECT_LIST_EQL(list, &node1, &node2, &node3, &node4);

    EXPECT_EQL(&node4, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, &node1, &node2, &node3);

    EXPECT_EQL(&node3, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, &node1, &node2);

    EXPECT_EQL(&node2, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, &node1);

    EXPECT_EQL(&node1, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, );

    EXPECT_EQL(NULL, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, );

    EXPECT_EQL(NULL, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, );
}

TEST(dlist_pop_front, dlist_pop_front) {
    dlist_t      list  = DLIST_EMPTY;

    dlist_node_t node1 = DLIST_NODE_EMPTY;
    dlist_node_t node2 = DLIST_NODE_EMPTY;
    dlist_node_t node3 = DLIST_NODE_EMPTY;
    dlist_node_t node4 = DLIST_NODE_EMPTY;
    dlist_node_t node5 = DLIST_NODE_EMPTY;

    dlist_append(&list, &node1);
    dlist_append(&list, &node2);
    dlist_append(&list, &node3);
    dlist_append(&list, &node4);
    dlist_append(&list, &node5);

    EXPECT_LIST_EQL(list, &node1, &node2, &node3, &node4, &node5);

    EXPECT_EQL(&node5, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, &node1, &node2, &node3, &node4);

    EXPECT_EQL(&node1, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, &node2, &node3, &node4);

    EXPECT_EQL(&node4, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, &node2, &node3);

    EXPECT_EQL(&node2, dlist_pop_front(&list));

    EXPECT_LIST_EQL(list, &node3);

    EXPECT_EQL(&node3, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, );

    EXPECT_EQL(NULL, dlist_pop_back(&list));

    EXPECT_LIST_EQL(list, );
}
