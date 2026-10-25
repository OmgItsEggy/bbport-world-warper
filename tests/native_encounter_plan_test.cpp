#include "bbport_world_warper_encounter_plan.h"
#include <cassert>
#include <limits>
#include <set>
#include <string>
int main() {
    using namespace BbWorldWarperEncounter;
    static_assert(sizeof(bosses)/sizeof(bosses[0])==22);
    std::set<std::string> names;
    for(const auto& boss:bosses) {
        assert(names.insert(boss.name).second);
        for(auto id:boss.ids) assert(id/100000!=29); // no Chalice entity family
        const auto plan=BuildPlan(boss);
        assert(plan.size()>=2);
        int previous=100;
        double start=0;
        for(unsigned i=0;i<plan.size();++i) {
            const auto& s=plan[i]; double local=0;
            assert(StageAt(plan.data(),plan.size(),start,&local)==i && local==0);
            assert(s.seconds>=15 && s.hp_percent<=previous && s.hp_percent>0);
            assert(!s.transition || !s.provoke);
            if(boss.special) assert(s.hp_percent==100);
            previous=s.hp_percent; start+=s.seconds;
        }
        double local=0;
        assert(Duration(plan.data(),plan.size())==start);
        assert(StageAt(plan.data(),plan.size(),start,&local)==plan.size());
        assert(StageAt(plan.data(),plan.size(),-1,&local)==plan.size());
        assert(StageAt(plan.data(),plan.size(),std::numeric_limits<double>::quiet_NaN(),&local)==plan.size());
        if(Matches(boss,2500800)) {
            assert(plan[0].range==14 && plan[1].range==3);
            assert(plan.size()==5 && plan[2].hp_percent==65 && plan[2].transition);
        }
        if(Matches(boss,2420800)) {
            assert(plan[0].provoke && plan[3].hp_percent==45 && plan[3].transition);
            assert(plan.size()==6 && Duration(plan.data(),plan.size())==120);
        }
    }
    assert(TargetHp(1000,65)==650);
    assert(TargetHp(1,30)==1 && TargetHp(2,30)==1);
}
