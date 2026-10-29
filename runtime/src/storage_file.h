// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <stdbool.h>
#include <stddef.h>

bool storage_write_file(const char *path, const char *data, size_t len);
