
// SPDX-License-Identifier: MIT

#pragma once

#include "log.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t cause;
    uint32_t location;
    uint32_t padding[2];
} badge_err_t;

typedef enum {
    // Unknown location or no appropriate badge_eloc_t value
    ELOC_UNKNOWN = 0,
    // Digital and analog GPIO pins not used by serial interfaces
    ELOC_GPIO,
    // I²C interface
    ELOC_I2C,
    // UART interface
    ELOC_UART,
    // SPI interface
    ELOC_SPI,
    // Display
    ELOC_DISP,
    // Internet protocol
    ELOC_IP,
    // WiFi protocol and/or hardware
    ELOC_WIFI,
    // BlueTooth protocol and/or hardware
    ELOC_BT,
    // Radio hardware
    ELOC_RADIO,
    // External memory and/or controller
    ELOC_EXTMEM,
    // Internal memory
    ELOC_INTMEM,
    // Multithreading and process control
    ELOC_THREADS,
    // Filesystem driver
    ELOC_FILESYSTEM,
    // Block device driver
    ELOC_BLKDEV,
} badge_eloc_t;

typedef enum {
    // Not an error.
    ECAUSE_OK      = 0,
    // Unknown error or no appropriate badge_ecause_t value
    ECAUSE_UNKNOWN = 1,
    // Parameter missing or invalid; generic invalid parameter error code
    ECAUSE_PARAM,
    // Parameter out of range
    ECAUSE_RANGE,
    // String or buffer too long
    ECAUSE_TOOLONG,
    // String of buffer too short
    ECAUSE_TOOSHORT,
    // Out of memory or not enough memory
    ECAUSE_NOMEM,
    // Insufficient privileges; permission denied
    ECAUSE_PERM,
    // No response or operation took too long
    ECAUSE_TIMEOUT,
    // The function encountered unexpected behaviour and doesn't know how to continue
    ECAUSE_UNEXPECTED,
    // The requested file/port/device/etc was not found
    ECAUSE_NOENT,
    // The given file does not have the expected format
    ECAUSE_FORMAT,
    // A network connection was reset by the peer
    ECAUSE_RESET,
    // A network connection couldn't be established
    ECAUSE_REFUSED,
    // Requested resource is already in use
    ECAUSE_INUSE,
    // Requested resource is available but not configured
    ECAUSE_NOTCONFIG,
    // Requested resource is unavailable
    ECAUSE_UNAVAIL,
    // Could not find a route to the requested host
    ECAUSE_NOROUTE,
    // A communication channel received a break condition
    ECAUSE_BREAK,
    // The requested operation isn't supported by the target
    ECAUSE_UNSUPPORTED,
    // The request was not acknowledged
    ECAUSE_NOTACK,
    // The operation was cancelled
    ECAUSE_CANCELLED,
    // The operation is illegal in the current state
    ECAUSE_ILLEGAL,
    // Write operation attempted on read-only resource
    ECAUSE_READONLY,
    // The requested resource was not found
    ECAUSE_NOTFOUND,
    // The requested path is a directory
    ECAUSE_IS_DIR,
    // The requested path is not a directory
    ECAUSE_IS_FILE,
    // The resource already exists
    ECAUSE_EXISTS,
    // The resource is out of space
    ECAUSE_NOSPACE,
    // The resource is not empty.
    ECAUSE_NOTEMPTY,
} badge_ecause_t;


// Sets `ec` to the given `location` and `cause` values if `ec` is not `NULL`.
// `ec` must be a variable name.
#define badge_err_set(ec, location_value, cause_value)                                                                 \
    do {                                                                                                               \
        if ((ec) != NULL) {                                                                                            \
            if (cause_value) {                                                                                         \
                uint32_t pc;                                                                                           \
                asm("auipc %0, 0" : "=r"(pc));                                                                         \
                logkf(LOG_DEBUG, "ELOC=%{d}, ECAUSE=%{d}, PC=0x%{size;x}", location_value, cause_value, pc);           \
            }                                                                                                          \
            (ec)->location = location_value;                                                                           \
            (ec)->cause    = cause_value;                                                                              \
        }                                                                                                              \
    } while (false)

// Sets `ec` to a "no error"-value.
// `ec` must be a variable name.
#define badge_err_set_ok(ec) badge_err_set(ec, 0, 0)

// Returns a non-zero value if `ec` is `NULL` or `ec` contains a non-error value.
// `ec` must be a variable name.
#define badge_err_is_ok(ec) (((ec) == NULL) || (((ec)->cause == 0) && ((ec)->location == 0)))
