# Boss-only route v2 audit

Travel endpoint audit only. Encounter activation/phase handling now uses
[v3's script audit](BOSS-SCRIPT-AUDIT.md). A boss-defeat travel point is not an
entrance trigger, and v2 timed attempts are not valid phase evidence.

25 October 2026. The earlier `--bosses` implementation mistakenly reused all 44 normal travel lamps plus two ordinary Dream returns. That implementation is superseded; its progress must never be reused for v2.

## Verified locally, without modifying game data

- Source: local CUSA03173 `dvdroot_ps4/param/gameparam/gameparam.parambnd.dcx`.
- SHA-256: `581E28302A231A10AD333806DFC90F41425DB4F9F146799DCA625F8D83C760C3`.
- Reader independently decompresses the DFLT DCX, reads its BND4 entry table and the 64-bit ReturnPointParam row table, then checks row IDs, map bytes and boss-defeat row labels.
- All 20 travel endpoints in `src/boss_destinations.json` passed this audit. Ordinary start/intermediate lamps do not pass the boss-defeat-label check. The row labels are internal development names, not necessarily the displayed English lamp names.
- The launcher repeats this read-only preflight before a boss-only run and refuses to launch on failure. No extracted game assets are bundled.
- Format references: [SoulsFormats BND4](https://github.com/soulsmods/SoulsFormats/blob/master/SoulsFormats/Formats/Binder/BND4/BND4.cs), [binder entry reader](https://github.com/soulsmods/SoulsFormats/blob/master/SoulsFormats/Formats/Binder/BinderFileHeader.cs), [PARAM reader](https://github.com/soulsmods/SoulsFormats/blob/master/SoulsFormats/Formats/PARAM/PARAM/PARAM.cs), [PARAM row reader](https://github.com/soulsmods/SoulsFormats/blob/master/SoulsFormats/Formats/PARAM/PARAM/Row.cs).

## Explicit destinations

| Intended encounter | Return point |
|---|---:|
| Cleric Beast | 2412952 |
| Father Gascoigne | 2412953 |
| Blood-starved Beast | 2302951 |
| Vicar Amelia | 2402951 |
| Witches | 2202951 |
| Darkbeast Paarl | 2302952 |
| Shadows | 2702951 |
| Rom | 3202952 |
| The One Reborn | 2802951 |
| Amygdala | 3302951 |
| Micolash | 2602952 |
| Mergo's Wet Nurse | 2602951 |
| Martyr Logarius | 2502952 |
| Celestial Emissary | 2422952 |
| Ebrietas | 2422951 |
| Ludwig | 3402952 |
| Laurence | 3402953 |
| Living Failures | 3502952 |
| Lady Maria | 3502951 |
| Orphan | 3602952 |

Gehrman and Moon Presence remain two explicit unavailable roster jobs, with no travel request. Local Dream rows establish ordinary return/death locations, not a verified arena-entry/ending-state workflow. Do not substitute ordinary Dream travel for either fight. Their reports must say unavailable, not covered.

## Regression checks and limits

- Native route must match the JSON manifest, in order and by expected entity. Exactly 22 roster jobs: 20 actual boss-room endpoints plus the two unavailable ending jobs.
- Each job restricts selection to its intended boss family and known actor variants. A nearby different boss may not substitute for the intended one.
- Missing/inactive/previously attempted/out-of-placement-bounds actors are reported and the route moves on after settling; no ordinary movement/sweep routine is run as a substitute.
- `bosses-v2-progress.txt` and its corresponding boss-attempt file isolate progress from v1. Old evidence is preserved, not destructively reset.
- Tests reject ordinary Central Yharnam and Dream substitutions, wrong maps, unsupported input, and unexplained unavailable jobs. The first real destination must be Cleric Beast's boss-defeat point, not the clinic or Central Yharnam lantern.
- This verifies destination identity, not all spawn conditions, arena collision, phase transitions or shader coverage. Witches/Shadows/shared-bar Living Failures/scripted Micolash remain observation-only. Missing actors/save-state handling and ending fights still require implementation.

Reproduce the local audit from the project root with the bundled Python interpreter:

```text
python tools/inspect_return_points.py <game>/dvdroot_ps4/param/gameparam/gameparam.parambnd.dcx --manifest src/boss_destinations.json
```
