// SPDX-License-Identifier: AGPL-3.0-only
// Exercise the production archive reader and its profile boundary directly.
#include "../src/net.c"
#include <assert.h>
host_config g_cfg;
void host_log(const char *fmt,...){(void)fmt;}
static void reset_zip(void){
    if(local_zip_state>0)mz_zip_reader_end(&local_zip);
    if(base_zip_state>0)mz_zip_reader_end(&base_zip);
    local_zip_state=base_zip_state=0;
}
static void expect_file(const char *name,const char *expected){
    char url[1000];snprintf(url,sizeof url,LOCAL_ORIGIN "/%s",name);size_t n=0;
    char *data=local_read(url,&n);
    if(expected){assert(data&&n==strlen(expected)&&!memcmp(data,expected,n));}else assert(!data);
    free(data);
}
int main(int argc,char **argv){
    assert(argc==2);char pack[1000],base[1000];
    snprintf(pack,sizeof pack,"%s/debug.zip",argv[1]);snprintf(base,sizeof base,"%s/base.zip",argv[1]);
    g_cfg.local_dir=pack;g_cfg.local_base=base;g_cfg.client_mode=PKR_ENHANCED;
    expect_file("index.html","debug entry");expect_file("images/shared.bin","stock asset");
    expect_file("locales/en/menu.json","debug translations");
    expect_file("assets/missing.js",NULL);expect_file("missing.json",NULL);expect_file("../private",NULL);
    reset_zip();g_cfg.client_mode=PKR_OFFLINE;expect_file("index.html",NULL);
    reset_zip();g_cfg.client_mode=PKR_ONLINE;expect_file("index.html",NULL);
    reset_zip();g_cfg.local_dir=base;g_cfg.local_base=NULL;g_cfg.client_mode=PKR_OFFLINE;
    expect_file("index.html","stock entry");expect_file("locales/en/menu.json","stock translations");
    reset_zip();g_cfg.client_mode=PKR_ENHANCED;expect_file("index.html",NULL);
    reset_zip();puts("Production overlay reader, pinned asset fallback and mode isolation PASS");return 0;
}
