# Development status

## Release candidate: v0.1.0-alpha.1

Experimental, not an all-shaders-complete release. Use [QUICKSTART](../QUICKSTART.md).
The default full workflow runs equipment/44 lanterns, then bosses without a manual
checkpoint; the test save is reset for each game process. First setup imports the
original Nexus ZIP and remembers installation/game folders. No author-specific
paths, Python installation, copied personal save or development runtime is required.
The save is not redistributed: its author explicitly prohibits reuploads.

### Latest actual-game evidence

- The independent-axis steering correction removed the Rom/Ebrietas circling.
  Both now confirmed their actual battle events after real approach/attacks.
- Gascoigne's normal entry confirmed his actual battle event.
- Cleric Beast's wait now starts **after** reaching the trigger, not while walking
  toward it. The follow-up full boss run confirmed activation and phase event 12414707.
- The new executable runs with the regular v1.9.4 installation's DLLs. Embedded
  launcher Python also successfully loaded the portable setup script.
- Full boss/phase validation is continuing. Older failure reports below are
  retained as history; do not infer every remaining phase has been fixed.

### Explicit gaps

Micolash is excluded by request. Gehrman/Moon Presence entry is not implemented.
Chalice bosses are out of scope. Group phases, exhaustive attacks, Hunter tools,
usable-item effects and time-of-day variants are not complete. The report records
attempts, observed events and failures separately. A quiet compile counter is not
proof that unseen content was compiled.

## Current: entry/wake controller v4 (25 October 2026)

The user's v3 run still failed Cleric Beast, Gascoigne, Rom and Ebrietas. Do not
treat v3's map-reference audit or regression passes as proof of a working tour.

- Added explicit first-entry versus repeat-fog state. Six authored fog Objects,
  their yaw/position, first-entry events and fog interaction events are checked
  against local MSB/EMEVD. A boss-defeat spawn behind a fog gets one fixed setup
  outside that door, then actual approach/interaction/walking through it. This
  isolated setup is not a return to boss-relative fight-time repositioning.
- Rom/Ebrietas approach into body contact, turn through real movement, check
  facing, request/release repeated R1 attacks and watch hit/battle events. The
  prior 3.5 m stand-off plus timed button pulse is removed. An input request alone
  is not a successful hit. Pre-existing hit flags are explicitly logged.
- Added position/entry-step/attack/activation diagnostics and a short four-boss
  `--entry-check` mode. It has no gear/health-stage/full-tour pass and prints each
  activation result. Local shortcut: `Check-Boss-Entrances.cmd`.
- v4 uses separate progress files, preserving all v3 evidence. Resetting the
  disposable save before every game process is unchanged.
- 36 Python tests and seven native regression programs pass. Native simulations
  include solid fog/repeat entry, boss-body collision, camera-follow rotation,
  wrong-facing attacks, no contact, missing events and bounded entry placement.
  These simulate mechanics; v4 is **not yet validated in the actual game**.

Do not ask the user to repeat the full boss batch to debug these four starts.
The short diagnostic is ready for a future test; no new testing was requested
from the user during this fix.

Other v3 issues retained from the log: Logarius fell during his later close pass;
Emissary's giant transition was not confirmed; Laurence's later target was outside
the local floor checks; Living Failures and Maria stalled on approach. BSB, Paarl
and Amygdala did produce their requested phase-event evidence. This is not
exhaustive attack or shader coverage and is not a fully working whole-game pass.

## Current: boss controller v3 (25 October 2026)

Supersedes v2's timed proximity-placement encounters. Full details and primary
script findings: [Boss script audit](BOSS-SCRIPT-AUDIT.md).

- Removed all boss-relative player-position warps and repeated gap corrections.
- Added authored entrance walking, actual battle-event gates, Maria chair
  interaction, closed-loop stick steering and bounded stall/fall handling.
- Added explicit redirected health owners and event-selected live forms for
  Gascoigne, Emissary, Ludwig and Orphan; fixed shared-owner/body identification
  for Wet Nurse, One Reborn and Living Failures.
- Known transitions wait for event evidence, not just an HP write or timer.
  Missing transitions end incomplete. AI-only phases remain unclassified;
  group encounters still have observation-only passes.
- Excludes Micolash without a warp. Ending entries remain unavailable.
- Isolates v3 progress, retains older logs/cache, and resets the disposable save
  before every game process. Failures are retained in the evidence report.
- Passed local MSB/EMEVD preflight for 19 included profiles, 35 Python tests and
  six native regression programs. In-game v3 validation is still pending.

