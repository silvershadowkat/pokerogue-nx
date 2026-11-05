// SPDX-License-Identifier: AGPL-3.0-only
#include "client_modes.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
bool client_archive_allowed(pkr_client_mode mode,bool debug_archive){return (mode==PKR_OFFLINE&&!debug_archive)||(mode==PKR_ENHANCED&&debug_archive);}
bool client_can_fetch_public(pkr_client_mode mode,const char *method,const char *url){
    static const char archive[]="https://raw.githubusercontent.com/silvershadowkat/pokerogue-offline/seed/docs/daily-seeds.json";
    if(mode!=PKR_ENHANCED||!method||strcmp(method,"GET")||!url||strncmp(url,archive,sizeof archive-1))return false;
    const char *p=url+sizeof archive-1;
    if(!*p)return true;
    if(strncmp(p,"?t=",3))return false;p+=3;if(!*p)return false;
    for(;*p;p++)if(!isdigit((unsigned char)*p))return false;
    return true;
}
