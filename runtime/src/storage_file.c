// SPDX-License-Identifier: AGPL-3.0-only
#include "storage_file.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern void host_log(const char *fmt, ...);

static bool storage_error(const char *operation, const char *path) {
    int error = errno;
    host_log("[storage] %s failed for %s: errno %d (%s)", operation, path, error, strerror(error));
    return false;
}

// On Switch, rename cannot replace an existing file. Keep the previous complete
// save in .bak while installing the new file. storage.js also recovers a complete
// .tmp or .bak after interruption; this is recoverable, not a single atomic rename.
bool storage_write_file(const char *path, const char *data, size_t len) {
    char tmp[1100];
    if (snprintf(tmp, sizeof tmp, "%s.tmp", path) >= sizeof tmp) {
        errno = ENAMETOOLONG;
        return storage_error("temporary path", path);
    }
    FILE *f = fopen(tmp, "wb");
    if (!f) return storage_error("open temporary file", path);
    bool ok = fwrite(data, 1, len, f) == len;
    int error = ok ? 0 : errno;
#ifdef __SWITCH__
    if (ok && (fflush(f) != 0 || fsync(fileno(f)) != 0)) {
        ok = false;
        error = errno;
    }
#endif
    if (fclose(f) != 0 && ok) { ok = false; error = errno; }
    if (!ok) { errno = error; return storage_error("write/flush temporary file", path); }
    return storage_install_file(tmp, path);
}

bool storage_install_file(const char *staged, const char *path) {
#ifdef __SWITCH__
    char backup[1100];
    if (snprintf(backup, sizeof backup, "%s.bak", path) >= sizeof backup) {
        errno = ENAMETOOLONG;
        return storage_error("backup path", path);
    }
    struct stat st;
    bool had_previous = stat(path, &st) == 0;
    if (!had_previous && errno != ENOENT) return storage_error("stat save", path);
    if (had_previous) {
        if (!S_ISREG(st.st_mode)) { errno = EINVAL; return storage_error("non-file save", path); }
        // Only retire an older backup while the current save is still intact.
        if (remove(backup) != 0 && errno != ENOENT) return storage_error("remove old backup", path);
        if (rename(path, backup) != 0) return storage_error("retain previous save", path);
    }
    if (rename(staged, path) != 0) {
        storage_error("install new save", path);
        if (had_previous && rename(backup, path) != 0) storage_error("restore previous save", path);
        return false;
    }
#else
    if (rename(staged, path) != 0) return storage_error("replace file", path);
#endif
    return true;
}

bool storage_recover_previous(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return S_ISREG(st.st_mode);
    if (errno != ENOENT) return false;
    char backup[1100];
    if (snprintf(backup, sizeof backup, "%s.bak", path) >= sizeof backup) return false;
    if (stat(backup, &st) != 0 || !S_ISREG(st.st_mode)) return false;
    if (rename(backup, path) != 0) return storage_error("recover previous file", path);
    return true;
}
