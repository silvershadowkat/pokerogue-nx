// SPDX-License-Identifier: AGPL-3.0-only
#include "game_packages.h"
#include "storage_file.h"
#include "miniz.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

bool game_version_valid(const char *v){
    if(!v||!*v||strlen(v)>=64||!isalnum((unsigned char)v[0]))return false;
    for(const char *p=v;*p;p++)if(!isalnum((unsigned char)*p)&&*p!='.'&&*p!='-'&&*p!='_')return false;
    return strstr(v,"..")==NULL;
}
bool game_base_path(char *out,size_t n,const char *root,const char *v){
    if(!game_version_valid(v)||!root||!*root)return false;
    int count=snprintf(out,n,"%s/games/%s.zip",root,v);return count>=0&&(size_t)count<n;
}
static bool exists(const char *path){FILE*f=fopen(path,"rb");if(!f)return false;fclose(f);return true;}
static bool read_version(const char *path,char *out,size_t n){
    storage_recover_previous(path);FILE*f=fopen(path,"rb");if(!f)return false;
    bool ok=fgets(out,(int)n,f)!=NULL;fclose(f);if(!ok)return false;
    out[strcspn(out,"\r\n")]=0;return game_version_valid(out);
}
// Manifest values are identifiers, never paths or arbitrary escaped strings.
static bool identifier(const char *json,const char *key,char *out,size_t n){
    char pattern[100];snprintf(pattern,sizeof pattern,"\"%s\"",key);
    const char*p=strstr(json,pattern);if(!p)return false;p+=strlen(pattern);
    while(isspace((unsigned char)*p))p++;if(*p++!=':')return false;
    while(isspace((unsigned char)*p))p++;if(*p++!='"')return false;
    size_t i=0;while(*p&&*p!='"'&&i+1<n)out[i++]=*p++;
    out[i]=0;return *p=='"'&&game_version_valid(out);
}
bool game_debug_info(const char *path,debug_package_info *info){
    mz_zip_archive z={0};memset(info,0,sizeof *info);
    if(!mz_zip_reader_init_file(&z,path,0))return false;
    int index=mz_zip_reader_locate_file(&z,"pokerogue-debug.json",NULL,0);
    mz_zip_archive_file_stat st;bool ok=index>=0&&mz_zip_reader_file_stat(&z,(mz_uint)index,&st)&&st.m_uncomp_size<=4096;
    size_t size=0;char *data=ok?mz_zip_reader_extract_to_heap(&z,(mz_uint)index,&size,0):NULL;
    char *json=data?malloc(size+1):NULL;
    if(json){memcpy(json,data,size);json[size]=0;char schema[64];
        ok=identifier(json,"schema",schema,sizeof schema)&&!strcmp(schema,"pokerogue-debug-v1")
            &&identifier(json,"version",info->version,sizeof info->version)
            &&identifier(json,"baseVersion",info->base_version,sizeof info->base_version)
            &&mz_zip_reader_locate_file(&z,"index.html",NULL,0)>=0;
    }else ok=false;
    free(json);mz_free(data);mz_zip_reader_end(&z);return ok;
}
bool game_find_offline(char *path,size_t pn,char *version,size_t vn,const char *root){
    char record[900];snprintf(record,sizeof record,"%s/game-current.txt",root);
    if(read_version(record,version,vn)&&game_base_path(path,pn,root,version)){
        storage_recover_previous(path);if(exists(path))return true;
    }
    // Preserve RC7 installations, including manually copied extracted games.
    snprintf(path,pn,"%s/game",root);snprintf(record,sizeof record,"%s/index.html",path);
    if(exists(record)){snprintf(record,sizeof record,"%s/currentVersion.txt",path);if(!read_version(record,version,vn))snprintf(version,vn,"unknown");return true;}
    snprintf(path,pn,"%s/game.zip",root);storage_recover_previous(path);
    if(!exists(path))return false;snprintf(record,sizeof record,"%s/game.zip.version",root);
    if(!read_version(record,version,vn))snprintf(version,vn,"unknown");return true;
}
bool game_find_debug(char *path,size_t pn,char *base,size_t bn,debug_package_info *info,const char *root){
    snprintf(path,pn,"%s/debug-game.zip",root);storage_recover_previous(path);
    if(!game_debug_info(path,info)||!game_base_path(base,bn,root,info->base_version))return false;
    storage_recover_previous(base);return exists(base);
}
