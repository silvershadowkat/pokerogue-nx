# NX testing status

Status as of 2026-11-02. 0.2.0-rc1 is the Vita-derived Online/Offline candidate. Earlier 0.1.x results below describe the old bundled-offline test and do not prove the new account path works on Switch.

## 0.2.0-rc4 continuous-motion correction

The maintainer tested RC3 on Retroid and reported that the pad was not smooth or responsive. Investigation reproduced a native renderer bug: every new thumb position restarted the transition at zero progress, preventing visual motion during an uninterrupted slide. RC4 samples the running transition before retargeting. A regression using the actual GLES overlay checks full circular gestures at 30/60/120 Hz, immediate native direction changes and zero D-pad texture uploads. The regression fails on the old renderer and passes after correction. On 2026-11-02 the maintainer tested installed RC4 on Retroid and reported that it now felt smooth. Switch feel testing remains pending.

## 0.2.0-rc3 controller checks

The temporary native grid is replaced with the retained SilverShadow controller. Its resolver/pose translation matches 15,045 comparisons against original TypeScript and multi-pointer transition traces. Touch-event, context-mask, scene-cleanup, direct-touch and modal-navigation checks passed. Headless GLES tests exercised the actual native overlay, perspective, opacity, legacy keyboard rendering, neutral-to-slide, action capture, fade/wake and release. They confirmed no texture uploads during D-pad motion after warm-up, but missed continuously retargeted animation. The maintainer's subsequent device test found the responsiveness issue documented above. Switch hardware testing remains pending.

## Maintainer report and 0.2.0-rc2 control fix

On 2026-11-02 the maintainer reported successful Android official-account login, starting an authenticated Daily Run matching the website, and successfully refreshing stale local account data from the server. These establish login/Daily/server-download behavior, not yet a verified upload/close/reopen round trip. Switch account testing remains pending.

The maintainer also found the upstream Login/Register modal had no D-pad navigation, and Login still required a screen tap after password entry. 0.2.0-rc2 adds focus navigation and confirmation through the existing modal callbacks, including fields, Login/Register, Back and final Login. Host regression checks cover these sequences and animation/overlay guards. No credential values are read by this navigation adapter; the original form still handles authentication.

The optional touch pad had been left enabled during development device testing. Fresh-install defaults remain Touch pad Off, Tap menus On. Existing preferences are preserved; select Touch pad Off in the selector to hide it. Physical controls work without this overlay.

## 0.2.0-rc1 checks

- Both Switch AArch64 NRO and non-debuggable signed Android ARM64 APK cross-builds passed during development. Final artifact hashes are recorded with staged deliverables.
- Android installed and ran on Retroid Pocket Nova (1280x960 landscape, Adreno 740). The new selector uses downloaded official artwork and Emerald font. It reached the live pokerogue.net Login/Register screen, opened Login by touch, and showed a synthetic password character as an asterisk. No account credentials were submitted.
- Direct-touch and optional pad were enabled together. Native pad artwork is code-drawn; labels reuse the downloaded game font. Full controller/touch interaction still needs human testing.
- Android downloaded the offline release `release-29441f7`: 542,705,080 bytes, ZIP directory validated with 34,246 entries. It opened directly from the archive, advanced the welcome dialogue and character choice by touch, and reached the guest title screen. Offline save/reopen for this candidate is still pending. Download pause/resume was not established by this session's hardware test.
- Host tests passed: native profile/capability isolation; finger ownership, multitouch and mode changes; official form callbacks and field focus in a 4:3 letterbox; repeated Switch-style saves, interrupted replacement/rollback and archive recovery; JavaScript storage recovery.
- JavaScript syntax and Python packaging-script compilation passed. Packages exclude `game.zip` and `debug.js`; the NRO runtime uses GPU EGL/GLES, not nx.js.
- **New Switch hardware checks remain pending:** account login, cloud save/reload, authenticated Daily Run, offline save persistence and performance. Android account login/cloud save/Daily are being handed to the maintainer for a private real-account test. External-provider OAuth has not been validated.

These are limited smoke tests, not full parity or long-session stability results. No real credentials/private saves belong in source or test artifacts.

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

## Broader checks not yet established

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

## Candidate input and mode tests

```sh
cc -Iruntime/src runtime/tests/client_modes_test.c runtime/src/client_modes.c -o runtime/build-tests/client-modes-test
runtime/build-tests/client-modes-test
cc -Iruntime/src runtime/tests/touch_controls_test.c runtime/src/touch_controls.c -o runtime/build-tests/touch-controls-test
runtime/build-tests/touch-controls-test
node runtime/tests/handheld_touch_test.cjs
```
