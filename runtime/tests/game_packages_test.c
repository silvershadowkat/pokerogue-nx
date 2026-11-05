// SPDX-License-Identifier: AGPL-3.0-only
#include "game_packages.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void host_log(const char *fmt,...){(void)fmt;}
int main(int argc,char **argv){
    assert(argc==2);char path[1200],base[1200],version[64];debug_package_info info;
    assert(game_version_valid("f045bef"));assert(game_version_valid("ss-0.3.0-beta.1-f045bef"));
    assert(!game_version_valid("../secret"));assert(!game_version_valid("/root"));assert(!game_version_valid("bad value"));assert(!game_version_valid("a..b"));
    char tiny[4];assert(!game_base_path(tiny,sizeof tiny,argv[1],"f045bef"));
    snprintf(path,sizeof path,"%s/debug.zip",argv[1]);assert(game_debug_info(path,&info));
    assert(!strcmp(info.base_version,"f045bef"));assert(!strcmp(info.version,"ss-0.3.0-beta.1-f045bef"));
    const char *invalid[]={"base.zip","bad-schema.zip","traversal.zip","no-entry.zip","oversized.zip","corrupt.zip"};
    for(size_t i=0;i<sizeof invalid/sizeof *invalid;i++){snprintf(path,sizeof path,"%s/%s",argv[1],invalid[i]);assert(!game_debug_info(path,&info));}
    assert(game_find_offline(path,sizeof path,version,sizeof version,argv[1]));assert(!strcmp(version,"f045bef"));
    assert(strstr(path,"games/f045bef.zip"));
    assert(game_find_debug(path,sizeof path,base,sizeof base,&info,argv[1]));assert(strstr(base,"games/f045bef.zip"));
    puts("Version identifiers, malformed packs and pinned/current package discovery PASS");return 0;
}
