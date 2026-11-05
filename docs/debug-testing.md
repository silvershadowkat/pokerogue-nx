# 0.3.0-beta.1 candidate evidence

Checks performed on 2026-11-05. This is development-candidate evidence, not a
new public release or a claim that every cheat or battle combination was tested.

## Source and host checks

- Pinned official `f045bef687ee7a60faa94679939d5d9e074cb19d`, game 1.12.2.4,
  with matching asset/locale pins recorded in `debug/upstream.json`.
- Full patch applied successfully to a second clean official checkout; all
  74 modified/new source and test hashes matched the reviewed source map.
- Portable Debug builder completed TypeScript checking, all **90 tests** in
  four editor/seeded-run test files, Vite compilation and archive validation.
- Actual editor integration regressions cover replacement pages, numeric bounds,
  full-list paging without skipped moves, duplicate starter copies, preserving
  custom move drafts and visible-parent message routing.
- Native C tests cover version identifiers, malformed/oversized packages,
  pinned/current/legacy archive discovery, production ZIP overlay lookup,
  executable fallback rejection and Online/Offline/Debug boundaries.
- Runtime JavaScript checks pass for plaintext/encrypted Debug import rejection,
  stock imports, official Online import restriction, integer save timestamps,
  interrupted storage recovery, direct-touch/modal controls and startup feedback.

The package receipt records checks performed by that specific invocation.
Packaging an already compiled `dist` does not assert fresh compiler/test results.

## Retroid Pocket Nova Android smoke

Fresh local installation used the explicitly authorized new release identity.
The device is an actual Retroid Pocket Nova with Adreno 740 GLES 3.2. APKs ship
no gameplay/assets; a stock archive and small version-matched Debug overlay were
staged separately and their whole-file hashes checked.

Verified:

- Familiar native selector, PokeRogue name/version and existing app icon.
- Debug boots using shared stock assets and shows a distinct Debug game label.
- Welcome/tutorial, main menu, Debug categories, setting selection and return.
- Full Editor enabled, level changed from 5 to 15 using numeric paging, move
  browser reached with 831 implemented moves, Absorb added and applied.
- Saved a Pokemon build. Started Classic with the edited Bulbasaur, used its
  added move, won battles and reached the reward screen and wave 2.
- Closed/reopened and upgraded the candidate in place; Continue resumed wave 2
  with level/moves preserved. One saved build remained in the isolated Debug
  system save. Save and Quit returned to the title.
- Custom seeded-mode selector reached. Public Daily archive downloaded and
  reported latest date 2026-11-05 without account access.
- Stock Offline booted its fresh welcome screen from the exact stock ZIP in a
  separate profile. Online independently reached official Login/Register; no
  account was entered during this test.

Idle title samples near 60 FPS on this Android device do not establish battle
performance, long-session stability or Nintendo Switch results. One nonfatal
duplicate upstream texture-name warning was observed after repeated run resumes;
no crash or unhandled JavaScript exception accompanied it.

## Remaining coverage

Individual cheats, every editor filter/form, complete seeded runs, manual
export/import through Android's system picker, long sessions and new Switch
hardware behavior require further testing. Automated capability checks do not
replace a real official-account cloud-save round trip. Prior RC7 account/save
confirmations remain documented in [NX testing](nx-testing.md); they are not
new 0.3.0 hardware results. The public Debug download channel is not published.

Task-created local test progress is removed before final Android handoff;
downloaded archives are retained. Private saves, credentials, cookies and signing
material are excluded from source, receipts and release artifacts.
