# Client release checks

This candidate follows the Vita-derived client route. The separate full C++ rewrite is preserved elsewhere and is not part of these builds. The upstream runtime merge is recorded as a normal merge commit so future Vita changes can be reviewed and integrated with Git.

## Vita-specific settings reviewed

| Area | Switch/Android handling |
| --- | --- |
| VitaSDK/vitaGL, Vita clocks and memory layout | Only the Vita platform source; not linked into these clients |
| Vita fixed heap/system-dialog GPU memory reservation | Not applied to Switch/Android |
| Vita thread/core priority helpers | SDL platform helper is currently a no-op; do not copy Vita core affinity assumptions blindly |
| Graphics | Switch requires hardware EGL/GLES through Mesa/nouveau; Android uses its native GLES driver |
| Frame cap | 30 FPS optional and off by default; normal 60 Hz pacing retained |
| Native fast paths, image/audio memory handling, translation cache | Retained; useful beyond Vita |
| Four-texture batches and shader rewrites | Retained pending Switch measurements; not proof that four is optimal for Switch |
| Phaser pre-FX allocation suppression | Retained upstream behavior; effects parity remains a test area |
| Browser hardware/memory hints | Conservative upstream hints retained; tune only with game behavior/performance comparisons |

There is no claim that every inherited optimization is necessary or visually neutral. Review the generic transformations against live-game changes rather than deleting them together.

## Candidate boundaries

Native mode capabilities control account network access and file imports. Profiles are separate directories, and mode changes require process restart. Debug code is not shipped in Online/Offline; the future enhanced build will need an explicit native launch capability and its own profile/content.

These restrictions do not make a user-controlled executable tamper-proof. Server validation remains the authority for online saves. No custom authentication bypass or trusted-client assertion is added.

Username/password entry uses the official game's login request through verified HTTPS. Native password display is masked; the test keyboard path does not log initial input contents. Account credentials have not been submitted during automated testing. The maintainer has since reported successful Android login, authenticated Daily Run and server-save refresh. External-provider OAuth flows are not yet verified on either device.

## Upstream check

On 2026-11-02 a fresh fetch confirmed that Vita `main` still points to `ea0d7d4123a78bd112535efff5e27fbf9d3bb72b` (runtime 1.0.6). All commits on that branch are included in RC4; there are zero missing mainline commits. The latest arena sprite batching changes are retained. Older experimental branches are not automatically merged.

## Switch cache-file replacement

RC6 uses the recoverable writer for compiled modules and `bytecode/startup.txt`, preventing stale `.tmp` index files caused by Switch rename-over-existing behavior. Failed index updates retry. This is independent of the RC5 clock/save correction. The on-device cleanup validates the pending index and referenced cache files before promoting it, with a private backup of the original; user profiles and settings are preserved.

## Cloud-save correction and confirmed round trip

RC4 Switch login/server download worked, but the server rejected its save upload because the runtime returned fractional `Date.now()` timestamps. RC5 restores integer browser timestamps in `runtime/js/core.js`; the actual-shim regression reproduces the reported value and passes after correction. This is a runtime compatibility fix; authentication, official game save rules and server validation are unchanged. On 2026-11-02 the maintainer confirmed Switch-to-PC upload/reopen, PC-to-Switch upload/reopen and automatic server refresh. This specific cloud-save blocker is cleared; remaining hardware checks still apply.

Online game Settings remain stock. Touch settings stay in the native selector, default to Fade on fresh RC5 installations, and remain device-local. Existing choices are preserved. Contextual shortcut coverage/labels, multi-aspect-ratio layout checks, touch-layout editing and haptics remain follow-up work.

## Updates and pending work

Online automatically retrieves the live entry point and manifest, reusing versioned cached assets. Offline checks every six hours and offers an update. Offline downloads currently transfer the full ZIP; real per-file/byte deltas require a separate manifest/range update design. No game archive is extracted to the SD card.

Runtime/app update notifications are not implemented; install updated APK/NRO releases manually. Cached launcher assets are reused locally. Debug, custom modes, a redesigned cheats menu and one-way account export workflow testing are follow-up work.

Switch login and bidirectional server save/reload are maintainer-confirmed. Before calling the release stable, confirm Switch authenticated Daily, then repeat offline creation/update/save/reopen. Android login, authenticated Daily and server-data refresh are maintainer-confirmed; save upload/close/reopen remains pending. Device performance and long-session stability remain hardware checks, not build results.
