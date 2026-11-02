// SPDX-License-Identifier: AGPL-3.0-only
#include "client_modes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 char a[120],b[120],c[120],tiny[4];
 assert(client_profile_path(a,sizeof a,"/app",PKR_ONLINE));assert(client_profile_path(b,sizeof b,"/app",PKR_OFFLINE));assert(client_profile_path(c,sizeof c,"/app",PKR_ENHANCED));
 assert(strcmp(a,b)&&strcmp(b,c)&&strcmp(a,c));assert(!strcmp(a,"/app/profiles/online"));assert(!strcmp(c,"/app/profiles/debug"));
 assert(!client_can_import(PKR_ONLINE));assert(client_can_import(PKR_OFFLINE));assert(client_can_import(PKR_ENHANCED));
 assert(client_can_connect_account(PKR_ONLINE));assert(!client_can_connect_account(PKR_OFFLINE));assert(!client_can_connect_account(PKR_ENHANCED));
 assert(!client_can_load_enhancements(PKR_ONLINE));assert(!client_can_load_enhancements(PKR_OFFLINE));assert(client_can_load_enhancements(PKR_ENHANCED));
 assert(!client_profile_path(tiny,sizeof tiny,"/app",PKR_ONLINE));assert(!client_profile_path(a,sizeof a,"/app",(pkr_client_mode)99));
 assert(!client_can_import((pkr_client_mode)99));assert(!client_can_connect_account((pkr_client_mode)99));assert(!client_can_load_enhancements((pkr_client_mode)99));
 puts("Separate profiles and native online/import/enhancement capabilities PASS");return 0;
}
