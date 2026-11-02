// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <stdbool.h>
#include <stddef.h>

bool storage_write_file(const char *path, const char *data, size_t len);
// The caller validates staged content before installing it. Keeps .bak on Switch.
bool storage_install_file(const char *staged, const char *path);
bool storage_recover_previous(const char *path);
