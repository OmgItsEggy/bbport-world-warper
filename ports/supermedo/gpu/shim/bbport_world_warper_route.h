// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace BbWorldWarperRoute {
struct Stop { std::uint32_t lamp, map; const char* name; };
inline constexpr Stop stops[] = {
    {2412950, 0x18010000, "Central Yharnam"},
    {2402950, 0x18000000, "Cathedral Ward"},
    {2302950, 0x17000000, "Old Yharnam"},
};
inline constexpr unsigned count = sizeof(stops) / sizeof(stops[0]);
constexpr Stop Lamp(unsigned a, unsigned n, const char* name) {
    return {a * 10000u + 2950u + n, ((a / 10) << 24) | ((a % 10) << 16), name};
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
}
