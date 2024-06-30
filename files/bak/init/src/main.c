
#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <signal.h>
#include <sys/wait.h>



size_t cstr_length(char const *const cstr) {
    char const *ptr = cstr;
    while (*ptr) ptr++;
    return ptr - cstr;
}

void print(char const *message) {
    syscall_temp_write(message, cstr_length(message));
}

bool volatile has_child_exploded = false;
void child_explosion(int signum) {
    int status;
    syscall_proc_waitpid(-1, &status, 0);
    if (WIFEXITED(status)) {
        print("I noticed, that the child exited.\n");
        has_child_exploded = true;
    }
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

    // Start child process, ok.
    char const *binary = "/sbin/test";
    int         pid    = syscall_proc_pcreate(binary, 1, &binary);
    if (pid <= 0)
        print("No pcreate :c\n");
    syscall_proc_sighandler(SIGCHLD, child_explosion);
    bool started = syscall_proc_pstart(pid);
    if (!started)
        print("No pstart :c\n");

    // Wait for child to exit.
    while (!has_child_exploded) {
        syscall_thread_yield();
    }

    // Gracefull shutdown.
    syscall_sys_shutdown(false);

    return 0;
}
