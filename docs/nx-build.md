# Building PokeRogue NX

This repository is a direct fork of krappaU/pokerogue-vita. The official game
is vendored under `game/` as a Git subtree; a second user-owned fork is not needed.
See `game/handheld/upstream.json` for the imported source revisions.

## Game bundle

Use Node.js 24.9 or newer and the pnpm version declared in `game/package.json`.
From a fresh checkout:

```sh
git submodule update --init --recursive
cd game
pnpm install --frozen-lockfile
pnpm exec vite build --mode native --config handheld/vite.config.ts
node handheld/build-native-touch.cjs ../runtime/js/handheld-touch.js
python3 handheld/package_native.py ../runtime/.cache/android/game.zip
cd ..
```

The packaging command validates the asset/localization revisions and puts the
game and assets in one indexed archive. The native runtime reads that archive
without extracting tens of thousands of files onto the SD card. `android` in
this cache path is historical; the same game archive is used by Switch.

## Switch runtime

Use Linux or WSL and devkitPro's Switch packages. The tested toolchain includes
devkitA64 r30, libnx 4.12.0, switch-tools 1.13.1, switch-cmake, switch-pkg-config,
switch-sdl2 2.28.5, switch-mesa 20.1.0, switch-libdrm_nouveau, switch-libglapi,
switch-curl 7.69.1, switch-mbedtls 2.28.10, switch-zlib, and switch-glad.
Install packages through devkitPro's supported package manager.

```sh
cd runtime
bash scripts/fetch_deps.sh
bash scripts/build_switch.sh
```

Dependency fetching downloads the pinned sources and applies the bundled
QuickJS patches. Use a fresh dependency directory: the inherited fetch script
resets tracked changes inside its QuickJS dependency checkout.

Output: `runtime/build-switch/pokerogue-native-test.nro`, including the
runtime JavaScript in RomFS. The offline test build enforces the bundled game;
its diagnostics do not enable the developer input/evaluation channel.

Stage a full first installation from the repository root:

```sh
python3 scripts/stage_switch.py --output outputs/Switch-Test \
  --game runtime/.cache/android/game.zip
```

For later runtime-only updates, omit `--game`. This produces a folder containing
`switch/pokerogue-native-test/`, not a ZIP. Close the app, copy the `switch` folder
to the SD root and replace only supplied files. Keep existing saves and assets.
Launch through full application mode, holding R while starting a game to enter
hbmenu. Applet mode is deliberately rejected due to its smaller memory limit.

Back up `localStorage-offline.json` and any `.tmp`/`.bak` files before experimental
save updates. The 0.1.3 save fix still needs real Switch save/reopen validation.

## Android and desktop

The experimental Android scripts currently target a Windows host, ARM64, API 26+,
Android SDK platform 35, build-tools 36.0.0, NDK 29.0.14206865 and JDK 21.
Set `ANDROID_HOME` and `JAVA_HOME`. Fetch shared dependencies with the script
above, then use `runtime/scripts/fetch_android_deps.py`,
`build_android.py`, and `package_android.py`. Package the game archive first.
The packager creates a local debug signing key if needed; never commit it.
The earlier WebView experiment under `game/handheld/android` is not the native port.

See `runtime/README.md` for the inherited Linux reference build. Some
documentation in the runtime and inherited docs describes upstream Vita features, not Switch
features implemented or validated here.

## Updating upstream

The `upstream` remote points to krappaU/pokerogue-vita. Integrate a reviewed
runtime revision on a dedicated branch with a normal Git merge; preserve and
test the Switch/Android adaptations. Record the reviewed revision in
`game/handheld/upstream.json`.

The `official` remote points to pagefaultgames/pokerogue. Import reviewed gameplay
updates using `git subtree pull --prefix=game official <reviewed-ref> --squash`
on a dedicated update branch. Resolve the small handheld overlay, update the
official revision in `game/handheld/upstream.json`, and initialize the exact
asset/localization submodule revisions at `game/assets` and `game/locales`.
The outer `.gitmodules` registers those nested paths for a fresh clone.

Rebuild the bundle and regenerate `runtime/js/handheld-touch.js`. Review
`runtime/src/gamefix.c`, native fast paths, shader transforms, and private UI
hooks when the game's code or Phaser changes. Run checks and device smoke tests
before shipping. Official updates are not guaranteed to be automatically
compatible. The source archive records both the official upstream revision and
the NX source commit used to build it.
