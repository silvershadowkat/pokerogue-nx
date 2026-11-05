// SPDX-License-Identifier: AGPL-3.0-only
// Downloads versioned stock bases and a separate Debug overlay from the selector.
//
// The build is the one Admiral-Billy's PokeRogue App uses for its offline mode: game.zip from the
// latest release of github.com/Admiral-Billy/pokerogue (about 530 MB). Interrupted
// downloads resume. Validated bases live under games/<version>.zip; Debug pins a
// matching base and supplies debug-game.zip. net.c plays directly from the ZIPs.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "host.h"
#include "miniz.h"
#include "storage_file.h"

#define RELEASE_API "https://api.github.com/repos/Admiral-Billy/pokerogue/releases/latest"
#define ASSET_NAME "game.zip"
#define DEBUG_ASSET_NAME "debug-pack.zip"
#define DEBUG_API "https://api.github.com/repos/silvershadowkat/pokerogue-nx/releases/tags/debug-game"

typedef struct {
    double t0, last_draw;
    long long last_bytes;
    double speed; // bytes per second, smoothed
    const char *what;
} progress_ui;

// ○ (circle) or START + SELECT cancels.
static bool cancel_pressed(void) {
    plat_event ev;
    bool cancel = false;
    while (plat_poll_event(&ev))
        if (ev.type == PEV_QUIT || ev.type == PEV_POINTER_DOWN || (ev.type == PEV_KEY_DOWN && ev.key && !strcmp(ev.key, "Escape"))) cancel = true;
    plat_gamepad pad;
    if (plat_gamepads(&pad, 1) > 0 && pad.buttons[1] > 0.5f) cancel = true;
    return cancel;
}

static bool show_progress(long long done, long long total, void *ud) {
    progress_ui *p = ud;
    double now = host_now_ms();
    if (now - p->last_draw < 250 && done != total) return !cancel_pressed();
    if (p->last_draw > 0) {
        double inst = (done - p->last_bytes) * 1000.0 / (now - p->last_draw);
        p->speed = p->speed > 0 ? p->speed * 0.8 + inst * 0.2 : inst;
    }
    p->last_draw = now, p->last_bytes = done;
    plat_keep_awake();
    char msg[600], eta[64] = "";
    if (total > 0 && p->speed > 1000) {
        int secs = (int)((total - done) / p->speed);
        snprintf(eta, sizeof eta, ", about %d:%02d left", secs / 60, secs % 60);
    }
    if (total > 0)
        snprintf(msg, sizeof msg, "%s\n\n%.0f of %.0f MB (%.1f MB/s%s)\n\nTap the screen or press the cancel button to pause.\nThe download continues where it stopped next time.",
                 p->what, done / 1048576.0, total / 1048576.0, p->speed / 1048576.0, eta);
    else
        snprintf(msg, sizeof msg, "%s\n\n%.0f MB\n\nTap the screen or press the cancel button to pause.", p->what, done / 1048576.0);
    overlay_present(msg);
    return !cancel_pressed();
}

// Shows a message until a button is pressed (so errors can be read).
void ui_message_wait(const char *msg) {
    char full[800];
    snprintf(full, sizeof full, "%s\n\nTap the screen or press confirm to go back.", msg);
    double t0 = host_now_ms();
    for (;;) {
        overlay_present(full);
        plat_event ev;
        bool done = false;
        while (plat_poll_event(&ev))
            if (ev.type == PEV_QUIT || (ev.type == PEV_KEY_DOWN && ev.key && !strcmp(ev.key, "Enter")) ||
                ev.type == PEV_POINTER_DOWN)
                done = true;
        plat_gamepad pad;
        if (host_now_ms() - t0 > 500 && plat_gamepads(&pad, 1) > 0 && (pad.buttons[0] > 0.5f || pad.buttons[1] > 0.5f))
            done = true;
        if (done) return;
#ifndef __vita__
        struct timespec ts = {0, 30 * 1000000L};
        nanosleep(&ts, NULL);
#endif
    }
}

