# Three-mode product plan

Requested 2026-10-29. This is the implementation specification, not a claim that
the modes already exist. The current build is the offline test described in
[nx-testing.md](nx-testing.md). Switch is the first target; Android builds remain
part of the project. Builds and validation are local; inherited GitHub Actions
remain disabled in this fork.

The startup screen will offer three explicit choices, usable with controller or
direct touch. Each mode gets its own profile, storage directory, cookie/auth
scope, and capabilities. Changing mode must fully tear down the previous game
instance so hooks, cheats, credentials, and pending network requests cannot leak
into another mode.

| Capability | Login to Official | Offline | SilverShadow Build |
| --- | --- | --- | --- |
| Official account login and cloud save | Yes, after compatibility validation | No | No |
| Official Daily Run/feed | Official authenticated service | No claim to be official daily | No claim to be official daily |
| Standard gameplay | Official live client | Pinned official offline bundle | Official base plus explicit enhancements |
| Cheats/debug input/evaluation | Disabled | Disabled | Separate, redesigned cheats menu |
| Manual save import | Never | Yes, including a copy exported from online | Yes, isolated enhanced profile |
| Manual save export | One-way offline copy where supported by official APIs | Yes | Yes |
| Google cloud saves | No custom Google integration | No | No |
| Enhanced minigames | No | No | Yes, with original credits |

## Online boundary

Reuse and adapt the upstream Vita client's live-game/account infrastructure,
targeting the normal online-client experience of Admiral-Billy's app. Do not
label it working on Switch until authentication, TLS, session renewal, official
Daily Run/feed retrieval, and server save/reload have been tested.

Disable import at both the UI and native bridge boundaries, not just by hiding a
button. Never upload an offline or enhanced profile, never reuse its local
storage as official progress, and do not expose native debug/evaluation hooks.
Server-issued authentication state belongs only to the online profile.

These client restrictions are not proof against a modified executable. Coordinate
with the official developers on compatibility and server-side validation before
claiming protection against hacked online saves. Do not invent a new trusted
client assertion or silently bypass official authentication.

## Save transfers

Online to offline is a copy/export operation, never an account conversion or a
two-way sync. The original server save remains unchanged. Offline and SilverShadow
exports must never be offered as online imports. Standard offline mode must not
apply enhanced settings, currencies or minigame rewards from an imported profile.
An enhanced export can be imported into SilverShadow mode; any future conversion
to standard offline requires explicit validation and removal of enhanced state.

Retain official serialization/validation where possible, version local extensions,
preserve backups during migration, and test interrupted writes and mode changes.
Remove the custom Google cloud-save experience from the new product. Keep manual
import/export clear and available in the two local modes.

## SilverShadow reimplementation

Use the old project as a feature inventory and behavioral reference. Do not merge
the Scooom-derived application or its patch pipeline into this fork. Implement
small explicit modules against the current official source and keep the standard
and online bundles free of cheat code. Retain attribution for any adapted code;
rewriting integration does not erase upstream credit or license obligations.

Proposed cheats categories: Battle, Capture, Progression, Rewards/Economy,
Starters/Collection, and Encounters. Each entry should explain when it takes
effect, its limits, and whether it changes saved progress. Provide per-category
reset and a clearly labeled master disable. No cheat applies outside this mode.

See [nx-migration-inventory.md](nx-migration-inventory.md) for source attribution
and feature discovery. No AI-generated game assets are to be added. Reuse
appropriately credited existing assets; do not copy generated artwork or custom
music with unestablished redistribution terms from the old local build.

## Delivery sequence

1. Verify repeated Switch save/reopen on the 0.1.3 fix.
2. Implement isolated mode profiles, lifecycle switching, and capability tests.
3. Validate the inherited official online path and one-way export boundary.
4. Reimplement enhanced settings/cheats and minigames in SilverShadow mode.
5. Test Switch and Android, including direct touch and controller navigation.
6. Archive the old project only after replacement feature acceptance and save
   migration are complete. Do not archive or modify the old project now.
