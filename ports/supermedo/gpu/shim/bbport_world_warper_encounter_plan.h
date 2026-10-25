#pragma once
#include <algorithm>
#include <cmath>
namespace BbWorldWarperEncounter {
struct Stage { const char* name; double seconds; float range; int hp_percent; bool provoke; };
inline constexpr Stage logarius_plan[] = {
    {"ranged",20,14,100,false}, {"close",25,3,100,true},
    {"lower-health-ranged",20,14,60,false}, {"lower-health-close",25,3,60,true},
    {"low-health-close",25,3,30,true},
};
inline constexpr Stage ebrietas_plan[] = {
    {"wake",15,4,100,true}, {"ranged",20,12,100,false}, {"close",25,4,100,true},
    {"lower-health-ranged",20,12,60,false}, {"low-health-close",25,4,30,true},
};
inline unsigned StageAt(const Stage* plan, unsigned count, double elapsed, double* local) {
    if (!std::isfinite(elapsed) || elapsed<0) { *local=0; return count; }
    for (unsigned i=0; i<count; ++i) {
        if (elapsed<plan[i].seconds) { *local=elapsed; return i; }
        elapsed-=plan[i].seconds;
    }
    *local=0; return count;
}
inline int TargetHp(int baseline, int percent) {
    return baseline>1 && percent>0 && percent<100
        ? std::max(1,static_cast<int>(static_cast<long long>(baseline)*percent/100)) : baseline;
}
inline double Duration(const Stage* plan, unsigned count) {
    double total=0; for (unsigned i=0;i<count;++i) total+=plan[i].seconds; return total;
}
}
