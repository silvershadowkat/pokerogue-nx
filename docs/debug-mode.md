# Debug mode in 0.3.0-beta.4

The app label is **PokeRogue**. Online, Offline and Debug share one native client,
but have separate game instances, saves, cookies and caches. Mode selection
requires restarting the game instance. Existing native touch and controller
behavior is retained; no legacy controller implementation is added by Debug.

- **Online** loads the live official game. It cannot import local save files or
  load the Debug archive. Account authentication and cloud saves use the normal
  official service.
- **Offline** loads the stock downloaded offline distribution with no cheats.
- **Debug** loads a compiled SilverShadow overlay over an exact pinned stock
  archive. Menu > Debug Tools exposes grouped local cheats and editor settings.
  Full Editor adds Pokemon editing, unrestricted move editing and saved builds
  to the starter/party menus. Custom runs retain their versioned seeds and
  snapshots, including daily date selection, Boss Rush, Custom and Random modes.

Debug account/API requests are blocked by the native host. The one additional
network exception is a read-only public daily-seed archive, not an authenticated
Daily Run or account service. Local save import/export remains available.
Google Drive, minigames and legacy controls are excluded. Do not upload Debug
saves to official accounts. Deliberately modified clients require server-side
validation; these boundaries do not make an executable tamper-proof.

Debug system and run exports carry a provenance marker. The client's stock
Offline file picker rejects marked plaintext/encrypted exports; Debug accepts
them. This catches normal exports from this build, not manually stripped or
third-party modified save data. The official Online import boundary remains
disabled entirely.

## Installation and updates

The client ships without game files. Offline downloads a stock ZIP on request;
Debug requires its small overlay and matching stock base. Base versions are
stored under `games/<version>.zip`; `game-current.txt` selects the normal Offline
version. `debug-game.zip` pins its own base, so a stock update does not silently
change Debug rules, abilities or seeded behavior. Legacy `game.zip` installations
are still recognized. Saves are outside game archives.

The Debug updater expects a GitHub release at tag `debug-game` in this repository,
with asset `debug-pack.zip`. Its **release name** must equal the overlay's unique
`version` identifier. The native downloader validates the marker and fetches the
exact Admiral-Billy `release-<baseVersion>` when that base is missing. The
selector offers updates rather than applying them during a run. Current packs
are about 1.6 MB; a different stock base still requires the full roughly 543 MB
archive. Binary/per-asset delta downloads are not implemented.

The public `debug-game` release supplies the compatible overlay. Select Debug to
download it and, on first use, its matching stock base. Existing Debug saves stay
in their own profile. Current base: `games/915fae1.zip`. No extraction is needed.
The app offers future compatible Debug updates in the selector. If the channel
cannot be reached, an installed compatible pack remains playable.

See [source map and rebuild instructions](../debug/README.md),
[testing](debug-testing.md) and [Android signing](android-signing.md).
