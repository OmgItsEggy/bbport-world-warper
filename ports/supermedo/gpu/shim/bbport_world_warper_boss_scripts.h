#pragma once
#include "bbport_world_warper_boss_catalog.h"
namespace BbWorldWarperEncounter {
// Numeric facts transcribed from vanilla EMEVD and independently read local MSB
// regions. Provenance/limitations: docs/BOSS-SCRIPT-AUDIT.md. Coordinates are
// WALK targets, never teleport coordinates. Flags are read-only gates.
struct Point { float x,y,z; };
struct Script {
    unsigned boss,region,battle,dead,health_owner,swap_flag,second_actor;
    Point entry;
    unsigned phase2=0,phase3=0;
    unsigned interaction=0;
};
inline constexpr Script scripts[]={
    {2410800,2412805,12414702,12411700,2410800,0,0,{-136.52f,-27.42f,52.77f},12414707},
    {2410810,2412815,12414802,12411800,2410811,12414807,2410811,{-48.76f,-40.92f,72.44f},0,12414807},
    {2300800,2302805,12304802,12301800,2300800,0,0,{-136.3349f,-126.8293f,53.8879f},12304807,12304808},
    {2400800,2402805,12404802,12401800,2400800,0,0,{57.7f,34.71f,322.6f},12404807},
    {2200800,2202805,12204802,12201800,2200800,0,0,{-336.14f,1.6f,726.6431f}},
    // Paarl/Rom/Ebrietas activate by proximity/attack, not a first-entry region.
    {2300810,0,12304702,12301700,2300810,0,0,{},12304707},
    {2700800,2702805,12704802,12701800,2700800,0,0,{-335.603f,-187.983f,485.437f}},
    {3200800,0,13204802,13201800,3200800,0,0,{},13204811,13204812},
    {2800800,2802805,12804802,12801800,2800803,0,0,{402.9f,-125.5f,-237.5f}},
    {3300800,3302805,13304802,13301800,3300800,0,0,{-98.8488f,1461.f,-46.3337f},13304807,13304808},
    {2600800,2602805,12604802,12601800,2600802,0,0,{146.46f,1124.25f,-37.9f}},
    {2500800,2502805,12504802,12501800,2500800,0,0,{62.81f,108.87f,-371.63f}},
    {2420810,2422815,12424702,12421700,2420811,12424790,2420811,{36.4748f,47.6723f,284.8857f},12424790},
    {2420800,0,12424802,12421800,2420800,0,0,{},12424980},
    {3400800,3402805,13404802,9471,3400801,13404825,3400801,{-407.93f,1503.f,-721.36f},13404822,13404825},
    // Crawling Laurence is a TAE/collision-mask event, not an actor replacement.
    {3400850,3402855,13404852,13401850,3400850,0,0,{-433.1f,1535.61f,-261.59f},0,13404875},
    {3500850,3502815,13504852,13501850,3500850,0,0,{-380.91f,1592.8f,-824.55f}},
    // Maria first entry is an interaction with her chair (13500948), not walking.
    {3500800,0,13504802,13501800,3500800,0,0,{},0,0,3500905},
    {3600800,3602805,13604802,13601800,3600801,13604820,3600801,{-700.2f,1575.4f,-956.2f},13604820},
};
inline const Script* ScriptFor(unsigned boss) {
    for(const auto& s:scripts) if(s.boss==boss) return &s;
    return nullptr;
}
}
