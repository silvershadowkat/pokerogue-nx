# NX testing status

This is an experimental port, not a completed compatibility claim. Status as of
2026-10-29; version labels below refer to the local test builds.

## Nintendo Switch: maintainer hardware reports

- 0.1.1 booted on the first attempt, played through wave 11, and used the NV120
  GPU through native EGL/OpenGL ES and Mesa/nouveau.
- The roughly ten-minute log contained 74 later gameplay performance windows
  averaging 52.0-60.3 FPS, with median 58.75. These are interval averages, not
  minimum frame rates. Startup and scene transitions include long frames.
- Reported heap peaked at 522 MB. This is not a long-session leak test.
- 0.1.1 failed to persist saves. Do not use that version for a run you want to keep.
- The maintainer confirmed the 0.1.2 A/B and X/Y correction works.
- 0.1.3 fixes save replacement and recovery. The native cross-build, fault-injected
  filesystem tests, and JavaScript recovery tests passed on PC. **Repeated
  save/close/reopen on real Switch hardware still needs verification.**
- Both supplied launches prepared 35 shader variants in 2.9 seconds. Shader
  preparation currently runs on every launch; JavaScript bytecode is cached.

## Android and desktop

The Android 0.1.1 native test ran on a Retroid handheld with Adreno GPU rendering.
Checks covered starter selection, battle/reward progression, direct touch menus,
save/reopen, settings restart, save export/import, cancellation, and clean exit.
The latest Switch save fix has not been retested as a new Android release.

Linux reference checks covered battles, rewards, save/restart, and save-format
crypto vectors. Desktop reference tests used software GL and dummy audio where
noted; they do not establish Switch frame rates or audible audio quality.

## Not yet established

Full feature parity, every fusion/effect, all controller types, extended sessions,
official online accounts, cloud saves, and the authentic online Daily Run.
No official account credentials or private saves are included in this repository.

## Reproducing the save regression tests

On Linux, from the repository root:

```sh
mkdir -p runtime/build-tests
cc -D__SWITCH__ -Iruntime/src \
  runtime/tests/storage_file_test.c runtime/src/storage_file.c \
  -Wl,--wrap=rename,--wrap=fsync,--wrap=fwrite \
  -o runtime/build-tests/storage-file-test
runtime/build-tests/storage-file-test
node runtime/tests/storage_recovery_test.cjs
```

The C test emulates Switch's refusal to rename over an existing destination and
injects write, flush, install, and rollback failures. It is a host test, not Switch
emulation. The JavaScript test checks recovery selection and restart behavior.
