# Boss controller v3 — 25 October 2026

## v4 correction after the user's failed entry test

The v3 implementation did not prove these entrances worked. Cleric Beast,
Gascoigne and Amelia timed out/stalled; Rom and Ebrietas never activated.
The audit below establishes map/script facts, not successful runtime traversal.

v4 reads the saved first-entry flag, approaches the **outside** of the actual fog
Object and uses the normal Cross interaction for repeat entries. When spawned
behind the one-way fog, there is exactly one allowlisted setup 3 m outside the
authored door. The settled position is checked before walking; no random or
boss-relative candidate positions are allowed. Six gate Object transforms and
their first-entry/fog-event references now have independent local preflight.

For Rom and initial Ebrietas entry, v4 walks into contact, turns through input,
checks facing, retries and releases R1, then watches the actual attacked/battle
events. It logs pre-existing flags rather than treating them as a new hit. Repeat
Ebrietas entry uses her normal fog interaction. No HP edits manufacture a hit.

`--entry-check` visits only Cleric Beast, Gascoigne, Rom and Ebrietas and ends each
stop after actual battle activation. It prints automatic pass/fail evidence and
does not run equipment or boss-health phases. The v4 build is regression-tested
but has not yet been validated in-game.

The v3 runtime rules below about **no placement at all** are superseded by the
single authored outside-gate setup. The prohibition on fight-time boss-relative
position teleports remains.

Supersedes v2's proximity-placement controller. A boss-defeat lantern is only a
travel endpoint, not an encounter trigger. A lower HP value is only a request,
not evidence that a phase occurred or that its shaders were drawn.

## Evidence

