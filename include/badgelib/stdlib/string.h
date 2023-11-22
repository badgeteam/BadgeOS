
// SPDX-License-Identifier: MIT

// Replacement for stdlib's <string.h>

#pragma once

#include "badge_strings.h"

#include <stddef.h>


void  *memchr(void const *, int, size_t);
int    memcmp(void const *, void const *, size_t);
void  *memcpy(void *__restrict, void const *__restrict, size_t);
void  *memmove(void *, void const *, size_t);
void  *memset(void *, int, size_t);
char  *strcat(char *__restrict, char const *__restrict);
char  *strchr(char const *, int);
int    strcmp(char const *, char const *);
char  *strcpy(char *__restrict, char const *__restrict);
size_t strlen(char const *);
char  *strncat(char *__restrict, char const *__restrict, size_t);
int    strncmp(char const *, char const *, size_t);
char  *strncpy(char *__restrict, char const *__restrict, size_t);
char  *strrchr(char const *, int);
