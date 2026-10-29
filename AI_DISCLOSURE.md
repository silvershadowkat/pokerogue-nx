# AI assistance disclosure

OpenAI Codex has been used extensively to assist with code investigation,
implementation, debugging, build scripts, documentation, and automated testing
for the NX port and handheld integration.

**No AI-generated art, sprites, music, sound effects, or other game assets have
been added as part of this port.** Existing game assets come from the upstream
projects and retain their original credits and license terms. This statement
describes this port's contributions, not an audit of every upstream asset's
creation process.

AI assistance is not a substitute for validation or a guarantee of correctness.
The maintainer is responsible for the published changes. Build checks and
automated tests are reported separately from manual device testing:

- Codex assisted with local PC builds, automated tests, and Android testing.
- The maintainer performed the reported Nintendo Switch hardware tests and
  supplied the runtime logs. Codex did not independently operate that Switch.
- A successful build or a limited play session does not establish full gameplay
  parity, long-session stability, or compatibility with every device.

See [NX testing status](docs/nx-testing.md) for the current evidence and remaining
checks. Please report reproducible problems through this repository's issues.
