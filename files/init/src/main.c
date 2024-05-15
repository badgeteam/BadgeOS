
#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <signal.h>



size_t cstr_length(char const *const cstr) {
    char const *ptr = cstr;
    while (*ptr) ptr++;
    return ptr - cstr;
}

void print(char const *message) {
    syscall_temp_write(message, cstr_length(message));
}

void sigtrap_handler(int signum) {
    print("Whoops, SIGTRAP ;)\n");
    syscall_proc_exit(2);
}



int main(int argc, char **argv) {
    char buf[128];
    print("Hello World from C???\n");

    // Read test.
    int fd = syscall_fs_open("/etc/motd", 0, OFLAGS_READONLY);
    if (fd < 0)
        print("No FD :c\n");
    long count = syscall_fs_read(fd, buf, sizeof(buf));
    syscall_fs_close(fd);
    if (count > 0)
        syscall_temp_write(buf, count);
    else
        print("No read :c\n");

    // Memory allocation test.
    int volatile *mem = syscall_mem_alloc(0, 32, 0, MEMFLAGS_RW);
    *mem              = 3;
    syscall_mem_dealloc(mem);

    // Start child process, ok.
    char const *binary = "/sbin/test";
    int         pid    = syscall_proc_pcreate(binary, 1, &binary);
    if (pid < 0)
        print("No pcreate :c\n");
    bool started = syscall_proc_pstart(pid);
    if (!started)
        print("No pstart :c\n");

    // SIGTRAP test.
    print("Time for trolling\n");
    syscall_proc_sighandler(SIGTRAP, sigtrap_handler);
    asm("ebreak");

    return 0;
}
