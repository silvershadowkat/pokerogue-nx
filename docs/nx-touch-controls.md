# SilverShadow controller in the native client

0.2.0-rc3 restores the retained SilverShadow chevron controller. It replaces the temporary A/B/X/Y grid. This is a presentation/input adaptation, available in standard Online and Offline without cheats or save modifications.

## Source and mappings

The behavioral reference is the old project's independent `silvershadow-touch-input.ts` and `silvershadow-dpad-visual.ts`, with its SVG/CSS integration patch. Exact reference sources/hashes are retained under `runtime/tests/reference/`. Provenance: silvershadowkat/pokerogue-offline at `878335241e98f034a1c46bcb4cabffa039cfb397`; actual source bytes are identified by SHA256 in `silvershadow-provenance.json`.

The input foundation was adapted from [Gen1Recomp](https://github.com/bryanthaboi/gen1recomp), dev reference `8e5501a`. Its MIT notice is retained in [THIRD_PARTY_TOUCH_NOTICES.md](../THIRD_PARTY_TOUCH_NOTICES.md). The native client uses the SilverShadow vector paths directly; no fallback PNGs, minigame content or old application patch pipeline were imported.

| Original | Native mapping |
| --- | --- |
| Pure touch state/resolver | `runtime/src/touch_controls.c` |
| Visual-pose helper | `tc_visual_pose`, separate from digital resolution |
| SVG cross, cap, groove, four open chevrons/material | `runtime/src/touch_art.c` |
| CSS perspective, pose/opacity transitions | `runtime/src/touch_overlay.c`, cached GPU layers |
| Existing keyboard/touch input event contract | `runtime/js/touch-events.js` |
| Contextual action visibility | Native mask driven by the current upstream UI mode |

## Preserved input rules

One pointer owns the complete D-pad, including a neutral-center touch. Sliding outside remains captured. The square dead zone is 16% of the stable 84%-width input reference. Horizontal dominance wins exact axis ties. Diagonal positions lean on both visual axes and light adjacent chevrons, while the game receives exactly one cardinal direction.

A direction change releases the old key before pressing the new key. Unchanged movement adds no transition; repeats remain 250 ms. Action buttons keep their original pointer ownership while dragging outside; independent held counts prevent one finger from releasing another's button. Action hit slop remains 15%. Focus loss, scene changes, cancellation, disable and resize clear held input.

The native pad emits the same logical keyboard/touch events as the original. It no longer merges into a physical gamepad, so changing controller bindings does not remap the touch controls. Physical control polling stays on its existing path. Direct taps outside captured control regions stay available.

## Visuals and performance

The cross/chevron paths and pose constants are retained: 96% stable visual wrapper, 8.5-degree tilt, one-degree neutral micro-tilt, four-pixel shadow displacement, 1.4% maximum compression, original primary/secondary light weighting, and 38 ms movement/115 ms release easing. Idle/neutral/active opacity remains 0.3/0.4/0.58.

Seven small vector textures are warmed before first input. A cached GPU framebuffer composites their group opacity correctly. Rocking and lighting change GPU geometry/uniforms; D-pad movement does not rasterize/re-upload the control image. Static poses reuse their GPU composite. Action layers update only on pressed state, visibility or dimensions. Fully faded controls add no rendering loop, while their hit regions can still wake on the first touch.

The launcher cycles **Touch pad: Off / Fade / Always**. Fade hides after two seconds without fingers down, using the original 180 ms fade. Direct menu taps remain a separate option. Previous `touchpad=1` preferences now mean Fade; `2` means Always. RC5 fresh installs default to Fade; existing Off/Fade/Always preferences are preserved.

## Verification and remaining device work

The maintainer's RC3 device test found that continuous sliding felt unresponsive. RC4 fixes a native animation timing bug: a new target restarted interpolation before sampling the running transition, so movement every frame froze the displayed pose until the finger stopped. The renderer now advances the existing transition before accepting the next target, preserving the original 38/115 ms timings. The original input resolver, ownership, repeat rate and artwork remain unchanged.

A new actual-GLES regression moves one captured finger around a complete circle at 30, 60 and 120 updates per second without stopping or lifting. It checks that displayed pixels continue changing, direction state updates immediately, and no control textures upload while moving. The continuous-motion assertion fails with the RC3 renderer and passes with the fix. The maintainer subsequently tested installed RC4 on Retroid and reported that it now felt smooth.

Differential tests compare 15,045 native resolver/pose cases and ordered multi-pointer traces with the retained original TypeScript. Tests also cover touch-event repeat/mapping, contextual controls, scene cleanup, 4:3 layout, the native GLES shader, group opacity, fade/wake and lifecycle release. The native renderer test confirms zero texture uploads while sliding/rocking after warm-up. Headless Mesa checks are functional graphics checks, not Switch performance evidence.

The original browser Move Touch Controls editor/saved layouts and native haptics are not integrated in this candidate. The native client currently uses the original default geometry/material with context-sensitive groups. Retroid feel is maintainer-confirmed; Switch thumb-slide and feel testing remains pending.

Build the reference driver with a host C compiler and `-lm`, then run `runtime/tests/touch_reference_test.cjs` with TypeScript available to Node. `PKR_TOUCH_REFERENCE_DRIVER` can override the driver path; Windows defaults to WSL Ubuntu, Linux runs it directly.
