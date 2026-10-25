#pragma once
#include "bbport_world_warper_walk.h"
#include <cstdint>
namespace BbWorldWarperEntry {
using BbWorldWarperEncounter::Point;
// Actual Object (MSB subtype 1), not the similarly numbered dummy fog model.
// The yaw points into the arena. Audit object/first-entry/fog-event references
// against local MSB/EMEVD before launch. These are NOT boss-relative positions.
struct Gate { unsigned boss,first_event,object,fog_event; Point door; float yaw_degrees; };
inline constexpr Gate gates[]={
    {2410800,12411702,2411800,12414730,{-154.44f,-27.02f,35.07f},-133.32f},
    {2410810,12411802,2411810,12414810,{-50.19f,-40.82f,70.8f},-134.46f},
    {2400800,12401802,2401800,12404840,{51.51f,35.71f,311.57f},-150.f},
    {3400800,13401801,3401800,13404800,{-407.93f,1503.8f,-716.45f},0.f},
    {3500850,13501851,3501810,13504850,{-378.54f,1592.9f,-824.55f},90.f},
    // On repeat visits Ebrietas uses fog entry; initial visits require a hit.
    {2420800,12421802,2421800,12424810,{94.f,4.2f,387.f},-150.740845f},
};
inline const Gate* GateFor(unsigned boss) {
    for(const auto& g:gates) if(g.boss==boss) return &g;
    return nullptr;
}
inline Point GatePoint(const Gate& g,float metres) {
    const float yaw=g.yaw_degrees*0.01745329251994329577f;
    return {g.door.x-std::sin(yaw)*metres,g.door.y,g.door.z-std::cos(yaw)*metres};
}
inline float GateSide(const Gate& g,Point p) {
    const auto inside=GatePoint(g,1);
    return (p.x-g.door.x)*(inside.x-g.door.x)+(p.z-g.door.z)*(inside.z-g.door.z);
}
// Only this allowlisted, one-off entry setup can call the engine placement
// function. It takes a boss ID, NOT caller-supplied coordinates or an NPC ptr.
bool SetupEntryStart(std::uint64_t player,unsigned boss);
inline unsigned HitEvent(unsigned boss) {
    return boss==3200800?13201802:boss==2420800?12421802:0;
}
}
