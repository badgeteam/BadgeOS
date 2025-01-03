
// SPDX-License-Identifier: MIT

#include "filesystem/vfs_internal.h"

#include "assertions.h"
#include "badge_strings.h"
#include "filesystem/vfs_vtable.h"
#include "log.h"
#include "malloc.h"

#include <stdatomic.h>
