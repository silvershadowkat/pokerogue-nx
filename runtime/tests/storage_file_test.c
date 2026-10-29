// SPDX-License-Identifier: AGPL-3.0-only
// Run on Linux with __SWITCH__ and linker wrappers to emulate no-overwrite rename.
#include "storage_file.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fail_install, fail_restore, fail_flush, fail_write;
int __real_rename(const char *, const char *);
int __real_fsync(int);
size_t __real_fwrite(const void *, size_t, size_t, FILE *);

void host_log(const char *fmt, ...) { (void)fmt; }
int __wrap_rename(const char *from, const char *to) {
    if ((fail_install && strstr(from, ".tmp")) || (fail_restore && strstr(from, ".bak"))) {
        errno = EIO;
        return -1;
    }
    struct stat st;
    if (stat(to, &st) == 0) { errno = EEXIST; return -1; }
    return __real_rename(from, to);
}
int __wrap_fsync(int fd) {
    if (fail_flush) { errno = EIO; return -1; }
    return __real_fsync(fd);
}
size_t __wrap_fwrite(const void *p, size_t size, size_t count, FILE *f) {
    if (fail_write) { errno = ENOSPC; return 0; }
    return __real_fwrite(p, size, count, f);
}
static void expect(const char *path, const char *expected) {
    char buf[128] = {0};
    FILE *f = fopen(path, "rb");
    assert(f);
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    assert(fclose(f) == 0);
    assert(n == strlen(expected) && strcmp(buf, expected) == 0);
}
int main(void) {
    char dir[] = "/tmp/pokerogue-storage-XXXXXX";
    assert(mkdtemp(dir) && chdir(dir) == 0);
    assert(storage_write_file("save.json", "first", 5));
    expect("save.json", "first");
    assert(storage_write_file("save.json", "second", 6));
    expect("save.json", "second"); expect("save.json.bak", "first");
    assert(storage_write_file("save.json", "third", 5));
    expect("save.json", "third"); expect("save.json.bak", "second");
    fail_install = 1;
    assert(!storage_write_file("save.json", "fourth", 6));
    expect("save.json", "third"); expect("save.json.tmp", "fourth");
    fail_restore = 1;
    assert(!storage_write_file("save.json", "fifth", 5));
    assert(access("save.json", F_OK) != 0);
    expect("save.json.bak", "third"); expect("save.json.tmp", "fifth");
    fail_install = fail_restore = 0;
    assert(storage_write_file("save.json", "fifth", 5));
    expect("save.json", "fifth"); expect("save.json.bak", "third");
    fail_flush = 1;
    assert(!storage_write_file("save.json", "sixth", 5));
    expect("save.json", "fifth"); expect("save.json.bak", "third");
    fail_flush = 0; fail_write = 1;
    assert(!storage_write_file("save.json", "seventh", 7));
    expect("save.json", "fifth"); expect("save.json.bak", "third");
    fail_write = 0;
    assert(mkdir("directory", 0700) == 0);
    assert(!storage_write_file("directory", "data", 4));
    struct stat st;
    assert(stat("directory", &st) == 0 && S_ISDIR(st.st_mode));
    assert(!storage_write_file("missing/save", "data", 4));
    remove("save.json"); remove("save.json.bak"); remove("save.json.tmp");
    remove("directory.tmp"); rmdir("directory");
    assert(chdir("/") == 0 && rmdir(dir) == 0);
    puts("PASS: repeated Switch-style saves, backup rotation, install/rollback failures, flush/ENOSPC errors, protected existing data");
    return 0;
}
