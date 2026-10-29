# PokeRogue NX credits

## Native runtime: krappaU

The native runtime is based on **[krappaU's PokeRogue Vita](https://github.com/krappaU/pokerogue-vita)**.
Its QuickJS-based execution environment, browser compatibility work, native
rendering and audio infrastructure, and existing optimizations are the foundation
of this port. We are adapting that work for Switch and Android, not claiming to
have created it from scratch.

Imported runtime revision: `01e917799043101c37c07fb7e5a5901b1de8e397`.
This repository is a direct fork of that project. Its Git history, license,
attribution and original documentation are preserved. The original README is
retained at [upstream-vita-readme.md](upstream-vita-readme.md).

## Official gameplay: Pagefault Games and contributors

**[Official PokeRogue](https://github.com/pagefaultgames/pokerogue)** supplies
the gameplay code, game data, and upstream asset/localization references.
See the original credits in the `game/README.md`, the assets and locales repositories,
and their individual attribution/license files.

This port takes official gameplay updates directly from Pagefault Games. Scooom's
offline fork and the maintainer's earlier mod/minigames project are not inputs
to this new build. Those projects remain separate.

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
