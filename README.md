# bbport World Warper

A shader-cache builder for Supermedo's native Bloodborne Windows port. It runs the exact renderer used for normal play, but on an isolated copy of the save while writing shaders and pipelines into the player's real GPU cache.

The project website is maintained as a dependency-free GitHub Pages site in `docs/`.

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
6. Cover cutscenes, time-of-day variants and optional Chalice Dungeon rows.
7. Replay representative areas before and after the tour to measure remaining compilation stutter.

A custom shader gallery may be added later for repeatable effect coverage, but it cannot replace real-map traversal: a gallery alone misses map-specific materials, lighting combinations, visibility state and encounter pipelines.

## Repository layout

- `reference/`: pinned upstream files used for analysis; these are not edited.
- `src/`: the Supermedo-facing implementation and portability layer.
- `tests/`: unit and source-contract tests.
- `tools/`: dependency and coverage audits.
- `docs/`: architecture, milestones, compatibility notes and the GitHub Pages site.

## Current status

The exact upstream revisions are pinned and the complete world-warper core, rules, generated equipment table, launcher tests and Supermedo benchmark code are present under `reference/`. Save isolation, real-cache selection, in-process controller injection, guarded memory access and bounded image/signature checks are implemented and tested. The next milestone connects the image binding to Supermedo's patch entry point and installs the verified main-thread hook.

## Ground rules

- Never write to the player's live save. The launcher copies the selected save to a dedicated warper folder.
- Use `BB_GPU_USER_DIR` for the real shader cache and `BB_USER_DIR` for the temporary save copy.
- Refuse to start if the Bloodborne 1.09 hook signatures do not match.
- Store resumable progress outside the save and flush it after every location.
- Online/party mode remains disabled during an automated run.
- A cache warm-up reduces compilation hitches; it does not increase steady-state GPU-limited FPS.

