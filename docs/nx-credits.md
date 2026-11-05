# PokeRogue NX credits

## Native runtime: krappaU

The native runtime is based on **[krappaU's PokeRogue Vita](https://github.com/krappaU/pokerogue-vita)**.
Its QuickJS-based execution environment, browser compatibility work, native
rendering and audio infrastructure, and existing optimizations are the foundation
of this port. We are adapting that work for Switch and Android, not claiming to
have created it from scratch.

Imported runtime revision: `d7765878e987101c9838006191ad997b9900cd92` (Vita 1.0.17),
reviewed on 2026-11-05 after baseline `ea0d7d4123a78bd112535efff5e27fbf9d3bb72b`.
See [the platform relevance and verification record](vita-sync-2026-11-05.md).
This repository is a direct fork of that project. Its Git history, license,
attribution and original documentation are preserved. The original README is
retained at [upstream-vita-readme.md](upstream-vita-readme.md).

## Official gameplay: Pagefault Games and contributors

**[Official PokeRogue](https://github.com/pagefaultgames/pokerogue)** supplies
the gameplay code, game data, and upstream asset/localization references.
See the original credits in the `game/README.md`, the assets and locales repositories,
and their individual attribution/license files.

Online and stock Offline take gameplay from the official source/distribution.
The separately compiled 0.3.0 Debug overlay adapts the maintainer's local
[SilverShadow offline enhancements](https://github.com/silvershadowkat/pokerogue-offline),
originally based on Scooom's offline fork. Its editors, cheats and custom seeded
runs retain their source history and mapping in `debug/`; its wrapper, minigames,
Google Drive and legacy controller implementation are excluded. Those Debug
changes are never loaded into Online or stock Offline.

## Offline distribution

[Admiral-Billy’s PokeRogue App](https://github.com/Admiral-Billy/Pokerogue-App) and [offline game distribution](https://github.com/Admiral-Billy/pokerogue) supply the existing offline-download approach used by the Vita client. The new app downloads that archive on request; it does not ship a Scooom-derived game or merge Scooom's mods.

The launcher uses the official game's downloaded plains background, logo, window, cursor and Emerald font, retaining upstream asset attribution. No new launcher artwork was invented.

## Platform and library contributors

Thanks to devkitPro and libnx contributors, SDL, Mesa/nouveau, QuickJS-ng, Phaser,
plutovg, curl, mbedTLS, and the other libraries used by the runtime. Dependency
source pins and original license notices remain in the native project. Existing
upstream Vita documentation includes additional component credits.

## NX adaptation and testing

Maintained by [silvershadowkat](https://github.com/silvershadowkat), with AI coding
and testing assistance disclosed in [AI_DISCLOSURE.md](../AI_DISCLOSURE.md).
No AI-generated game assets have been added by this port. Switch hardware tests
and feedback are provided by the maintainer.

The 0.3.0 candidate reuses the maintainer-selected P/R/SS Poké Ball app icon from
the earlier project unchanged. No new icon or other artwork was generated for
this update; this is not a claim about that pre-existing icon's creation process.

## SilverShadow touch controller

The native chevron controller adapts the maintainer's retained SilverShadow control modules and vector paths, whose input foundation credits [Gen1Recomp](https://github.com/bryanthaboi/gen1recomp). Gen1Recomp portions are copyright 2026 BOIS CLUB GAMES, LLC, MIT licensed. The complete notice is in [THIRD_PARTY_TOUCH_NOTICES.md](../THIRD_PARTY_TOUCH_NOTICES.md). No Xelu fallback PNGs are shipped. See [controller provenance and tests](nx-touch-controls.md).
