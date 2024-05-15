
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <signal.h>
#include <syscall.h>

size_t cstr_length(char const *const cstr) {
    char const *ptr = cstr;
    while (*ptr) ptr++;
    return ptr - cstr;
}

void print(char const *message) {
    syscall_temp_write(message, cstr_length(message));
}

int main(int argc, char **argv) {
    print("This be the application 2.\n");
    return 0;
}
