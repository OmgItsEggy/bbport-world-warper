#pragma once
#include "bbport_world_warper_encounter_plan.h"
#include "bbport_world_warper_observe.h"
#include "bbport_world_warper_compile.h"
#include "bbport_world_warper_movement.h"
#include <set>
namespace BbWorldWarperEncounter {
class Run {
    std::uint64_t target=0, player=0;
    unsigned entity=0, stage=5, gaps=0;
    int baseline_hp=0;
    double next_sample=0;
    bool phases=false;
    const char* name="none";
    const Stage* plan=nullptr;
    std::set<int> animations;
    BbWorldWarperCompile::Snapshot counts{};
    template<class T> static T Read(std::uint64_t address) { return BbWorldWarperObserve::Read<T>(address); }
    std::uint64_t Damage() const {
        if (!target || Read<unsigned>(target+0x1f0)!=entity) return 0;
        const auto modules=Read<std::uint64_t>(target+0x3b0);
        return modules ? Read<std::uint64_t>(modules+0x20) : 0;
    }
    void Sample() {
        const auto mods=Read<std::uint64_t>(target+0x3b0);
        const auto ta=mods ? Read<std::uint64_t>(mods+0x18) : 0;
        if (!ta) return;
        for (unsigned i=0;i<10;++i) {
            const int anim=Read<int>(ta+0x20+0x10ull*i);
            if (anim>0) animations.insert(anim);
        }
    }
    void Report() {
        if (!plan || stage>=5) return;
        const auto now=BbWorldWarperCompile::Read();
        const auto damage=Damage();
        std::printf("[warper] boss-stage-result boss=%s stage=%s animations=%zu pipelines=%llu compiles=%llu hp=%d; animation IDs are not attack/phase classification\n",
            name,plan[stage].name,animations.size(),
            static_cast<unsigned long long>(now.pipelines-counts.pipelines),
            static_cast<unsigned long long>(now.events-counts.events), damage ? Read<int>(damage+0xf8) : -1);
        std::printf("[warper] boss-animation-ids boss=%s stage=%s",name,plan[stage].name);
        for (int id : animations) std::printf(" %d",id);
        std::printf("\n"); std::fflush(stdout);
    }
public:
    bool Active() const { return plan!=nullptr; }
    double Seconds() const { return plan ? Duration(plan,5) : 0; }
    void Begin(unsigned lamp, std::uint64_t local_player, bool allow_phases) {
        *this=Run{};
        player=local_player; phases=allow_phases;
        if ((lamp==2422951 || lamp==2422952) && BbWorldWarperObserve::ebrietas) {
            target=BbWorldWarperObserve::ebrietas; entity=2420800; name="Ebrietas"; plan=ebrietas_plan;
        } else if (lamp==2502952 && BbWorldWarperObserve::logarius) {
            target=BbWorldWarperObserve::logarius; entity=2500800; name="Logarius"; plan=logarius_plan;
        }
        const auto damage=Damage();
        baseline_hp=damage ? Read<int>(damage+0xf8) : 0;
        // Check the same geometric bounds as placement before enabling a run.
        float boss[3]{}, me[3]{};
        if (!plan || baseline_hp<=1 || !BbWorldWarperObserve::Position(target,boss) ||
            !BbWorldWarperObserve::Position(player,me) || std::fabs(boss[1]-me[1])>8 ||
            std::hypot(boss[0]-me[0],boss[2]-me[2])>100) {
            if (lamp==2422951 || lamp==2422952 || lamp==2502952)
                std::printf("[warper] boss-run boss=%s lamp=%u unavailable-or-out-of-bounds; staged coverage not attempted\n",name,lamp);
            plan=nullptr; return;
        }
        std::printf("[warper] boss-run boss=%s stages=5 hp-stages=%s; requested health levels do NOT verify phases\n",name,phases?"enabled (disposable character only)":"disabled");
    }
    void Finish() { Report(); stage=5; }
    BbWorldWarperMovement::Input Tick(double elapsed) {
        if (!plan) return {128,128,128,128,0};
        const auto damage=Damage();
        if (!damage || Read<int>(damage+0xf8)<=0) {
            if (stage<5) { Report(); stage=5; }
            return {128,128,128,128,0};
        }
        double local=0;
        const unsigned next=StageAt(plan,5,elapsed,&local);
        if (next!=stage) {
            Report(); stage=next;
            if (stage>=5) return {128,128,128,128,0};
            counts=BbWorldWarperCompile::Read(); animations.clear(); gaps=0;
            // Seed the ring baseline; later samples report newly observed animation IDs.
            Sample(); const auto baseline_anims=animations;
            animations.clear(); baseline_animations=baseline_anims;
            const auto& s=plan[stage];
            const bool placed=BbWorldWarperObserve::ApproachBoss(player,target,entity,name,s.range);
            bool hp_verified=false;
            if (phases && s.hp_percent<100) {
                const int desired=TargetHp(baseline_hp,s.hp_percent);
                const int current=Read<int>(damage+0xf8);
                hp_verified=current>0 && current<=desired;
                if (current>desired) hp_verified=BbWorldWarperMemory::Write(damage+0xf8,&desired,sizeof desired) && Read<int>(damage+0xf8)==desired;
            }
            std::printf("[warper] boss-stage boss=%s stage=%s range=%.1f placement=%s hp-percent=%d hp-request=%s\n",
                name,s.name,s.range,placed?"requested":"FAILED",s.hp_percent,
                s.hp_percent==100 ? "unchanged" : !phases ? "disabled" : hp_verified?"verified value; phase unverified":"FAILED");
            next_sample=elapsed;
        }
        if (stage>=5) return {128,128,128,128,0};
        if (elapsed>=next_sample) {
            next_sample=elapsed+0.5;
            Sample();
            for (int id : baseline_animations) animations.erase(id);
        }
        const auto& s=plan[stage];
        if (gaps<2 && local>=10*(gaps+1)) {
            ++gaps;
            float boss[3]{}, me[3]{};
            if (BbWorldWarperObserve::Position(target,boss) && BbWorldWarperObserve::Position(player,me) &&
                std::fabs(boss[1]-me[1])<=1.5f &&
                std::fabs(std::hypot(boss[0]-me[0],boss[2]-me[2])-s.range)>5)
                BbWorldWarperObserve::ApproachBoss(player,target,entity,name,s.range);
        }
        // Leave the camera steady for most AI activity; a short sweep ends each stage.
        const bool hit=s.provoke && ((local>=8 && local<8.2) || (local>=10 && local<10.2));
        return {128,128,local>s.seconds-4 ? 180 : 128,128,hit?0x800u:0u};
    }
private:
    std::set<int> baseline_animations;
};
}
