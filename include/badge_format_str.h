
// SPDX-License-Identifier: MIT

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Callback function used by format_str.
typedef bool (*format_str_cb_t)(char const *msg, size_t len, void *cookie);

// Format a string and output characters via callback.
//
// Format specifiers:
//   %{cs} - Print a C-String.
//   %{<type>d} - Print decimal.
//   %{<type>x} - Print unsigned hexadecimal.
//   %{<type>o} - Print octal.
//   %{<type>q} - Print octal.
//   %{<type>c} - Print a single character.
//   %{}        - Print a literal % character.
//
// Type speciciers:
//   (omitted)    - signed int
//   llong;       - signed long long
//   long;        - signed long
//   int;         - signed nt
//   short;       - signed short int
//   char;        - signed char
//   ullong;      - unsigned long long
//   ulong;       - unsigned long
//   uint;        - unsigned nt
//   ushort;      - unsigned short int
//   uchar;       - unsigned char
//   ui64; / i64; - uint64_t / int64_t
//   ui32; / i32; - uint32_t / int32_t
//   ui16; / i16; - uint16_t / int16_t
//   ui8;  / i8;  - uint8_t  / int8_t
//
// If one of the format specifiers is misformatted, the remaining text is output verbatim.
bool format_str_va(char const *msg, format_str_cb_t callback, void *cookie, va_list vararg);
