#include "bbport_world_warper_encounter_plan.h"
#include <cassert>
#include <limits>
int main() {
    using namespace BbWorldWarperEncounter;
    double local=0;
    assert(StageAt(logarius_plan,5,0,&local)==0 && local==0);
    assert(logarius_plan[0].range>logarius_plan[1].range);
    assert(StageAt(logarius_plan,5,19.99,&local)==0);
    assert(StageAt(logarius_plan,5,20,&local)==1 && local==0);
    assert(StageAt(logarius_plan,5,45,&local)==2 && local==0);
    assert(StageAt(logarius_plan,5,115,&local)==5);
    assert(StageAt(logarius_plan,5,-1,&local)==5);
    assert(StageAt(logarius_plan,5,std::numeric_limits<double>::quiet_NaN(),&local)==5);
    assert(Duration(logarius_plan,5)==115);
    assert(Duration(ebrietas_plan,5)==105);
    assert(ebrietas_plan[0].provoke && !ebrietas_plan[1].provoke);
    assert(TargetHp(24048,60)==14428);
    assert(TargetHp(1,30)==1);
    assert(TargetHp(2,30)==1);
    assert(TargetHp(24048,100)==24048);
    for (const auto* plan : {logarius_plan,ebrietas_plan}) {
        int previous=100;
        for (unsigned i=0;i<5;++i) {
            assert(plan[i].seconds>=15 && plan[i].range>=3 && plan[i].range<=14);
            assert(plan[i].hp_percent<=previous && plan[i].hp_percent>0);
            previous=plan[i].hp_percent;
        }
    }
}
