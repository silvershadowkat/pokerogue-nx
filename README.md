# PokeRogue NX

Nintendo Switch and Android adaptation of **[krappaU’s PokeRogue Vita](https://github.com/krappaU/pokerogue-vita)**. One app offers Online and Offline; Debug is reserved for a later release. **No game files or game artwork are bundled in the APK/NRO.**

krappaU’s native runtime, QuickJS integration, browser APIs, GPU rendering, audio and optimizations are this project's foundation. This is an adaptation of that work. Gameplay and assets belong to **[official PokeRogue and its contributors](https://github.com/pagefaultgames/pokerogue)**.

## Modes

- **Online:** loads live pokerogue.net, including its normal account login and server APIs. Authenticated cloud-save and Daily Run verification on Switch is still pending. No cheats, developer evaluation channel, or save-file import.
- **Offline:** downloads the normal offline distribution used by [Admiral-Billy’s app](https://github.com/Admiral-Billy/Pokerogue-App) when requested. Plays directly from one `game.zip`, without extracting tens of thousands of files. No account-server access or cheats.
- **Debug:** visible but unavailable in 0.2.0-rc1. Isolated SilverShadow cheats, enhancements and custom modes are planned. No minigames or Google Drive integration in this release.

The selector uses the official plains background, logo, window, cursor and Emerald font, downloaded and cached on first use. Mode-specific saves, cookies and caches live in separate profiles. Close and reopen the app to choose another mode; the JavaScript runtime is never reused across modes.

Direct menu taps, optional on-screen controls and physical controllers can be used together. The on-screen controls are drawn in code without generated artwork. Some direct-touch screens still require broader device testing; the pad is available as an alternative.

## Updates

Online fetches the current page/manifests and reuses cached versioned assets. A live update can require a runtime compatibility update; it is not a guarantee of perpetual compatibility.

Offline checks for an update and offers it in the selector. Downloads can resume, and saves are separate from the game archive. **Offline updates currently download the full archive, not binary deltas.** App/runtime updates are separate from game updates.

## Build and test status

0.2.0-rc1 is a candidate, not a claim of completed Switch account testing. Android has reached the live official login screen on a Retroid Pocket Nova with native Adreno rendering. Login touch selection and masked password entry have been checked without submitting credentials. See [testing evidence](docs/nx-testing.md) and the [release checklist](docs/nx-client-release.md).

- [Build and copy-ready installation](docs/nx-build.md)
- [Mode boundaries and roadmap](docs/nx-modes.md)
- [Credits and provenance](docs/nx-credits.md)
- [AI assistance disclosure](AI_DISCLOSURE.md)

## Architecture

Native C host using libnx and SDL2 on Switch, with QuickJS-ng executing the upstream JavaScript/Phaser game. EGL/OpenGL ES through Mesa/nouveau uses the Switch GPU. Android uses SDL2 and the device's OpenGL ES driver. No nx.js or WebView is used. This client is separate from the full C++ gameplay rewrite experiment.

`runtime/` contains the client. `game/` retains the earlier official source snapshot and small touch adapter source for review/regeneration; **release packaging does not build or bundle that game**. See [the upstream Vita README](docs/upstream-vita-readme.md) for its original project description.

## AI assistance and attribution

**AI assisted coding, debugging, documentation and testing. No AI-generated game assets were added.** Automated checks and Android tests do not establish Switch hardware performance. Original history, licenses and attribution are preserved; assets retain their own terms.

This is an independent fan project, not an official release or endorsement by Pagefault Games, krappaU, Nintendo, Sony, The Pokémon Company or Game Freak.
