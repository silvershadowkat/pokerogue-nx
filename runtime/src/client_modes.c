// SPDX-License-Identifier: AGPL-3.0-only
#include "client_modes.h"
#include <stdio.h>
const char *client_mode_name(pkr_client_mode mode){
    switch(mode){case PKR_ONLINE:return "online";case PKR_OFFLINE:return "offline";case PKR_ENHANCED:return "debug";default:return NULL;}
}
bool client_profile_path(char *out,size_t size,const char *root,pkr_client_mode mode){
    const char *name=client_mode_name(mode);if(!name||!root||!*root||!size)return false;
    int count=snprintf(out,size,"%s/profiles/%s",root,name);return count>=0&&(size_t)count<size;
}
bool client_can_import(pkr_client_mode mode){return mode==PKR_OFFLINE||mode==PKR_ENHANCED;}
bool client_can_connect_account(pkr_client_mode mode){return mode==PKR_ONLINE;}
bool client_can_load_enhancements(pkr_client_mode mode){return mode==PKR_ENHANCED;}
