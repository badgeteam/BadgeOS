
#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



size_t cstr_length(char const *const cstr) {
    char const *ptr = cstr;
    while (*ptr) ptr++;
    return ptr - cstr;
}

void print(char const *message) {
    syscall_temp_write(message, cstr_length(message));
}



int main(int argc, char **argv) {
    char buf[128];
    print("Hello World from C???\n");
    int fd = syscall_fs_open("/etc/motd", 0, OFLAGS_READONLY);
    if (fd < 0)
        print("No FD :c\n");
    long count = syscall_fs_read(fd, buf, sizeof(buf));
    syscall_fs_close(fd);
    if (count > 0)
        syscall_temp_write(buf, count);
    else
        print("No read :c\n");
    int volatile *mem = syscall_mem_alloc(0, 32, 0, MEMFLAGS_RW);
    syscall_mem_dealloc(mem);
    return 0;
}
