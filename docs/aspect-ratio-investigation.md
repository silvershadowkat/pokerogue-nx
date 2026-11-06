# Nova 4:3 investigation, 2026-11-06

This is a source review and implementation plan. No gameplay aspect-ratio change
is included in the 0.3.0-beta.2 input fixes.

## Why the bars exist

Android already reads the actual display dimensions in
`runtime/android/NativeActivity.java`. The native `gl.c:fb_map` fits and centers
the game's canvas while preserving its aspect ratio. The game's pinned official
source (`f045bef687ee7a60faa94679939d5d9e074cb19d`) explicitly creates a
1920x1080 Phaser FIT canvas in `src/main.ts`. `src/scene-base.ts` independently
hardcodes `scaledCanvas` to 320x180, using the game's six-times scale convention.

On the Nova's observed 1280x960 screen, the game therefore occupies 1280x720:
120 unused pixels above and below. A true 4:3 layout at the same logical width
would be 320x240, with a 1920x1440 internal canvas. It needs 60 additional logical
rows, not a different screen-size detector. Stretching would distort sprites;
zooming to fill would crop 25% of the original horizontal content.

## What can be reused

Many menus already reference `globalScene.scaledCanvas.width/height`. Native
viewport/scissor mapping, game-to-screen conversion and the direct-touch adapter
also derive coordinates from the actual canvas dimensions. These are useful
foundations, provided one layout metric is used consistently.

However, changing those dimensions alone is insufficient:

| Area | Source evidence / work required |
| --- | --- |
| Shared layout | Replace both fixed canvas configuration and `SceneBase.scaledCanvas` with one dimension contract. Handle resize/safe areas, not only initial orientation. |
| Battle field and HUD | `BattleScene` builds its field container at `game.canvas.height`, scales UI/field containers by six, and mixes logical anchors with fixed actor/arena coordinates. Define battle and HUD rectangles together. |
| Starter, party and summary | The starter screen mixes height-derived containers with fixed grid/scrollbar positions and atlas backgrounds. More space must produce intentional layout, not a gap. |
| Artwork | Existing full-screen/background textures are composed for the original layout. Use existing texture panels, repeatable regions or carefully anchored backdrop extensions; keep Pokemon and text proportions unchanged. No new AI artwork. |
| Battle animations/effects | Animation code includes fixed background rectangles and frame coordinates. Preserve a canonical battle coordinate space and map effects, actors, masks and camera offsets together. |
| Input/keyboard/modals | Recheck hit regions, field overlays, safe areas and controller focus after layout changes. The beta.2 pad/direct-touch policy remains independent of aspect ratio. |
| Native selector/loading | Its current design uses a separate fixed 960x540 canvas. A 4:3 game alone would not make that selector responsive. |

## Recommended approach

1. Introduce explicit layout metrics and retain the exact current 16:9 result.
   Detect available width/height and choose an aspect profile; avoid changing
   battle rules, seeds, save serialization or progression.
2. Prototype optional 4:3 in the pinned Debug build. Begin with title/menu,
   starter selection and one complete battle/reward flow. Reuse a canonical
   battle region while reflowing the surrounding UI, then address each full-screen
   scene: party/summary, shop, eggs, Pokedex, custom runs, modals and transitions.
3. Add before/after layout and hit-test checks at 1280x960, 1280x720 and a wide
   phone size. Include long labels, double battles, large/fused sprites, move
   effects and editor/seed lists. Compare seeded replay outputs to confirm only
   presentation changed.
4. Measure actual GPU/frame time and loading on hardware. Filling 1280x960
   instead of 1280x720 shades up to 33% more pixels; this is not a prediction of
   a 33% FPS change. Keep Switch widescreen behavior as the regression baseline.
5. Preserve stock Online while the local implementation matures. Online fetches
   a live bundle; applying a responsive rewrite there introduces another set of
   private layout assumptions to maintain across updates. The clean long-term
   route is upstream responsive support, or a reviewed, version-checked
   presentation adapter with a safe stock fallback.

An optional decorative fill using existing artwork could hide black bars with
much less work, but would not give the game a true expanded 4:3 layout. A genuine
4:3 version is feasible as a staged presentation project, not a one-line resize.
