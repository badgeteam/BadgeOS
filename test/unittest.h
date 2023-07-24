#pragma once

#include "assertions.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define CONSTRUCTOR __attribute__((constructor))
#define TEST_DECL(X)                                                                                                   \
    static void unittest_##X(void) CONSTRUCTOR;                                                                        \
    static void unittest_##X(void)
#define TEST_DECL_WRAP(X) TEST_DECL(X)
#define TEST(...)         TEST_DECL_WRAP(__COUNTER__)

#define EXPECT(expected)             assert_always(expected)
#define EXPECT_EQL(expected, actual) assert_always((expected) == (actual))
#define EXPECT_TRUE(expected)        assert_always((expected) == true)
#define EXPECT_FALSE(expected)       assert_always((expected) == false)

int main() {
    return 0;
}

void kernel_assertion_failure(char const *assertion_msg) {
    fprintf(stderr, "ASSERTION FAILED: %s", assertion_msg);
    abort();
}
