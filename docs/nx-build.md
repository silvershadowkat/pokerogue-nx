# Building PokeRogue NX

This is a fork of [krappaU/pokerogue-vita](https://github.com/krappaU/pokerogue-vita). Release APK/NRO files contain the native client, runtime scripts, CA certificates and the maintainer's existing app icon. They do not include gameplay, sprites, audio, fonts or launcher artwork. No game build or asset checkout is needed for client packaging. Debug game packs are built separately using [the pinned source workflow](../debug/README.md).

## Switch

Use Linux/WSL with devkitPro: devkitA64, libnx, switch-tools, switch-cmake, switch-sdl2, switch-mesa, switch-libdrm_nouveau, switch-libglapi, switch-curl, switch-mbedtls, switch-zlib and switch-glad. The local tested toolchain includes devkitA64 r30, libnx 4.12.0 and switch-mesa 20.1.0.

```sh
cd runtime
bash scripts/fetch_deps.sh
bash scripts/build_switch.sh
```

Fetch dependencies into a fresh dependency directory: the inherited script resets its QuickJS dependency checkout. Do not run it against a directory shared with unfinished work. For Windows-managed worktrees built in WSL, pass `PKR_COMMIT=<Windows git short HEAD>` to the build script; otherwise it reads the local Git checkout normally.

Output: `runtime/build-switch/pokerogue-nx.nro`. `PKR_DEBUG`, `PKR_DEV` and `PKR_OFFLINE_TEST` are disabled. RomFS contains runtime scripts and the CA bundle, with no `debug.js` or game archive.

From the repository root:

```sh
python3 scripts/stage_switch.py --output outputs/PokeRogue-0.3.0-beta.1-Switch
```

Copy the resulting **switch folder** to the SD root. No unzip step. Launch `switch/pokerogue-nx/pokerogue-nx.nro` through full application mode (hold R while starting a game to enter hbmenu). Applet mode is rejected. Preserve existing saves.

The new app directory is distinct from the old `pokerogue-native-test` installation. Do not move old offline saves into its online profile. Manual migration into Offline can be tested through normal save import after backing up the old installation.

## Android

Windows host; ARM64, Android API 26+, SDK platform 35, build-tools 36.0.0, NDK 29.0.14206865 and JDK 21. Set `ANDROID_HOME` and `JAVA_HOME`. Fetch shared dependencies as above, then:

```powershell
python runtime/scripts/fetch_android_deps.py
python runtime/scripts/build_android.py
python runtime/scripts/package_android.py
```

Output: `runtime/build-android/apk/PokeRogue-0.3.0-beta.1-android-arm64.apk`. Package ID `com.silvershadow.pokeroguenx`; this installs separately from the older native test. The APK is non-debuggable, disallows backups and cleartext HTTP, and requests Internet access. Document import/export uses Android's system picker without broad storage permissions.

The packager requires the existing release key in the maintainer's durable private signing directory, or `PKR_SIGNING_DIR`. It does not create keys and stops if the public certificate differs from the pinned identity. See [private backup locations and the one-time RC7 migration](android-signing.md). Never distribute key/password files.

## Content and updates

First launch downloads/caches the official menu artwork and font. Online loads the live official game; Offline downloads `game.zip` on request from Admiral-Billy's release feed. Offline gameplay uses the archive directly. An update is offered rather than silently replacing an offline game; current updates download the whole archive and resume interrupted downloads.

## Updating upstream

Merge reviewed `krappaU/pokerogue-vita` commits normally from the `upstream` remote on a dedicated branch. Keep platform additions in the SDL platform layer, launcher/input modules and packaging scripts. Record the revision in `game/handheld/upstream.json`. The current imported runtime is `ea0d7d4123a78bd112535efff5e27fbf9d3bb72b`.

The release game is fetched at runtime; updating the older `game/` reference subtree is a separate maintenance task. If the live game changes its private UI fields, review the direct-touch adapter against official source. Its generator is `game/handheld/build-native-touch.cjs`, requiring TypeScript. The generated `runtime/js/handheld-touch.js` is committed, so normal runtime builds do not require Node or a game build.

Run native mode/touch/save regression tests, inspect packages for unintended game/debug files, then test Online login/save/Daily and Offline save/reopen on both platforms before declaring a stable release. See [testing](nx-testing.md).
