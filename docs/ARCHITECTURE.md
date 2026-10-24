# Architecture

## Why this is a source feature

Supermedo's benchmark runner can replay controller recordings from prepared saves, but it cannot discover lamps, change game state, enumerate loaded enemies or cycle equipment by itself. A route pack remains useful as an early cache experiment; near-complete coverage needs an in-process state machine.

The Visy implementation already has the correct high-level state machine. Its rendering work is produced by the game, not by synthetic SPIR-V compilation, so the cache keys match the running port. The porting task is therefore to retain that state machine while replacing its dependencies on Visy's multiplayer subsystem.

## Components

### Warper core

Owns phases, progress, input sequences, area traversal, coverage counters, retries and shutdown. The first import stays behaviour-compatible with Visy v0.7.5.

### Supermedo backend

Provides a narrow solo interface:

- image-base translation and guarded game-memory reads/writes;
- main-thread tick hook and Bloodborne 1.09 signature checks;
- game-state snapshot and title/menu text detection;
- ReturnPointParam enumeration, coordinates and lamp warp;
- game-param lookup;
- loaded-enemy census, position and HP;
- shader/pipeline counters, logging and cache flush/exit.

This backend is the key boundary. No networking, session, guest, replication or party-director code belongs in it.

### Launcher integration

The launcher creates a copy of the selected save, points `BB_USER_DIR` at that copy, keeps `BB_GPU_USER_DIR` pointed at the normal cache, sets `BB_WORLD_WARPER` and `BB_WORLD_WARPER_STATE`, starts the game and displays progress. Cancel leaves the completed cache intact and the original save untouched.

### Optional shader gallery

A deterministic gallery can make effect coverage faster and easier to validate. It should be hosted in a real loaded area and supplement—not replace—the tour. Its useful targets are weapon transformations, projectiles, consumable SpEffects, Hunter tools, status effects and selected enemy attacks.

## Development sequence

1. Import and test the pure route/options/progress/action rules.
2. Implement guarded memory access and image-offset translation.
3. Add title continuation, one-location lamp warp and resumable progress.
4. Expand to the full real-area tour with camera/movement actions.
5. Add gear and consumable effect passes.
6. Add enemy census/kill coverage, then optional bosses.
7. Add Hunter tools, cutscenes, time-of-day and Chalice passes.
8. Add launcher UI, cache export/import and before/after validation replay.

## Main technical risk

Visy's world warper depends on helpers built during its multiplayer work. Most are generic game-memory utilities in disguise, but they must be reimplemented against Supermedo's runtime. Importing the whole party tree would be faster initially and worse permanently: it would increase hook conflicts, binary size and maintenance surface.

