#pragma once
#include <cstdint>
namespace BbWorldWarperEncounter {
// Thresholds/source caveats: docs/BOSS-PHASE-REFERENCE.md. Samples use 5% below
// the documented boundary. IDs follow the pinned reference's boss-entity census.
struct BossSpec { const char* name; std::uint32_t ids[8]; int phase2,phase3; bool wake,special; };
inline constexpr BossSpec bosses[] = {
    {"ClericBeast",{2410800},70,0,false,false},
    {"Gascoigne",{2410810,2410811},80,34,false,false},
    {"BloodStarvedBeast",{2300800},67,33,false,false},
    {"Amelia",{2400800},50,0,false,false},
    {"Witches",{2200800,2200801},0,0,false,true},
    {"Paarl",{2300810},67,0,false,false},
    {"Shadows",{2700800,2700801,2700802},0,0,false,true},
    {"Rom",{3200800},75,50,false,false},
    {"OneReborn",{2800800,2800801,2800802,2800803},0,0,false,false},
    {"Amygdala",{3300800},70,30,false,false},
    {"Micolash",{2600850},0,0,false,true},
    {"WetNurse",{2600800,2600801,2600802},70,0,false,false},
    {"Logarius",{2500800,2500801,2500802},70,0,false,false},
    {"Emissary",{2420810,2420811},60,30,false,false},
    {"Ebrietas",{2420800},50,0,true,false},
    {"Gehrman",{2100800},75,50,false,false},
    {"MoonPresence",{2100810},70,0,false,false},
    {"Ludwig",{3400800,3400801},80,50,false,false},
    {"Laurence",{3400850},80,40,false,false},
    {"LivingFailures",{3500850,3500851,3500852,3500853,3500854},0,0,false,true},
    {"Maria",{3500800},60,30,false,false},
    {"Orphan",{3600800,3600801},50,0,false,false},
};
inline bool Matches(const BossSpec& boss,unsigned id) {
    for (auto candidate:boss.ids) if (candidate && candidate==id) return true;
    return false;
}
}