// Finds "key":"value" after `from` in a JSON text (enough for GitHub's release JSON).
bool json_str(const char *from, const char *key, char *out, size_t n) {
    char pat[64];
    snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(from, pat);
    if (!p) return false;
    p += strlen(pat);
    while (*p == ' ') p++;
    if (*p != '"') return false;
    p++;
    size_t o = 0;
    while (*p && *p != '"' && o < n - 1) out[o++] = *p++;
    out[o] = 0;
    return true;
}

long long json_num(const char *from, const char *key) {
    char pat[64];
    snprintf(pat, sizeof pat, "\"%s\":", key);
    const char *p = strstr(from, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p == ' ') p++;
    return atoll(p);
}

// ---------------------------------------------------------------- update check
// The start screen asks GitHub for the latest release in the background (at most every 6 hours;
// the answer is kept in <data>/offline-latest.txt) and offers an update when its tag differs from
// the installed version.
#define CHECK_EVERY_S (6 * 3600)
static pthread_mutex_t check_lock = PTHREAD_MUTEX_INITIALIZER;
static char latest[2][64];   // newest release tag without "release-", "" while unknown
static bool check_running[2];

static const char *strip_tag(const char *tag) { return !strncmp(tag, "release-", 8) ? tag + 8 : tag; }

static void latest_path(char *out, size_t n,int channel) { snprintf(out, n, "%s/%s-latest.txt", g_cfg.app_dir,channel?"debug":"offline"); }

static void *check_thread(void *arg) {
    int channel=(int)(intptr_t)arg;
    size_t len = 0;
    long status = 0;
    char *json = net_get_threadsafe(channel?DEBUG_API:RELEASE_API, &len, &status);
    char tag[64] = "";
    const char *asset_name=channel?DEBUG_ASSET_NAME:ASSET_NAME;
    char asset_pattern[100];snprintf(asset_pattern,sizeof asset_pattern,"\"name\":\"%s\"",asset_name);
    char asset_pretty[100];snprintf(asset_pretty,sizeof asset_pretty,"\"name\": \"%s\"",asset_name);
    if (json && status == 200 && json_str(json, channel?"name":"tag_name", tag, sizeof tag) && game_version_valid(channel?tag:strip_tag(tag)) &&
        (strstr(json,asset_pattern)||strstr(json,asset_pretty))) {
        char path[700];
        latest_path(path, sizeof path,channel);
        FILE *f = fopen(path, "w");
        if (f) {
            fprintf(f, "%lld %s\n", (long long)time(NULL), channel?tag:strip_tag(tag));
            fclose(f);
        }
        host_log("[offline] latest release on GitHub: %s", tag);
    } else {
        host_log("[offline] update check failed (HTTP %ld)", status);
        tag[0] = 0;
    }
    if(tag[0]&&!game_version_valid(channel?tag:strip_tag(tag)))tag[0]=0;
    free(json);
    pthread_mutex_lock(&check_lock);
    if (tag[0]) snprintf(latest[channel], sizeof latest[channel], "%s", channel?tag:strip_tag(tag));
    check_running[channel] = false;
    pthread_mutex_unlock(&check_lock);
    return NULL;
}

static void game_update_check_start(int channel) {
    char path[700], tag[64] = "";
    long long when = 0;
    latest_path(path, sizeof path,channel);
    FILE *f = fopen(path, "r");
    if (f) {
        if (fscanf(f, "%lld %63s", &when, tag) != 2) tag[0] = 0;
        fclose(f);
    }
    pthread_mutex_lock(&check_lock);
    if (tag[0]) snprintf(latest[channel], sizeof latest[channel], "%s", tag);
    bool due = !tag[0] || time(NULL) - when > CHECK_EVERY_S || time(NULL) < when;
    if (due && !check_running[channel]) {
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 256 * 1024); // TLS handshakes need a deep stack
        check_running[channel] = pthread_create(&t, &attr, check_thread, (void *)(intptr_t)channel) == 0;
        if (check_running[channel]) pthread_detach(t);
        pthread_attr_destroy(&attr);
    }
    pthread_mutex_unlock(&check_lock);
}

