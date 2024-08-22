
// SPDX-License-Identifier: MIT

#include "syscall.h"



size_t strlen(char const *cstr) {
    char const *pre = cstr;
    while (*cstr) cstr++;
    return cstr - pre;
}

void print(char const *cstr) {
    syscall_temp_write(cstr, strlen(cstr));
}

char const hextab[] = "0123456789ABCDEF";

int main() {
    badge_err_t ec = {0};
    print("Hi, Ther.\n");

    syscall_sys_shutdown(false);
    return 0;
}
