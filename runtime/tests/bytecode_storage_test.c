// SPDX-License-Identifier: AGPL-3.0-only
// Exercise the actual module/index writers with Switch no-overwrite semantics.
#ifndef HOST_SOURCE_FILE
#define HOST_SOURCE_FILE "../src/js_host.c"
#endif
#include HOST_SOURCE_FILE
#include <assert.h>
#include <errno.h>
#include <unistd.h>

host_config g_cfg = {.data_dir = "."};
static double test_ms = 1000;
static bool fail_install;
double host_now_ms(void) { return test_ms; }
void host_log(const char *fmt, ...) { (void)fmt; }
int __real_rename(const char *, const char *);
int __wrap_rename(const char *from, const char *to) {
    struct stat st;
    if (fail_install && strstr(from, ".tmp")) { errno = EIO; return -1; }
    if (stat(to, &st) == 0) { errno = EEXIST; return -1; }
    return __real_rename(from, to);
}
static void expect(const char *path, const char *text) {
    char buf[2048] = {0};
    FILE *f = fopen(path, "rb");
    assert(f);
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    assert(fclose(f) == 0);
    assert(n == strlen(text) && !strcmp(buf, text));
}
int main(void) {
    char dir[] = "/tmp/pokerogue-bytecode-XXXXXX";
    assert(mkdtemp(dir) && chdir(dir) == 0);
    const char *one = "./bytecode/0000000000000001.qbc3";
    const char *two = "./bytecode/0000000000000002.qbc3";
    assert(module_bytecode_write(one, "old module", 10));
    assert(module_bytecode_write(one, "new module", 10));
    expect(one, "new module");
    expect("./bytecode/0000000000000001.qbc3.bak", "old module");
    bytecode_note_loaded(one);
    test_ms += 3001;
    module_list_flush();
    expect("./bytecode/startup.txt", "./bytecode/0000000000000001.qbc3\n");
    assert(!bc_list_dirty);
    assert(module_bytecode_write(two, "second module", 13));
    bytecode_note_loaded(two);
    test_ms += 3001;
    fail_install = true;
    module_list_flush();
    assert(bc_list_dirty);
    expect("./bytecode/startup.txt", "./bytecode/0000000000000001.qbc3\n");
    expect("./bytecode/startup.txt.tmp", "./bytecode/0000000000000001.qbc3\n./bytecode/0000000000000002.qbc3\n");
    fail_install = false;
    test_ms += 3001;
    module_list_flush();
    assert(!bc_list_dirty);
    expect("./bytecode/startup.txt", "./bytecode/0000000000000001.qbc3\n./bytecode/0000000000000002.qbc3\n");
    assert(access("./bytecode/startup.txt.tmp", F_OK) != 0);
    expect("./bytecode/startup.txt.bak", "./bytecode/0000000000000001.qbc3\n");
    remove(one); remove(two);
    remove("./bytecode/0000000000000001.qbc3.bak");
    remove("./bytecode/startup.txt"); remove("./bytecode/startup.txt.bak");
    assert(rmdir("./bytecode") == 0 && chdir("/") == 0 && rmdir(dir) == 0);
    puts("PASS: actual module/index writers replace Switch files, preserve prior data on failure, retry and clear completed temporary writes");
    return 0;
}
