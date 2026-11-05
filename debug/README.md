# SilverShadow Debug overlay

This is a separately compiled official PokeRogue game with local SilverShadow
enhancements. It is never injected into the live Online game or stock Offline
archive. The APK/NRO contains the native client, not this game pack.

`upstream.json` records the exact official, assets and locales commits, the patch
checksum and normalized SHA-256 of every modified source file. `game.patch` is
the complete source/test diff against that official commit. `feature-map.json`
maps 38 local settings and the additional editors and seeded modes. New source
files are included in the patch. English-only additions live in
`locale-delta.json`; stock translations remain in the base archive.

## Rebuild

Use Python 3.11+, Git, Node and the pnpm version required by the pinned official
project. Work in a separate official checkout; the builder never resets it.

```sh
git clone https://github.com/pagefaultgames/pokerogue work/debug-source
git -C work/debug-source checkout --detach f045bef687ee7a60faa94679939d5d9e074cb19d
python scripts/build_debug.py prepare --source work/debug-source
git -C work/debug-source submodule update --init --depth 1 assets locales
cd work/debug-source
pnpm install --frozen-lockfile
cd ../..
python scripts/build_debug.py build --source work/debug-source
```

Run commands from the client repository except for the explicit `cd`. The last
step verifies pins, merges locale additions, runs type checking and the editor /
seeded-run tests with one worker, builds the game, and writes
`outputs/Debug/debug-pack.zip` and its receipt. Python subprocesses have bounded
timeouts. The `verify` command checks an already prepared checkout. `package`
packages existing `dist` without rerunning the compiler/tests and records that
limitation in the receipt. It must only be used with known Debug compiler output.

The output includes compiled modules and small data/locale additions. Images,
fonts, audio and remaining localization come from the exact stock base ZIP.
Nothing is extracted into thousands of files on the device.

## Bring over upstream changes

1. Make a new isolated official checkout and record its full commit and exact
   asset/locale submodule pins. Confirm a matching stock offline distribution
   exists before using its short commit as `baseVersion`.
2. Apply the old patch for review. Resolve conflicts against current upstream
   behavior rather than replacing current files with legacy versions.
3. Review every row in `feature-map.json`, especially starter copies, move
   attributes, save serialization, daily RNG/snapshots and reward phases.
4. Run checks and focused device tests. Export a new full-index patch against
   the new official commit and regenerate every modified-file SHA-256. Never
   silently advance pins or claim the old tests cover a new build.
5. Use a unique safe `debugVersion` for every published pack, including changes
   against the same base. The native updater compares that version.

The original feature reference was the maintainer's local
[SilverShadow offline project](https://github.com/silvershadowkat/pokerogue-offline)
against official `0d94c5bbbc7a4fc67014c480e31dab1cfdf7ceb4`, including newer local
editor and Boss Rush payloads. Its wrapper, updater, controllers, Google Drive
and minigames are excluded. This source patch retains adapted code and requires
the same AGPL source availability as the original official game.