// The newer release's tag, or NULL (no update known, or `installed` is the latest).
static const char *game_update_available(const char *installed,int channel) {
    static char out[2][64];
    pthread_mutex_lock(&check_lock);
    bool newer = latest[channel][0] && installed && installed[0] && strcmp(latest[channel], installed) != 0;
    if (newer) snprintf(out[channel], sizeof out[channel], "%s", latest[channel]);
    pthread_mutex_unlock(&check_lock);
    return newer ? out[channel] : NULL;
}

void offline_update_check_start(void){game_update_check_start(0);}
void debug_update_check_start(void){game_update_check_start(1);}
const char *offline_update_available(const char *v){return game_update_available(v,0);}
const char *debug_update_available(const char *v){return game_update_available(v,1);}

// Asks a yes/no question: X yes, O no.
bool ui_confirm(const char *msg) {
    char full[900];
    snprintf(full, sizeof full, "%s\n\nConfirm: yes   Cancel: no\nTouch: left half = yes, right half = no", msg);
    double t0 = host_now_ms();
    for (;;) {
        overlay_present(full);
        plat_event ev;
        while (plat_poll_event(&ev)) {
            if (ev.type == PEV_POINTER_DOWN && host_now_ms()-t0>500) { int w,h;plat_drawable_size(&w,&h);return ev.x<w/2; }
            if (ev.type == PEV_QUIT || (ev.type == PEV_KEY_DOWN && ev.key && !strcmp(ev.key, "Escape"))) return false;
            if (ev.type == PEV_KEY_DOWN && ev.key && !strcmp(ev.key, "Enter")) return true;
        }
        plat_gamepad pad;
        if (host_now_ms() - t0 > 500 && plat_gamepads(&pad, 1) > 0) {
            if (pad.buttons[0] > 0.5f) return true;
            if (pad.buttons[1] > 0.5f) return false;
        }
#ifndef __vita__
        struct timespec ts = {0, 30 * 1000000L};
        nanosleep(&ts, NULL);
#endif
    }
}

// ---------------------------------------------------------------- download

bool ui_download(const char *url, const char *path, const char *what, char *err, size_t errn) {
    progress_ui ui = {.t0 = host_now_ms(), .what = what};
    return net_download_file(url, path, show_progress, &ui, err, errn);
}

