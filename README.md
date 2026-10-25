# bbport World Warper

A shader-cache builder for Supermedo's native Bloodborne Windows port. It runs the exact renderer used for normal play, but on an isolated copy of the save while writing shaders and pipelines into the player's real GPU cache.

The project website is maintained as a dependency-free GitHub Pages site in `docs/`.

## Download and run

[Experimental Windows prerelease](https://github.com/OmgItsEggy/bbport-world-warper/releases/tag/v0.1.0-alpha.1)
— download `windows-x64.zip`, extract separately from bbport, then double-click
**Start-WorldWarper.cmd**. [Full instructions for you and your friend](QUICKSTART.md).

The default **Full warm-up** runs weapons/armour, all 44 lantern destinations,
then the experimental boss/phase batch automatically. It uses the embedded Python
and runtime libraries in your existing Supermedo **v1.9.4** installation. No build
tools or separate Python install are needed.

The launcher imports [the original test-save ZIP](https://www.nexusmods.com/bloodborne/mods/619?tab=files&file_id=3156)
locally and resets a disposable copy before **every** game launch. The save author's
permissions forbid reuploads, so the save cannot be bundled without permission.
Never install it over your personal save.

**Experimental, not complete shader coverage.** v4 fixes authored first entry and
real wake-up hits; actual battle events have now been confirmed for Cleric Beast,
Gascoigne, Rom and Ebrietas. Several other boss/phase paths still need work.
See [current status](docs/STATUS.md) and [the script audit](docs/BOSS-SCRIPT-AUDIT.md).

The optional short `--entry-check` diagnostic checks only Cleric Beast, Gascoigne,
Rom and Ebrietas. Do not repeat the whole boss tour to debug their activation.

## Target

- Base: `Supermedo/bloodborne_pc` tag `windows-v1.9.4` (`3fabfa7a7af67a34a6dfc630b6ec2539b68fad4d`)
- Reference implementation: `visy/bloodborne_pc_windows_port` tag `v0.7.5` (`6e0dec4ff4d2f19d86b97298e2382062f534275e`)
- Game data: Bloodborne 1.09
- Licence: GPL-3.0-or-later, because the imported world-warper implementation is GPL-3.0-or-later

## Intended coverage

The finished run is deliberately hybrid rather than a single synthetic map:

1. Visit real maps through ReturnPointParam locations so map materials, lighting, geometry and render passes are compiled.
2. Exercise camera sweeps, movement, dodges, both weapon forms, charged attacks and firearms.
3. Rotate 34 armour appearances, 26 right-hand weapon models and 15 left-hand weapon models.
4. Trigger usable item effects and Hunter tools in a controlled pass.
5. Find nearby enemy kinds, exercise their rendering, attacks and death effects, and optionally include boss arenas.
6. Cover cutscenes and time-of-day variants. Chalice Dungeons are out of scope.
7. Replay representative areas before and after the tour to measure remaining compilation stutter.

A custom shader gallery may be added later for repeatable effect coverage, but it cannot replace real-map traversal: a gallery alone misses map-specific materials, lighting combinations, visibility state and encounter pipelines.

## Repository layout

- `reference/`: pinned upstream files used for analysis; these are not edited.
- `src/`: the Supermedo-facing implementation and portability layer.
- `tests/`: unit and source-contract tests.
- `tools/`: dependency and coverage audits.
- `docs/`: architecture, milestones, compatibility notes and the GitHub Pages site.

## Current status

The equipment pass and 44-destination lamp tour have completed in actual game
tests, including automatic load/exit and resumed runs. The boss controller uses
closed-loop grounded walking, actual battle/hit-event gates and event-selected
forms/health owners. Its forward and sideways stick directions are measured
independently; a failed attempt is not reported as confirmed coverage. Item/tool
gallery, full attack coverage, group phases and ending entries are not complete.

Release source/build details: [BUILD.md](docs/BUILD.md). Corresponding source is
provided alongside the Windows package; no game files, live saves or GPU caches
are distributed.

## Ground rules

- Before authoring encounter health levels or phase handling, consult [Boss phase and coverage reference](docs/BOSS-PHASE-REFERENCE.md). Keep documented triggers, implementation requests and verified in-game outcomes separate.
- Never write to the player's live save. The launcher copies the selected save to a dedicated warper folder.
- Use `BB_GPU_USER_DIR` for the real shader cache and `BB_USER_DIR` for the temporary save copy.
- Refuse to start if the Bloodborne 1.09 hook signatures do not match.
- Store resumable progress outside the save and flush it after every location.
- Online/party mode remains disabled during an automated run.
- A cache warm-up reduces compilation hitches; it does not increase steady-state GPU-limited FPS.

