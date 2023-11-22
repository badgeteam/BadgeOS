
#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void sys_temp_write(char const *data, size_t length);

size_t cstr_length(char const *const cstr) {
    char const *ptr = cstr;
    while (*ptr) ptr++;
    return ptr - cstr;
}

void print(char const *message) {
    sys_temp_write(message, cstr_length(message));
}



int main(int argc, char **argv) {
    print("Hello World from C!\n");
    return 0;
}
