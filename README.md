# PokeRogue NX

An experimental Nintendo Switch homebrew port of official **PokeRogue**, with
Android and desktop support under development. This is a fork of krappaU's Vita
client. The native host lives in `runtime/`;
a pinned official game subtree under `game/` supplies the offline bundle.

**Native runtime foundation: [krappaU's PokeRogue Vita](https://github.com/krappaU/pokerogue-vita).**
This port builds on krappaU's substantial work on the native host, QuickJS
integration, browser API compatibility, rendering, and audio. It is an adaptation
of that project, not a native runtime written from scratch here.

**Gameplay and assets: [Pagefault Games' official PokeRogue](https://github.com/pagefaultgames/pokerogue)**
and its contributors. Gameplay updates come directly from official upstream.
See [NX credits](docs/nx-credits.md) for provenance and licensing.

## Architecture

- Native C host using libnx and SDL2 on Switch.
- Embedded QuickJS-ng runs the official game's JavaScript and Phaser.
- Native EGL/OpenGL ES through Mesa/nouveau uses the Switch GPU.
- Offline game assets are read from one archive without bulk extraction.
- No nx.js or full browser dependency, and no full C++ gameplay rewrite.

## Status and getting started

This is an **offline test port**. Official online login, server saves, and the
authentic online Daily Run are not integrated. Cheats are not included.
The maintainer has played through wave 11 on Switch and confirmed the button
mapping correction. The latest save fix passes host tests and cross-compilation
but still needs repeated save/reopen verification on real Switch hardware.

- [Build and copy-ready Switch installation](docs/nx-build.md)
- [Testing evidence and known limitations](docs/nx-testing.md)
- [Source revisions](game/handheld/upstream.json)
- [Credits](docs/nx-credits.md)
- [Three-mode roadmap: Official, Offline, SilverShadow](docs/nx-modes.md)

## AI assistance disclosure

**AI assisted with coding, debugging, documentation, and testing. No AI-generated
game assets were added for this port.** Switch hardware results were provided by
the human maintainer; AI-assisted builds and tests do not imply full validation.
Read the [full disclosure](AI_DISCLOSURE.md).

## Attribution and project status

This is an independent fan project, not an official Pagefault Games release or
an endorsement by krappaU, Nintendo, The Pokemon Company, Game Freak, or Sony.
Original license and attribution files are preserved. Game assets and third-party
components retain their respective terms; the code license is not a blanket
license for the game's artwork or trademarks.


## Online and offline scope

The upstream Vita client already supports the live pokerogue.net game, accounts,
cloud saves, and offline play. That work is retained as the foundation for future
online support here. Current Switch test builds deliberately enforce offline
mode; upstream Vita functionality is not proof that the same paths are validated
on Switch. Official server coordination, save separation, and device testing are
still required before enabling an online player mode.

## Repository layout

| Directory | Purpose |
| --- | --- |
| `runtime/` | Vita-derived native host, Switch and Android adaptation |
| `game/` | Official PokeRogue subtree and small handheld build/touch adapters |
| `docs/` | NX build, testing, credits, and inherited development documentation |
| `scripts/stage_switch.py` | Copy-ready Switch test folders; no unzip step |

The [original Vita README](docs/upstream-vita-readme.md) and
[official game README](game/README.md) retain upstream information and credits.
Some inherited documents describe Vita capabilities rather than NX test status.
