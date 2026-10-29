# SilverShadow migration inventory

Initial read-only inventory of the previous local project. Minigames were
explicitly removed from the current scope on 2026-10-29. These enhancements
have **not yet been imported or implemented in NX**. The old project's files and
installed saves are preserved. This list is not a full feature audit.

## Active scope: cheats, enhancements, and custom runs

- Reorganize cheats into Battle, Capture, Progression, Rewards/Economy,
  Starters/Collection and Encounters, with clear effects and reset controls.
- Reimplement the existing quality-of-life settings and Pokemon editing tools
  only inside SilverShadow mode; preserve official rules in the other modes.
- Custom Run and Random Run variants of 5/10/20/30/50/100 waves, including
  their proper save, history and resume identities.
- Boss Rush Normal and Hard: three level-100 starters, ten deterministic
  level-100 bosses, escalating shields, no capture or escape, separate rewards
  and shops, and the different between-battle healing rules.
- Deterministic date-based Offline Daily Run with calendar selection; distinguish
  it clearly from the authentic official online Daily Run/feed.
- One-way online-to-offline copy and isolated manual local import/export.
- No Google cloud-save integration. No minigame code, UI, currency, music or
  assets in this implementation phase.

These are requirements recovered from the previous project's documented behavior,
not a claim that every feature is already validated or implemented here.

## Deferred minigame attribution records (not included in NX)

- **SolVolrund**: [pokerogue-2p-beta](https://github.com/SolVolrund/pokerogue-2p-beta),
  revision `f921532241d52032e36c4297b2963b7cc85daf48`, and its matching
  [asset repository](https://github.com/SolVolrund/pokerogue-assets), revision
  `8c957d481a10bd026446ab002a5bd7a96aab0141`. The previous port adapted mining
  and the 4x8, three-hit Voltorb game. Preserve its AGPL source notices and
  per-asset attribution when adapting these features. Additional mining-resource
  author attribution must be checked in the retained source material.
- **RaFa / rafa.ssbm**: the previous port credits the public RaFaRogue client
  for casino visual implementations, including Slots, Plinko, Blackjack,
  Roulette and Crash. Its locally archived source/provenance must be reviewed
  before reuse; do not guess a GitHub account or silently remove that credit.
- **Sabotage Studio / Sea of Stars**: design inspiration for the custom Wheels
  minigame. The previous implementation used original PokeRogue character assets
  and custom rules/AI; it did not bundle Sea of Stars artwork, audio or executable
  code. Preserve this distinction and the original PokeRogue art credits.

## Deferred minigame inventory

- Manual-stop Slots and the separate RaFa Slots variant.
- Plinko, Blackjack, Roulette and Crash; saved rounds and once-only settlement.
- Adapted Voltorb game, classic Voltorb variant, Mining, Wheels and seahorse racing.
- Local minigame wallet, board/match persistence, collection/progression, pause,
  touch/controller parity, and audio lifecycle behavior.

## Asset exclusions

The old Plinko provenance explicitly identifies six chip images as generated
with image_gen. They are excluded from this new port. This keeps its no-AI-assets
disclosure accurate. Custom music in the local project also requires provenance
and redistribution review; use existing credited upstream audio in the meantime.

No old application merge, Google cloud-save integration, automatic account
upload, or repository archival is part of this inventory step.
