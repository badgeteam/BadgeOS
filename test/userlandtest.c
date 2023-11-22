
extern void exit(int code);

char const rodata[] = "Hello, World!";
char       data[]   = "This is a data";
char       bss[32];

void _start() {
    exit(1234);
}
