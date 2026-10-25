# Bloodborne boss phases and warm-up coverage reference

Researched: 25 October 2026. Target game: Bloodborne 1.09, including The Old Hunters.

This is the encounter implementation reference, not a claim about what the current build covers. Read the relevant row before adding or changing a boss profile. Do not use one generic 60%/30% sequence for every boss.

Scope: all 17 main-game bosses and all five DLC bosses. Chalice bosses/dungeons and unused/cut content are explicitly out of scope at the user's request.

## Interpretation and evidence

- Percentages mean **remaining maximum HP**, not damage dealt. Community guide thresholds are approximate unless independently checked in game scripts. They are research inputs, not confirmed executable constants.
- **Documented** means the linked source describes the trigger/state. **Threshold unresolved** means the behaviour is described but a reliable numeric trigger was not established in this research. It does not mean “no phase.”
- **No separate phase established** means the reviewed source does not establish an HP-phase progression. Do not manufacture one from a generic low-health sample.
- Community wikis disagree about some thresholds and about whether an expanded moveset counts as a distinct phase. Keep the behaviour and trigger more important than the phase number. Below, the detailed Bloodborne Wiki attack-pattern sections are the default, with conflicts noted.
- The engine may require encounter activation, AI/event progression, a damage event, a transition animation, a new actor, or group state. Changing an HP field alone does not prove any of those happened.
- Source descriptions and coverage targets are paraphrased. Distances, dwell times and proposed sample HP values are our engineering choices, not source-prescribed values.

## Main game — 17 bosses

