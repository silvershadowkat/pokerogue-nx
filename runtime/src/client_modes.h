// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef enum { PKR_ONLINE, PKR_OFFLINE, PKR_ENHANCED } pkr_client_mode;
const char *client_mode_name(pkr_client_mode mode);
bool client_profile_path(char *out,size_t size,const char *root,pkr_client_mode mode);
bool client_can_import(pkr_client_mode mode);
bool client_can_connect_account(pkr_client_mode mode);
bool client_can_load_enhancements(pkr_client_mode mode);
