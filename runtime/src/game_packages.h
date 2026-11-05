// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct { char version[64], base_version[64]; } debug_package_info;
bool game_version_valid(const char *version);
bool game_base_path(char *out,size_t n,const char *root,const char *version);
bool game_debug_info(const char *path,debug_package_info *info);
bool game_find_offline(char *path,size_t path_n,char *version,size_t version_n,const char *root);
bool game_find_debug(char *path,size_t path_n,char *base,size_t base_n,debug_package_info *info,const char *root);