| Boss | Documented phases / triggers | Rendering and encounter coverage to exercise | Source / caveat |
|---|---|---|---|
| Cleric Beast | Two described stages; second around 70% HP, announced by a roar. Broken limbs/head can be repaired separately from total HP. | Opening, normal/longer combos, leap, grab, red repair effect, injured body parts. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/cleric-beast.html). Do not confuse limb repair with HP healing. |
| Father Gascoigne | One-handed axe/gun; extended axe can begin after about 20% HP loss (~80% remaining), but spacing affects when he transforms. Beast form around 30% remaining. | Both axe forms, firearm, transformation and beast attacks; music-box reaction is separate optional coverage. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/father-gascoigne.html). [Wikidot](https://bloodborne.wikidot.com/father-gascoigne) instead describes axe extension after about one-third damage: numeric timing needs script confirmation. |
| Blood-starved Beast | Three described stages: poison-bearing melee below ~70%; proximity poison/Poison Blast at ~30%. | Normal melee, poison melee, grab, transition roar, poison cloud/blast and persistent aura. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/blood-starved-beast.html). Wait for the poison transition instead of interrupting it. |
| Vicar Amelia | More aggressive/healing stage around 50%. Limb damage is another state axis. | Opening transformation, melee, shockwave, healing effect, injured limbs. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/vicar-amelia.html). Do not suppress healing while collecting its effects. |
| Witch of Hemwick | Group/visibility progression, not one HP bar. Second witch joins when the first drops below ~50%; one witch going down starts the dangerous remaining-witch/revival state. Guide describes revival after ~50 seconds at ~20% HP. Mad Ones depend on Insight. | Both witches, proximity visibility, teleports, Mad One summons, binding spell, grab, revival. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/hemwick-witch.html). Zero Insight omits Mad Ones. Requires a bespoke group/death/revival pass, not our never-kill single-boss routine. |
| Darkbeast Paarl | Second described stage at ~66%, with stronger electrical aura. Breaking limbs can remove electricity, which he can restore. | Charged and discharged body, recharge, lightning projectiles, discharge AoE, limb-break effects. | [Detailed guide](https://www.bloodborne-wiki.com/2015/02/darkbeast-paarl.html). Continuous stagger can prevent the desired effects from playing. |
| Shadows of Yharnam | Group phase 2 when one falls below ~30% OR all three reach ~50%; phase 3 described when all are below ~25% or only one remains with a full bar. | Three distinct fighters, fireballs/flaming blade, snake-body attacks, giant snake summons. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/shadow-of-yharnam.html). Must track the whole group; do not assume three independent identical profiles. |
| Rom | Three described stages: first teleport/spell stage below ~75%; another teleport/spider wave around 50%, adding close-range thrashing. | Initial spiders, both teleports, additional waves, falling/rising arcane attacks, spherical blast, thrash. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/rom-vacuous-spider.html). Reacquire after teleports. Investigate this port's reported arena-floor anomaly separately. |
| The One Reborn | No separate numbered HP-phase progression established from the reviewed guide; limb/collapse and supporting enemies matter. | Bell-ringing women and their support, boss projectiles, ground hazards, collapse/recovery and close-range attacks. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/the-one-reborn.html). No invented universal third phase. |
| Amygdala | Second stage around 70%, adding energy-enhanced attacks; third below ~30%, tearing off arms as weapons. | Lasers, impact explosions, acid, arm-removal transition, arm-club attacks and leaps. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/amygdala.html). Allow arm removal to complete. |
| Micolash | Two combat/chase sections. At ~50% he relocates; final room adds A Call Beyond. | Puppets, Augur, chase/teleport events, final-room gates and A Call Beyond. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/micolash-host-of-nightmare.html), [GameSpot](https://www.gamespot.com/articles/bloodborne-boss-guide-how-to-beat-micolash-host-of/1100-6426376/). HP editing alone cannot replace room/chase progression. |
| Mergo's Wet Nurse | Guide places second-stage eligibility below ~70%; the important visible change occurs during the enhanced Nightmare Veil/clone sequence. | Entrance cinematic, base melee, teleport, darkness/veil, clone and clone attacks. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/mergos-wet-nurse.html). Merely crossing the threshold does not prove the darkness/clone technique was used. |
| Martyr Logarius | Two main stages; transition around 70%, with a charged buff/AoE before more aggressive sword/scythe behaviour. | Full-health ranged spells first; transition charge and buff uninterrupted; later melee, aerial movement and planted-sword projectile cloud. | [Detailed challenge guide](https://www.bloodborne-wiki.com/p/challenge-runs.html), [Fandom](https://bloodborne.fandom.com/wiki/Martyr_Logarius). No third phase established here. Backstabbing the charge suppresses buff coverage. |
| Celestial Emissary | Three described states: small disguised boss; giant below ~60%; tendrils/projectiles below ~30%. | Small boss plus adds, growth transition, giant melee, tendril growth and ranged beams. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/celestial-emissary.html). Do not target an arbitrary small add or interrupt growth. |
| Ebrietas | Must be attacked to start the initial encounter. Two phases; below ~50%, Cosmic Radiation aura and expanded attacks. | Activation, base melee/charge/blood spit, aura transition, Star Shower and flying/two-stage charge. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/ebrietas-daughter-of-cosmos.html), [Fandom](https://bloodborne.fandom.com/wiki/Ebrietas%2C_Daughter_of_the_Cosmos). A 60%-HP window is still above her documented phase-2 threshold. No separate 30%-HP third phase established. |
| Gehrman | Three described stages: scythe; sword/gun below ~75%; quickening/powered moves around 50%. | Both weapon forms, firearm, transformation, aura/quickening, large charged blast and airborne wind attack. | [Detailed guide](https://www.bloodborne-wiki.com/2015/04/gehrman-first-hunter.html). Requires an appropriate ending/encounter state. |
| Moon Presence | Guide describes expanded second-stage attacks at ~70%. Ending prerequisites are separate. | Opening, normal melee, regeneration visual, HP-to-one flash, healing-blocking blood effects. | [Detailed guide](https://www.bloodborne-wiki.com/2015/03/moon-presence.html). Its HP-to-one player attack must not be mislabelled as failure of death protection. |

## The Old Hunters — all five bosses

| Boss | Documented phases / triggers | Coverage targets | Source / caveat |
|---|---|---|---|
| Ludwig | Two main forms; Holy Blade transition/cinematic around 50%. Additional first-form attacks unlock below ~80%. | Beast form, later beast charge/drop/geyser attacks, transition cinematic, sword form, moonlight waves and charged AoE. | [Detailed guide](https://www.bloodborne-wiki.com/2015/10/ludwig.html), [Prima](https://primagames.com/news/bloodborne-old-hunters-guide-and-walkthrough-defeat-ludwig-accursed-find-whirligig-saw-and-beast-cutter). User reported him absent: resolve encounter state before treating a room visit as coverage. |
| Laurence | Three behavioural stages when counting the expanded upright moveset: below ~80%, then crawling/leg-loss at ~40%. Some guides call only the upright/crawling distinction two phases. Limb breaks also change follow-ups. | Opening, upright flame attacks/explosions, expanded combos, limb effects, leg-loss transition, crawling and persistent lava. | [Detailed guide](https://www.bloodborne-wiki.com/2015/11/laurence-first-vicar.html). Track behaviours, not phase-label disagreements. |
| Living Failures | Shared group HP; Meteor Swarm becomes available below ~60%. Individuals respawn. | Several bodies, melee and arcane attacks, simultaneous meteor wind-up, changed sky/meteor barrage and respawn. | [Detailed guide](https://www.bloodborne-wiki.com/2015/11/living-failures.html). Individual actor HP is not the shared boss bar; do not interrupt the meteor wind-up. |
| Lady Maria | Three stages: ordinary weapons; blood-extended weapons below ~60%; fire-enhanced blood attacks below ~30%. | Opening interaction, normal moveset, blood transformation/explosion and trails, final aerial burst/fire trails. | [Detailed guide](https://www.bloodborne-wiki.com/2015/11/lady-maria-of-astral-clocktower.html). Do not guess 70% from memory; reference uses 60%. |
| Orphan of Kos | Two main phases; transformation below ~50%. | Opening cinematic, first-form attacks/projectiles, winged transformation, aerial/projectile attacks and lightning waves. | [Detailed challenge guide](https://www.bloodborne-wiki.com/p/challenge-runs.html). Exact event/animation verification is still needed in the port; no third phase established. |

## How this changes our implementation

1. Replace generic “lower-health” assumptions with boss-specific authored states and source links. **Do not call 60% Ebrietas phase 2**; her documented boundary is ~50%. A proposed sample at 45% would cross it, but that sample is our choice, not the boundary.
2. Logarius needs phase-1 ranged observation before crossing ~70%, an uninterrupted buff/transition window, then phase-2 ranged-effect and close-combat windows. A sample around 65% is an engineering choice to cross the documented threshold. Do not manufacture a third phase at 30%.
3. Separate approach/activation, transition completion, close/mid/ranged observation, body/limb state, and actual visible effects. A phase number is not enough.
4. Do not attack during a transition that we want to render. Do not advance just because a timer or HP readback succeeded. Prefer supported event/state/animation identifiers; otherwise report “requested, unconfirmed,” with a bounded timeout.
5. Resolve missing actors and saved encounter flags first. A completed lamp warp cannot recreate a defeated or disabled boss by itself.
6. Group bosses, shared bars, deaths/revivals, replacements and scripted chases need bespoke controllers. The current single-target never-kill routine does not cover these mechanics.
7. Use the true instance maximum HP once the correct field/parameter is verified. The revised build reads maximum HP at damage-module +0xfc and refuses invalid values rather than treating starting HP as maximum.
8. Keep the disposable save reset and online disabled. Do not lower HP in the user's real playthrough. Do not claim all shader compilation is complete from a few pipeline counters.

## Project evidence versus research

| Item | What is actually known in our project |
|---|---|
| 44 lamps | Timed routines completed across resumed sessions; not 44 verified encounters. |
| Equipment | All 34 rotation rows passed equip assignment verification; not proof of every action/effect. |
| Ebrietas | User confirmed corrected facing, attacks landing and waking her. A retained run logged +7 successful new pipelines and +13 compile scopes. Second-phase completion has not been verified. |
| Logarius | User saw ranged attacks while the character was too far away. Subsequent single-distance and generic health-stage builds were implementation attempts, not phase research. |
| Celestial Emissary | Loaded/alive observed; staged growth/tendril coverage not implemented/verified. |
| Missing bosses | User did not see Gascoigne or Ludwig; phase knowledge does not resolve absent-actor/save-state issues. |
| Cinematics | Entrance cinematics seen for Logarius, Wet Nurse, Laurence and Orphan. Explicit cinematic-clock suspension was added; this does not verify later boss phases. |
| Revised full-roster batch | Uses sourced per-boss thresholds with samples 5 percentage points below each boundary, instance maximum HP, ranged/close stages and quiet transition holds. The 22-boss roster is tracked; missing actors and scripted/group observations are not certified as covered. |

## Unresolved research tasks owned by development, not the tester

- Verify numeric triggers and phase/transition identifiers against 1.09 event/AI data, especially Gascoigne's weapon switch.
- Maximum HP is now read from damage-module +0xfc, documented in the pinned reference party_balance.cpp:16. Shared-boss-bar handling remains unresolved.
- Map attack/transition animation IDs to named actions; raw new IDs include idle, locomotion, hit reactions and non-attack transitions.
- Determine which transitions require actual damage events rather than raw HP assignment.
- Validate actor replacement, arena floor/collision checks and camera visibility before generalising proximity placement to more bosses.

Testing should answer “did our researched sequence execute correctly in this port?” It should not require the user to identify the game's phases for us.
