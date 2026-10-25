# Development status

## Complete

- Pinned Supermedo v1.9.4 and Visy v0.7.5 by tag and commit.
- Imported the world-warper state machine, pure rules, generated gear table and upstream tests as immutable references.
- Imported Supermedo's benchmark and launcher sources needed for integration analysis.
- Added a dependency audit that classifies every project include used by the reference core.
- Imported the portable route, option, progress, action, lamp and equipment rules into `src/native`.
- Implemented and tested launcher-side save isolation, resume state parsing, online-variable removal and real-cache selection.
- Defined the solo native backend contract; it contains no party/session/networking API.
- Ported in-process button/stick injection into Supermedo's runtime pad layer.
- Added guarded guest-memory reads and writes using Supermedo's runtime fault-recovery path.
- Added bounded image binding and byte-signature verification for Bloodborne 1.09 hook sites.
- Added a minimal solo main-thread detour for the byte-verified `SprjFlipper::Update` entry.
- Added the exact Supermedo loader patch that registers the unpatched image and its size.
- Added a responsive GitHub Pages project site under `docs/`.

## In progress

- Recover ReturnPointParam lookup and loaded-enemy enumeration without Visy's party layer.
- Connect Supermedo's pipeline counters to `CompileCounters`.
- Encounter-aware coverage: distinguish cutscenes, missing bosses and active combat from timed scenery passes.

## Live validation — 25 October 2026

- Combined equipment and lamp run reached all 44 destinations across resumed sessions: 44 `done`, no `skip` records. Final log confirms stop 44/44 and automatic exit request.
- Initial gear pass logged all 34 equipment rows verified, with zero equip-verification failures. This verifies equipment assignment, not successful rendering of every action/effect.
- Gear completion now persists across restarts; the disposable save still resets each launch. Prior session logs are archived.
- User observed Celestial Emissary, Ebrietas, Blood-starved Beast, Darkbeast Paarl and other bosses alive. Father Gascoigne and Ludwig were not seen.
- User observed entrance cutscenes for Martyr Logarius, Mergo's Wet Nurse, Laurence and Orphan of Kos; inputs appeared to wait until they ended. The new build explicitly pauses gear/scenery clocks and neutralizes input while the game's cinematic flag is set, logging both transitions. This behavior has synthetic tests but awaits in-game validation.
- New build records a bounded, read-only enemy census at each scenery pass's start/end: entity IDs, handles and HP, with living/dead totals. Loaded presence is explicitly not a claim of visibility, boss identity, attack playback or shader coverage. Pipeline compilation counters and controlled encounter actions are not wired yet.
- User observed abnormal floor rendering in Rom's arena; cause not established.
- First Cainhurst rooftop attempt crashed with Vulkan device lost during submit, preceded by texture-memory pressure warnings. A later resumed run completed the destination; underlying crash cause remains unresolved.
- A `done` record means the timed routine finished, not that all shaders, boss phases, visible scenery or effects were exercised.

## Not started

- Refactor the upstream state machine onto `Backend`.
- Comprehensive item effects, enemy attack/death and boss-phase passes; launcher page.
- Dedicated Hunter tools, controlled cutscene, time-of-day and Chalice passes.

