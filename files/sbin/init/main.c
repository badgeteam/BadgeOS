
// SPDX-License-Identifier: MIT

#include "signal.h"
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
    syscall_proc_sighandler(SIGHUP, SIG_IGN);

    badge_err_t ec = {0};
    print("Hi, Ther.\n");

    syscall_sys_shutdown(false);

    return 0;
}