Historical v3 handoff: run the existing `Run-Boss-Batch.cmd` for combined validation; the game
continues across encounters and exits afterward. Report entrance failures,
falls and missing transformations together, not as individual checkpoints.

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
- Follow-up user test confirmed attacks hit Ebrietas and woke her after correcting the facing yaw. The retained log recorded +7 successful new pipelines and +13 completed compile scopes at that stop; these numbers are not an exhaustive coverage claim.
- User reported Logarius stayed distant and used ranged attacks. New build adds entity-2500800 proximity placement to 3m at the rooftop stop, plus at most three checks at 20s intervals to close gaps over 10m only when vertical separation is at most 1.5m. This is awaiting in-game validation. No HP/phase forcing is implemented.

## Earlier batched encounter build (historical; superseded below)

- User requested fewer checkpoints and ranged attacks before close combat. Logarius now has five stages: ranged 14m/20s, close 3m/25s, 60%-HP ranged 14m/20s, 60%-HP close 3m/25s, 30%-HP close 3m/25s. Ebrietas: wake 4m/15s, ranged 12m/20s, close 4m/25s, 60%-HP ranged 12m/20s, 30%-HP close 4m/25s.
- Health requests are enabled only by the focused encounter launcher, target the selected live boss's damage module, only reduce HP, and clamp above zero. Starting HP is the baseline, not a separately verified maximum. These are requests for lower-health behaviour, NOT verified phase transitions or damage-event simulation.
- Each stage logs placement outcome, HP readback, new animation IDs relative to its starting ring, new pipelines and compile events. Animation sampling is every 0.5s and can miss short events; IDs include non-attack animations. Natural HP/AI event paths may be required for some transitions.
- Camera stays steady during most observation with a short sweep at stage end. Up to two distance corrections per stage; airborne or cross-floor corrections are refused. No pathfinding, collision/floor raycast, guaranteed camera targeting or full boss-phase recognition is implemented.
- `encounters-report.json` is generated after each child process exits and includes stage evidence and failures across that log's retries. Normal save isolation, fresh test save per launch, cutscene clock suspension, progress resume and automatic quit remain.
- Static/native tests cover timeline boundaries, full-health ranged-before-close order, HP clamps, writes disabled outside the phase-enabled test, stale entity rejection, facing conventions and reporting. The combined staged batch needs in-game validation; it is not a claim that every encounter or shader is covered.
- User observed abnormal floor rendering in Rom's arena; cause not established.
- First Cainhurst rooftop attempt crashed with Vulkan device lost during submit, preceded by texture-memory pressure warnings. A later resumed run completed the destination; underlying crash cause remains unresolved.
- A `done` record means the timed routine finished, not that all shaders, boss phases, visible scenery or effects were exercised.

## Not started

- Refactor the upstream state machine onto `Backend`.
- Comprehensive item effects, enemy attack/death and boss-phase passes; launcher page.
- Dedicated Hunter tools, controlled cutscene and time-of-day passes. Chalices are out of scope.

## Current full non-Chalice roster batch

- **Routing v2 supersedes the original full-lamp routing mistake.** `Run-Boss-Batch.cmd` / `--bosses` now uses 20 locally audited boss-defeat destinations and two explicit unavailable ending jobs. It never visits ordinary lanterns or substitutes ordinary Dream travel. See `BOSS-ROUTE-VALIDATION.md` and `src/boss_destinations.json`.
- Every stop binds an expected boss entity family; another nearby boss cannot substitute. Missing targets do not run a normal movement/sweep fallback. Separate `bosses-v2` route and boss-attempt progress prevent resuming into the old 46-stop sequence. No gear repeat.
- Catalog includes all 22 names and known entity variants. At each stop the nearest unattempted living catalog actor within 100m and 8m vertical separation is selected. This is an availability-driven attempt, not guaranteed boss spawning; absent or disabled bosses are reported, never silently certified.
- Profiles derive below-threshold sample HP from the boss reference: 5 percentage points below each documented threshold. Instance maximum HP is read at damage-module +0xfc (pinned party_balance.cpp:16); invalid values reject the run. HP is only reduced and never set to zero.
- Every applicable phase has ranged and close windows. Transition holds are 15s with no attacks, camera sweep or gap corrections. Logarius uses 100% ranged/close, 65% transition, then ranged/close; no invented third phase. Ebrietas adds activation, then 45% transition and both phase-2 distance windows.
- Witches, Shadows, Micolash and Living Failures receive group/scripted observation-only profiles: no unsafe use of one actor's HP as a shared/group progression substitute. Their complete encounter scripts, group transitions and death/revival states remain unimplemented.
- Native and Python tests cover all 22 catalog entries, no Chalice family/route, exact boss-only destinations and expected entities, unavailable ending entries, sourced samples, maximum-vs-starting HP, no repeated attempts, transition silence, stale target protection, positive placement/facing and roster reporting. Local game-data preflight independently checks all 20 warp endpoints.
- Reports distinguish not-attempted, previous-session attempt, incomplete, stages-attempted and scripted-observation-only. They do not certify actual named phases or exhaustive shader coverage. New full-roster behaviour awaits one combined user run; no claim that all 22 bosses are functional.

