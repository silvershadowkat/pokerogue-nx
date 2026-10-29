# SilverShadow migration inventory

Initial read-only inventory of the previous local project. These enhancements
have **not yet been imported or implemented in NX**. The old project's files and
installed saves are preserved. This list is not a full feature audit.

## Minigame attribution found in the old records

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

## Features to audit and reimplement

- Manual-stop Slots and the separate RaFa Slots variant.
- Plinko, Blackjack, Roulette and Crash; saved rounds and once-only settlement.
- Adapted Voltorb game, classic Voltorb variant, Mining, Wheels and seahorse racing.
- Local minigame wallet, board/match persistence, collection/progression, pause,
  touch/controller parity, and audio lifecycle behavior.
- Existing quality-of-life settings and live cheats, including battle, capture,
  starters, progression and reward controls. Reorganize the menu rather than
  preserving the old long settings list.

## Asset exclusions

The old Plinko provenance explicitly identifies six chip images as generated
with image_gen. They are excluded from this new port. This keeps its no-AI-assets
disclosure accurate. Custom music in the local project also requires provenance
and redistribution review; use existing credited upstream audio in the meantime.

No old application merge, Google cloud-save integration, automatic account
upload, or repository archival is part of this inventory step.
