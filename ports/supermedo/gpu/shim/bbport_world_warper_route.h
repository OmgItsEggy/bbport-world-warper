// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <array>
namespace BbWorldWarperRoute {
struct Stop { std::uint32_t lamp, map; const char* name; std::uint32_t boss=0; };
inline constexpr Stop stops[] = {
    {2412950, 0x18010000, "Central Yharnam"},
    {2402950, 0x18000000, "Cathedral Ward"},
    {2302950, 0x17000000, "Old Yharnam"},
};
inline constexpr unsigned count = sizeof(stops) / sizeof(stops[0]);
// Fast activation-only regression, no equipment rotation or health-stage tour.
inline constexpr Stop entry_stops[] = {
    {2412952,0x18010000,"Cleric Beast entrance",2410800},
    {2412953,0x18010000,"Gascoigne entrance",2410810},
    {3202952,0x20000000,"Rom wake-up",3200800},
    {2422951,0x18020000,"Ebrietas wake-up",2420800},
};
// Focused validation: two Upper Cathedral boss destinations plus Cainhurst rooftop.
inline constexpr Stop encounter_stops[] = {
    {2422952,0x18020000,"Upper Cathedral Ward",2420810},
    {2422951,0x18020000,"Upper Cathedral Ward",2420800},
    {2502952,0x19000000,"Cainhurst rooftop",2500800},
};
constexpr Stop Lamp(unsigned a, unsigned n, const char* name, unsigned boss=0) {
    return {a * 10000u + 2950u + n, ((a / 10) << 24) | ((a % 10) << 16), name,boss};
}
inline constexpr Stop all_stops[] = {
    Lamp(241,0,"Central Yharnam"), Lamp(241,1,"Central Yharnam"), Lamp(241,2,"Central Yharnam"), Lamp(241,3,"Central Yharnam"),
    Lamp(240,0,"Cathedral Ward"), Lamp(240,1,"Cathedral Ward"),
    Lamp(242,0,"Upper Cathedral Ward"), Lamp(242,2,"Upper Cathedral Ward"), Lamp(242,1,"Upper Cathedral Ward"),
    Lamp(230,0,"Old Yharnam"), Lamp(230,1,"Old Yharnam"), Lamp(230,2,"Old Yharnam"),
    Lamp(220,0,"Hemwick"), Lamp(220,1,"Hemwick"), Lamp(270,0,"Forbidden Woods"), Lamp(270,1,"Forbidden Woods"),
    Lamp(320,0,"Byrgenwerth / Lecture Building"), Lamp(320,2,"Byrgenwerth / Lecture Building"),
    Lamp(280,0,"Yahar'gul"), Lamp(280,3,"Yahar'gul"), Lamp(280,1,"Yahar'gul"), Lamp(280,2,"Yahar'gul"),
    Lamp(250,0,"Cainhurst"), Lamp(250,2,"Cainhurst"), Lamp(250,1,"Cainhurst"), Lamp(211,0,"Abandoned Workshop"),
    Lamp(320,1,"Lecture Building"), Lamp(320,3,"Lecture Building"), Lamp(330,0,"Nightmare Frontier"), Lamp(330,1,"Nightmare Frontier"),
    Lamp(260,0,"Nightmare of Mensis"), Lamp(260,3,"Nightmare of Mensis"), Lamp(260,2,"Nightmare of Mensis"), Lamp(260,1,"Nightmare of Mensis"),
    Lamp(340,0,"Hunter's Nightmare"), Lamp(340,1,"Hunter's Nightmare"), Lamp(340,2,"Hunter's Nightmare"), Lamp(340,3,"Hunter's Nightmare"),
    Lamp(350,0,"Research Hall"), Lamp(350,1,"Research Hall"), Lamp(350,2,"Research Hall"),
    Lamp(360,0,"Fishing Hamlet"), Lamp(360,1,"Fishing Hamlet"), Lamp(360,2,"Fishing Hamlet"),
};
inline constexpr unsigned all_count = sizeof(all_stops) / sizeof(all_stops[0]);
// Explicit boss destinations audited against local 1.09 ReturnPointParam names.
// Keep synchronized with src/boss_destinations.json; tests reject normal lamps.
inline constexpr std::array<Stop,22> boss_stops{{
    Lamp(241,2,"Cleric Beast",2410800),
    Lamp(241,3,"Father Gascoigne",2410810),
    Lamp(230,1,"Blood-starved Beast",2300800),
    Lamp(240,1,"Vicar Amelia",2400800),
    Lamp(220,1,"Witches of Hemwick",2200800),
    Lamp(230,2,"Darkbeast Paarl",2300810),
    Lamp(270,1,"Shadows of Yharnam",2700800),
    Lamp(320,2,"Rom",3200800),
    Lamp(280,1,"The One Reborn",2800800),
    Lamp(330,1,"Amygdala",3300800),
    Lamp(260,2,"Micolash",2600850),
    Lamp(260,1,"Mergo's Wet Nurse",2600800),
    Lamp(250,2,"Martyr Logarius",2500800),
    Lamp(242,2,"Celestial Emissary",2420810),
    Lamp(242,1,"Ebrietas",2420800),
    {0,0x15000000,"Gehrman",2100800}, // unavailable ending entry; never warp to ordinary Dream
    {0,0x15000000,"MoonPresence",2100810},
    Lamp(340,2,"Ludwig",3400800),
    Lamp(340,3,"Laurence",3400850),
    Lamp(350,2,"Living Failures",3500850),
    Lamp(350,1,"Lady Maria",3500800),
    Lamp(360,2,"Orphan of Kos",3600800),
}};
}
