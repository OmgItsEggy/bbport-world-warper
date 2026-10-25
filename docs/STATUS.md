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
- Validate new cumulative shader/pipeline compile telemetry in-game (independent of upstream's periodically reset frame-stat counters).
- Encounter-aware coverage: distinguish cutscenes, missing bosses and active combat from timed scenery passes.

## Live validation — 25 October 2026

- Combined equipment and lamp run reached all 44 destinations across resumed sessions: 44 `done`, no `skip` records. Final log confirms stop 44/44 and automatic exit request.
- Initial gear pass logged all 34 equipment rows verified, with zero equip-verification failures. This verifies equipment assignment, not successful rendering of every action/effect.
- Gear completion now persists across restarts; the disposable save still resets each launch. Prior session logs are archived.
- User observed Celestial Emissary, Ebrietas, Blood-starved Beast, Darkbeast Paarl and other bosses alive. Father Gascoigne and Ludwig were not seen.
- User observed entrance cutscenes for Martyr Logarius, Mergo's Wet Nurse, Laurence and Orphan of Kos; inputs appeared to wait until they ended. The new build explicitly pauses gear/scenery clocks and neutralizes input while the game's cinematic flag is set, logging both transitions. This behavior has synthetic tests but awaits in-game validation.
- New build records a bounded, read-only enemy census at each scenery pass's start/end: entity IDs, handles and HP, with living/dead totals. Loaded presence is explicitly not a claim of visibility, boss identity, attack playback or shader coverage.
- New entities within 30m trigger 20 seconds of protected camera observation, once per entity ID per process. This allows natural AI activity but does not force aggression, boss activation, phases or specific attacks.
- Compilation telemetry records completed shader/pipeline compile scopes, active scopes and successful new graphics/compute pipelines. Counts are process-local and may include driver-cache hits; scope timings overlap. Stops wait for three seconds without observed compile activity, capped at 15 additional active seconds. Quiet does not imply complete rendering or queued GPU work has drained.
- Focused `--encounters` route uses two Upper Cathedral destinations and the Cainhurst rooftop, separate progress, and no equipment pass. Use local `Test-Encounters.cmd` for the next in-game validation instead of repeating all 44 lamps.
- Focused run completed 3/3; user reported Ebrietas remained inactive because the player stayed too far away. New build resolves entity 2420800 and requests placement 4m away on the player's side through the player's existing virtual warp method. It refuses placements over 100m or across more than 8m vertical difference. Upper Cathedral stops only; then a stationary 60s observation with two brief R1 attempts replaces retreating movement. Activation and destination safety still need in-game validation; no claim of a completed fight or phase coverage.
- User observed abnormal floor rendering in Rom's arena; cause not established.
- First Cainhurst rooftop attempt crashed with Vulkan device lost during submit, preceded by texture-memory pressure warnings. A later resumed run completed the destination; underlying crash cause remains unresolved.
- A `done` record means the timed routine finished, not that all shaders, boss phases, visible scenery or effects were exercised.

## Not started

- Refactor the upstream state machine onto `Backend`.
- Comprehensive item effects, enemy attack/death and boss-phase passes; launcher page.
- Dedicated Hunter tools, controlled cutscene, time-of-day and Chalice passes.