Primary reference: [vanilla Bloodborne event scripts](https://github.com/Grimrukh/soulstruct-vanilla/tree/e6de1b79370755152f4f89c4a106a2d67c9ae674/bloodborne/events),
inspected as text, not executed. No game script assets are redistributed.

Entrance coordinates were independently decoded from the user's installed MSB
files. Format references: [Soulstruct region layout](https://github.com/Grimrukh/soulstruct/blob/main/src/soulstruct/bloodborne/maps/regions.py)
and [SoulsFormats MSBB](https://github.com/soulsmods/SoulsFormats/tree/master/SoulsFormats/Formats/MSB/MSBB).

`tools/audit_boss_scripts.py` independently decodes local EMEVD tables and checks
battle event IDs, referenced actors/phase IDs, PLAYER region conditions and
compiled coordinates (within 1 mm of local MSB values). All 19 included profiles
passed. The launcher repeats this read-only preflight before a boss batch.

This proves references/coordinates exist. It does **not** prove an unobstructed
walking route, a particular AI attack, or exhaustive shader coverage.

## Mechanics used by v3

Region entries below are WALK targets, never player-position writes. Battle
events are completion flags for the event that enables AI/health bars. Flags
are read-only: the controller cannot force entrance/progression/phase flags.

| Encounter | Initial activation | Battle event | Health ownership / phase evidence |
|---|---|---|---|
| Cleric Beast | PLAYER enters 2412805 | 12414702 | 2410800; below 70%, event 12414707 waits for TAE 100 |
| Gascoigne | Region 2412815; cinematic | 12414802 | 2410810 damage refers to 2410811; beast below 34%, replacement event 12414807 |
| BSB | Region 2302805 | 12304802 | 2300800; below 67%/33%, events 12304807/12304808 wait for TAE 10/20 |
| Amelia | Region 2402805; cinematic | 12404802 | 2400800; below 50%, event 12404807 waits for TAE 100 |
| Witches | Region 2202805 | 12204802 | Two independent bars/actors; group observation only in v3 |
| Paarl | PLAYER within 16 m; wake animation | 12304702 | 2300810; below 67% plus effect 5402, event 12304707 waits for TAE 20 |
| Shadows | Region 2702805 | 12704802 | Three actors; group observation only in v3 |
| Rom | Actual attack against 3200800 | 13204802 | Teleport events set 13204811/13204812 at 75%/50%; do not walk toward the shore region on another floor |
| One Reborn | Region 2802805; cinematic | 12804802 | Visible 2800800 is not the shared bar owner 2800803 |
| Amygdala | Region 3302805 | 13304802 | Below 70%/30%, events 13304807/13304808 wait for TAE 10 |
| Wet Nurse | Region 2602805; cinematic | 12604802 | Visible 2600800/2600801 damage refers to hidden owner 2600802; lower-health window remains unclassified |
| Logarius | Region 2502805; cinematic | 12504802 | 2500800; ranged before close observation; lower-health window remains unclassified |
| Emissary | Region 2422815; group wake | 12424702 | 2420810 damage refers to 2420811; giant swap 12424790 below 60%; 30% window not independently certified |
| Ebrietas | Actual attack against 2420800 | 12424802 | Below 50%, event 12424980 waits for TAE 500 |
| Ludwig | Region 3402805; cinematic | 13404802 | 3400800 damage refers to 3400801; expanded beast event 13404822; Holy Blade cinematic/swap 13404825 |
| Laurence | Region 3402855; cinematic | 13404852 | Same actor 3400850; crawl evidence 13404875 waits for TAE 400, then changes collision mask |
| Living Failures | Region 3502815 | 13504852 | 3500851–3500854 refer damage to shared 3500850; observe a real body, not the dummy; group observation only |
| Maria | Approach chair 3500905, face it, pulse Cross (13500948 → 13500947) | 13504802 | Region 3502808 is a post-cinematic player placement, not her chair; lower-health windows remain unclassified |
| Orphan | Region 3602805; cinematic; then proximity/attack | 13604802 | 3600800 damage refers to 3600801; event 13604820 waits for TAE 100, swaps actor, then waits for TAE 50 |

Micolash is excluded at the user's request without travelling there. Gehrman and
Moon Presence remain unavailable: ending entry is not implemented. Chalice
encounters remain out of scope.

## Runtime rules

- No `Chr::Warp`, no boss-relative Y placement and no randomized gap corrections.
  Only established lamp/map travel remains a teleport.
- Grounded walking calibrates the camera-relative stick basis from actual world
  movement, steers using position feedback, and stops after 5 s without progress.
  It does not guess an obstacle bypass. Entrance budget: 90 active seconds.
- Battle activation must be observed before observation or health stages start.
  Cinematics pause the coverage clock. Maria targets her actual chair actor, not
  the unrelated player placement point 22 m away.
- Position/animation targets follow event-selected forms. HP edits validate the
  explicit redirected owner plus the live form's ratio, each against its own max
  HP. Requests never heal or reduce HP to zero.
- Ranged observation precedes close observation. Approaches have an 18 s budget;
  the observation window starts afterward. Unreached ranges are logged.
- Known transitions wait up to 45 active seconds for their actual event flag.
  No attacks/walking during that wait. A missing event ends the encounter as
  incomplete; a pre-existing flag is not new transition evidence.
- Floor-envelope violations, rapid falls, unexpected loads, lost actors, bad
  health ownership or unavailable flag reads are failures, not coverage. A fall
  stops inputs; there is no rescue teleport. Unexpected loading restarts with
  a pristine disposable save.
- New `bosses-v3-progress.txt` / `encounters-v3-progress.txt` files prevent reuse
  of timed v2 attempts. Real save, pristine seed and GPU cache are preserved.

## User's v2 validation report

- Cleric Beast, Gascoigne, Amelia, Wet Nurse and Living Failures did not cross
  entrance triggers. One Reborn and Ludwig activated only after incidental moves.
- BSB and Paarl spawned; Paarl's repositioning later caused an out-of-map death.
- Emissary and Ebrietas fell through the floor.
- Ludwig played entry/later cinematics without a confirmed phase pass.
- Laurence fell outside the room before crawling behaviour was observed.
- Orphan spawned, fell under the map, and showed no confirmed phase change.

v3 is locally audited and regression-tested, not yet validated in-game. Remaining
gaps include obstacle-specific routes, group phase scheduling, AI-only phase
identification and exhaustive attack/effect coverage. Do not label this “all
shaders compiled.”