// Downloads the latest release to <data>/game.zip (replacing an installed one only once the
// download is complete). Returns true when <data>/game.zip is ready.
// Keep immutable base archives by version. Updating Offline cannot strand a
// Debug pack on newer, incompatible assets, and both modes share the same ZIP.
static bool regular_archive(const char *path){
    mz_zip_archive z={0};if(!mz_zip_reader_init_file(&z,path,0))return false;
    bool ok=mz_zip_reader_locate_file(&z,"index.html",NULL,0)>=0&&mz_zip_reader_locate_file(&z,"pokerogue-debug.json",NULL,0)<0;
    mz_zip_reader_end(&z);return ok;
}
static bool activate_base(const char *version){
    char record[1000];snprintf(record,sizeof record,"%s/game-current.txt",g_cfg.app_dir);
    return storage_write_file(record,version,strlen(version));
}
static bool game_install(bool debug,const char *requested,bool activate){
    char api[1200],err[600]="";
    if(requested&&!game_version_valid(requested))return false;
    if(debug)snprintf(api,sizeof api,"%s",DEBUG_API);
    else if(requested)snprintf(api,sizeof api,"https://api.github.com/repos/Admiral-Billy/pokerogue/releases/tags/release-%s",requested);
    else snprintf(api,sizeof api,"%s",RELEASE_API);
    char games[1000];snprintf(games,sizeof games,"%s/games",g_cfg.app_dir);mkdir(games,0777);
    // A previously downloaded/pinned base needs no network or duplicate copy.
    char game_zip[1100];
    if(!debug&&requested&&game_base_path(game_zip,sizeof game_zip,g_cfg.app_dir,requested)){
        storage_recover_previous(game_zip);
        if(regular_archive(game_zip))return !activate||activate_base(requested);
        char old[1000],version[64];
        if(game_find_offline(old,sizeof old,version,sizeof version,g_cfg.app_dir)&&!strcmp(version,requested)&&regular_archive(old)){
            if(!activate_base(requested))return false;
            return storage_install_file(old,game_zip);
        }
    }
    overlay_present(debug?"PokeRogue\n\nLooking for a compatible Debug patch pack...":"PokeRogue\n\nLooking for the offline game...");
    size_t length=0;long status=0;char *json=net_get_sync(api,&length,&status);
    if(!json||status!=200){free(json);ui_message_wait(debug?"The Debug patch channel is not available yet.\nAn installed compatible Debug version remains available.":"Couldn't reach the offline game release.\nCheck Wi-Fi and try again.");return false;}
    char tag[64]="",version[64]="",url[1200]="";
    bool version_ok=json_str(json,debug?"name":"tag_name",tag,sizeof tag);
    snprintf(version,sizeof version,"%s",debug?tag:strip_tag(tag));
    version_ok=version_ok&&game_version_valid(version)&&(!requested||!strcmp(version,requested));
    const char *asset_name=debug?DEBUG_ASSET_NAME:ASSET_NAME;
    char pattern[100];snprintf(pattern,sizeof pattern,"\"name\":\"%s\"",asset_name);
    const char *asset=strstr(json,pattern);
    if(!asset){snprintf(pattern,sizeof pattern,"\"name\": \"%s\"",asset_name);asset=strstr(json,pattern);}
    long long expected=asset?json_num(asset,"size"):-1;
    bool url_ok=asset&&json_str(asset,"browser_download_url",url,sizeof url);
    free(json);
    if(!version_ok||!url_ok||expected<=0){ui_message_wait("This release has no valid compatible game package.\nThe installed version is kept.");return false;}
    if(debug)snprintf(game_zip,sizeof game_zip,"%s/debug-game.zip",g_cfg.app_dir);
    else if(!game_base_path(game_zip,sizeof game_zip,g_cfg.app_dir,version))return false;
    if(!debug&&regular_archive(game_zip))return !activate||activate_base(version);
    char partial[1200],part_tag[1300],previous[64]="";
    snprintf(partial,sizeof partial,"%s.part",game_zip);snprintf(part_tag,sizeof part_tag,"%s.version",partial);
    FILE *f=fopen(part_tag,"rb");if(f){fgets(previous,sizeof previous,f);fclose(f);}
    if(strcmp(previous,version)){remove(partial);if(!storage_write_file(part_tag,version,strlen(version)))return false;}
    struct stat st;long long done=stat(partial,&st)==0?st.st_size:0;
    long long free_bytes=plat_free_space(g_cfg.app_dir);
    if(free_bytes>=0&&free_bytes+done<expected+16*1024*1024){ui_message_wait("Not enough free space to download safely.\nThe installed game and local saves are kept.");return false;}
    char what[200];snprintf(what,sizeof what,debug?"Downloading Debug patch pack (%s)":"Downloading shared game assets (%s)",version);
    progress_ui progress={.t0=host_now_ms(),.what=what};
    if(!net_download_file(url,partial,show_progress,&progress,err,sizeof err)){
        if(strcmp(err,"cancelled"))ui_message_wait("The download stopped. Choose the mode again to resume it.");return false;
    }
    debug_package_info info;
    bool size_ok=stat(partial,&st)==0&&st.st_size==expected;
    bool valid=size_ok&&(debug?game_debug_info(partial,&info):regular_archive(partial));
    if(valid&&debug)valid=!strcmp(info.version,version);
    if(!valid){remove(partial);ui_message_wait("The downloaded package failed its version or archive check.\nThe installed game is kept.");return false;}
    // Fetch the precise unmodified base, not whatever happens to be latest
    // while this Debug overlay was built. Existing cached versions are reused.
    if(debug&&!game_install(false,info.base_version,false))return false;
    if(!storage_install_file(partial,game_zip)){ui_message_wait("Couldn't install the game package. The previous version is recoverable.");return false;}
    remove(part_tag);
    if(!debug&&activate&&!activate_base(version))return false;
    char backup[1300];snprintf(backup,sizeof backup,"%s.bak",game_zip);remove(backup);
    host_log("[packages] installed %s %s",debug?"Debug patch pack":"stock base",version);
    return true;
}
bool offline_install(void){return game_install(false,NULL,true);}
bool debug_game_install(void){return game_install(true,NULL,false);}
