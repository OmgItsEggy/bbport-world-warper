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
- Added a responsive GitHub Pages project site under `docs/`.

## In progress

- Connect image binding to Supermedo's loaded-image registration path.
- Recover ReturnPointParam lookup and loaded-enemy enumeration without Visy's party layer.
- Connect Supermedo's pipeline counters to `CompileCounters`.

## Not started

- Refactor the upstream state machine onto `Backend`.
- One-location smoke build and live test.
- Full-area route, gear/effects/enemy passes and launcher page.
- Optional bosses, Hunter tools, cutscenes, time-of-day and Chalice passes.

