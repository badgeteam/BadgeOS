
#include "badge_err.h"

// Names for badge_eloc_t.
char const *badge_eloc_name[_badge_eloc_num] = {
    "ELOC_UNKNOWN",
    "ELOC_GPIO",
    "ELOC_I2C",
    "ELOC_UART",
    "ELOC_SPI",
    "ELOC_DISP",
    "ELOC_IP",
    "ELOC_WIFI",
    "ELOC_BT",
    "ELOC_RADIO",
    "ELOC_EXTMEM",
    "ELOC_INTMEM",
    "ELOC_THREADS",
    "ELOC_FILESYSTEM",
    "ELOC_BLKDEV",
    "ELOC_PROCESS",
};

// Names for badge_ecause_t.
char const *badge_ecause_name[_badge_ecause_num] = {
    "ECAUSE_OK",       "ECAUSE_UNKNOWN",     "ECAUSE_PARAM",   "ECAUSE_RANGE",      "ECAUSE_TOOLONG", "ECAUSE_TOOSHORT",
    "ECAUSE_NOMEM",    "ECAUSE_PERM",        "ECAUSE_TIMEOUT", "ECAUSE_UNEXPECTED", "ECAUSE_NOENT",   "ECAUSE_FORMAT",
    "ECAUSE_RESET",    "ECAUSE_REFUSED",     "ECAUSE_INUSE",   "ECAUSE_NOTCONFIG",  "ECAUSE_UNAVAIL", "ECAUSE_NOROUTE",
    "ECAUSE_BREAK",    "ECAUSE_UNSUPPORTED", "ECAUSE_NOTACK",  "ECAUSE_CANCELLED",  "ECAUSE_ILLEGAL", "ECAUSE_READONLY",
    "ECAUSE_NOTFOUND", "ECAUSE_IS_DIR",      "ECAUSE_IS_FILE", "ECAUSE_EXISTS",     "ECAUSE_NOSPACE", "ECAUSE_NOTEMPTY",
    "ECAUSE_STATE",
};



// Get the name of a badge_eloc_t.
char const *badge_eloc_get_name(badge_eloc_t eloc) {
    return (eloc < 0 || eloc >= _badge_eloc_num) ? "unknown location" : badge_eloc_name[eloc];
}

// Get the name of a badge_ecause_t.
char const *badge_ecause_get_name(badge_ecause_t ecause) {
    return (ecause < 0 || ecause >= _badge_ecause_num) ? "unknown cause" : badge_ecause_name[ecause];
}
