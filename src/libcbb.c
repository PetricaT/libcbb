/*
 * libcbb - compatibility shim.
 *
 * This file is provided so existing build systems that already expect to
 * compile `src/libcbb.c` keep working without modification. The library is
 * now distributed as an amalgamated single header; the recommended way to
 * use libcbb is:
 *
 *   #define CBB_IMPLEMENTATION
 *   #include "libcbb.h"
 *
 * in exactly one of your .c files. See include/libcbb.h for full details.
 *
 * MIT License - Copyright (c) 2026 Petrica
 */

#define CBB_IMPLEMENTATION
#include "libcbb.h"