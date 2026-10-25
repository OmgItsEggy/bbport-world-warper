#pragma once
#include "bbport_world_warper_boss_catalog.h"
#include "bbport_world_warper_boss_scripts.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace BbWorldWarperEncounter {
struct Stage { const char* name; double seconds; float range; int hp_percent; bool provoke; bool transition=false; unsigned event_flag=0; };
inline std::vector<Stage> BuildPlan(const BossSpec& boss) {
    const bool large=Matches(boss,2420800) || Matches(boss,3400800) || Matches(boss,3400850);
    const float close=large ? 4.f : 3.f, ranged=Matches(boss,2500800) ? 14.f : 12.f;
    std::vector<Stage> plan;
    plan.push_back({"phase1-ranged",20,ranged,100,false});
    plan.push_back({"phase1-close",20,close,100,!boss.special});
    if (boss.special) {
        plan.push_back({"scripted-observation",45,8,100,false});
        return plan; // Group/shared-bar/chase mechanics: observation, no arbitrary HP writes.
    }
    const auto* script=ScriptFor(boss.ids[0]);
    const int thresholds[]={boss.phase2,boss.phase3};
    for (unsigned i=0;i<2;++i) if (thresholds[i]) {
        const int sample=std::max(1,thresholds[i]-5);
        plan.push_back({i==0?"phase2-transition-request":"phase3-transition-request",15,8,sample,false,true,
                        script?(i==0?script->phase2:script->phase3):0});
        plan.push_back({i==0?"phase2-ranged-request":"phase3-ranged-request",25,ranged,sample,false});
        plan.push_back({i==0?"phase2-close-request":"phase3-close-request",25,close,sample,false});
    }
    return plan;
}
inline unsigned StageAt(const Stage* plan,unsigned count,double elapsed,double* local) {
    if (!std::isfinite(elapsed)||elapsed<0) { *local=0; return count; }
    for (unsigned i=0;i<count;++i) {
        if (elapsed<plan[i].seconds) { *local=elapsed; return i; }
        elapsed-=plan[i].seconds;
    }
    *local=0; return count;
}
inline int TargetHp(int maximum,int percent) {
    return maximum>1 && percent>0 && percent<100
        ? std::max(1,static_cast<int>(static_cast<long long>(maximum)*percent/100)) : maximum;
}
inline double Duration(const Stage* plan,unsigned count) {
    double total=0; for(unsigned i=0;i<count;++i) total+=plan[i].seconds; return total;
}
}
