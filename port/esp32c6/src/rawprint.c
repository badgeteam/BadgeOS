
// SPDX-License-Identifier: MIT

#include "rawprint.h"

#include "num_to_str.h"
#include "port/hardware.h"
#include "time.h"

#include <stddef.h>

char const hextab[] = "0123456789ABCDEF";

// Simple printer with specified length.
void rawprint_substr(char const *msg, size_t length) {
    if (!msg)
        return;
    char prev = 0;
    while (length--) {
        if (*msg == '\r') {
            rawputc('\r');
            rawputc('\n');
        } else if (*msg == '\n') {
            if (prev != '\r') {
                rawputc('\r');
                rawputc('\n');
            }
        } else {
            rawputc(*msg);
        }
        prev = *msg;
        msg++;
    }
}

// Simple printer.
void rawprint(char const *msg) {
    if (!msg)
        return;
    char prev = 0;
    while (*msg) {
        if (*msg == '\r') {
            rawputc('\r');
            rawputc('\n');
        } else if (*msg == '\n') {
            if (prev != '\r') {
                rawputc('\r');
                rawputc('\n');
            }
        } else {
            rawputc(*msg);
        }
        prev = *msg;
        msg++;
    }
}

// Simple printer.
void rawputc(char msg) {
    while (!(READ_REG(USB_JTAG_BASE + 4) & 2))
        ;
    WRITE_REG(USB_JTAG_BASE, msg);
    WRITE_REG(UART0_BASE, msg);
}

// Bin 2 hex printer.
void rawprinthex(uint64_t val, int digits) {
    for (; digits > 0; digits--) {
        rawputc(hextab[(val >> (digits * 4 - 4)) & 15]);
    }
}

// Bin 2 dec printer.
void rawprintudec(uint64_t val, int digits) {
    char   buf[20];
    size_t buf_digits = uint_to_cstr_packed(val, buf, sizeof(buf));
    if (digits < (int)buf_digits)
        digits = (int)buf_digits;
    else if (digits > (int)sizeof(buf))
        digits = sizeof(buf);
    rawprint_substr(buf, digits);
}

// Bin 2 dec printer.
void rawprintdec(int64_t val, int digits) {
    if (val < 0) {
        rawputc('-');
        val = -val;
    }
    rawprintudec(val, digits);
}

// Current uptime printer for logging.
void rawprintuptime() {
    char   buf[20];
    size_t digits = num_uint_to_str(time_us() / 1000, buf);
    if (digits < 8)
        digits = 8;

    rawputc('[');
    rawprint_substr(buf + 20 - digits, digits - 3);
    rawputc('.');
    rawprint_substr(buf + 17, 3);
    rawputc(']');
}
