#pragma once
#include "bbport_world_warper_encounter_plan.h"
#include "bbport_world_warper_observe.h"
#include "bbport_world_warper_compile.h"
#include "bbport_world_warper_movement.h"
#include <set>
#include <string>
namespace BbWorldWarperEncounter {
inline std::set<std::string> attempted;
inline const char* state_path=nullptr;
inline void LoadState(const char* path) {
    state_path=path; attempted.clear();
    if (path) if (auto* file=std::fopen(path,"r")) {
        char name[64]{};
        while(std::fscanf(file,"%63s",name)==1) attempted.insert(name);
        std::fclose(file);
    }
    for (const auto& boss:bosses) std::printf("[warper] boss-roster boss=%s previous-attempt=%s special=%s\n",
        boss.name,attempted.count(boss.name)?"yes":"no",boss.special?"group-or-scripted":"no");
}
class Run {
    std::uint64_t target=0,player=0;
    unsigned entity=0,stage=~0u,gaps=0;
    int maximum_hp=0;
    double next_sample=0;
    bool phases=false,failed=false,aborted=false;
    unsigned reacquires=0;
    double retry_at=0;
    const BossSpec* spec=nullptr;
    std::vector<Stage> plan;
    std::set<int> animations,baseline_animations;
    BbWorldWarperCompile::Snapshot counts{};
    template<class T> static T Read(std::uint64_t address) { return BbWorldWarperObserve::Read<T>(address); }
    std::uint64_t Damage() const {
        if (!target || Read<unsigned>(target+0x1f0)!=entity) return 0;
        const auto mods=Read<std::uint64_t>(target+0x3b0);
        return mods ? Read<std::uint64_t>(mods+0x20) : 0;
    }
    bool Reacquire() {
        // Actor replacements/teleports: rescan only after the current target disappears.
        // Never keep writing through a stale entity pointer.
        BbWorldWarperObserve::Census(world,player,lamp,"start-reacquire");
        for (const auto& actor:BbWorldWarperObserve::actors) if (Matches(*spec,actor.entity)) {
            const auto mods=Read<std::uint64_t>(actor.chr+0x3b0);
            const auto dm=mods?Read<std::uint64_t>(mods+0x20):0;
            if (dm && Read<int>(dm+0xf8)>0) { target=actor.chr; entity=actor.entity; return true; }
        }
        return false;
    }
    std::uint64_t world=0;
    unsigned lamp=0;
    void Sample() {
        const auto mods=Read<std::uint64_t>(target+0x3b0);
        const auto ta=mods?Read<std::uint64_t>(mods+0x18):0;
        if(ta) for(unsigned i=0;i<10;++i) {
            const int anim=Read<int>(ta+0x20+0x10ull*i);
            if(anim>0) animations.insert(anim);
        }
    }
    void Report() {
        if(!spec || stage>=plan.size()) return;
        const auto c=BbWorldWarperCompile::Read(); const auto dm=Damage();
        std::printf("[warper] boss-stage-result boss=%s stage=%s animations=%zu pipelines=%llu compiles=%llu hp=%d; phase/attack classification unverified\n",
            spec->name,plan[stage].name,animations.size(),
            static_cast<unsigned long long>(c.pipelines-counts.pipelines),
            static_cast<unsigned long long>(c.events-counts.events),dm?Read<int>(dm+0xf8):-1);
        std::printf("[warper] boss-animation-ids boss=%s stage=%s",spec->name,plan[stage].name);
        for(int id:animations) std::printf(" %d",id);
        std::printf("\n"); std::fflush(stdout);
    }
public:
    bool Active() const { return spec!=nullptr; }
    double Seconds() const { return aborted?0:Duration(plan.data(),static_cast<unsigned>(plan.size())); }
    void Begin(unsigned stop,std::uint64_t local_player,bool allow_phases,std::uint64_t local_world=0,unsigned expected_boss=0) {
        *this=Run{}; player=local_player; phases=allow_phases; world=local_world; lamp=stop;
        float me[3]{}; if(!BbWorldWarperObserve::Position(player,me)) return;
        float nearest=100;
        for(const auto& boss:bosses) if(!attempted.count(boss.name)) {
            if(expected_boss && !Matches(boss,expected_boss)) continue;
            for(const auto& actor:BbWorldWarperObserve::actors) if(Matches(boss,actor.entity)) {
                float p[3]{};
                if(!BbWorldWarperObserve::Position(actor.chr,p)||std::fabs(me[1]-p[1])>8) continue;
                const float distance=std::hypot(me[0]-p[0],me[2]-p[2]);
                if(distance<nearest) { nearest=distance; spec=&boss; target=actor.chr; entity=actor.entity; }
            }
        }
        if(!spec) { std::printf("[warper] boss-stop lamp=%u expected=%u missing-inactive-already-attempted-or-out-of-bounds; no substitute boss selected\n",lamp,expected_boss); return; }
        const auto dm=Damage();
        const int hp=dm?Read<int>(dm+0xf8):0;
        const int maxhp=dm?Read<int>(dm+0xfc):0; // pinned reference party_balance.cpp:16
        if(hp<=0 || maxhp<hp || maxhp<=1) { spec=nullptr; std::printf("[warper] boss-stop lamp=%u invalid-current-or-maximum-hp\n",lamp); return; }
        maximum_hp=maxhp; plan=BuildPlan(*spec);
        std::printf("[warper] boss-run boss=%s stages=%zu max-hp=%d hp-stages=%s special=%s; transitions unverified\n",
            spec->name,plan.size(),maximum_hp,phases&&!spec->special?"enabled-test-only":"disabled",
            spec->special?"group-or-scripted-observation-only":"no");
    }
    void Finish() {
        Report(); stage=static_cast<unsigned>(plan.size());
        if(!spec) return;
        std::printf("[warper] boss-attempt boss=%s result=%s; NOT exhaustive coverage\n",spec->name,
            failed?"incomplete":spec->special?"scripted-observation-only":"stages-attempted");
        attempted.insert(spec->name);
        if(state_path) if(auto* file=std::fopen(state_path,"a")) { std::fprintf(file,"%s\n",spec->name); std::fclose(file); }
    }
    BbWorldWarperMovement::Input Tick(double elapsed) {
        if(!spec || aborted) return {128,128,128,128,0};
        auto dm=Damage();
        if(!dm || Read<int>(dm+0xf8)<=0) {
            if(elapsed<retry_at) return {128,128,128,128,0};
            retry_at=elapsed+2;
            if(world && reacquires++<3 && Reacquire()) { dm=Damage(); reacquires=0; }
            else {
                failed=true;
                if(!world || reacquires>=3) {
                    aborted=true;
                    std::printf("[warper] boss-run boss=%s target-lost; bounded reacquisition ended; incomplete\n",spec->name);
                }
                return {128,128,128,128,0};
            }
        }
        double local=0; const auto next=StageAt(plan.data(),static_cast<unsigned>(plan.size()),elapsed,&local);
        if(next!=stage) {
            Report(); stage=next;
            if(stage>=plan.size()) return {128,128,128,128,0};
            counts=BbWorldWarperCompile::Read(); animations.clear(); gaps=0;
            Sample(); baseline_animations=animations; animations.clear();
            const auto& s=plan[stage];
            const bool placed=BbWorldWarperObserve::ApproachBoss(player,target,entity,spec->name,s.range);
            failed|=!placed;
            bool hp_verified=false;
            if(phases && !spec->special && s.hp_percent<100) {
                const int desired=TargetHp(maximum_hp,s.hp_percent), current=Read<int>(dm+0xf8);
                hp_verified=current>0 && current<=desired;
                if(current>desired) hp_verified=BbWorldWarperMemory::Write(dm+0xf8,&desired,sizeof desired)&&Read<int>(dm+0xf8)==desired;
                failed|=!hp_verified;
            }
            std::printf("[warper] boss-stage boss=%s stage=%s range=%.1f placement=%s hp-percent=%d hp-request=%s\n",
                spec->name,s.name,s.range,placed?"requested":"FAILED",s.hp_percent,
                s.hp_percent==100?"unchanged":!phases?"disabled":hp_verified?"verified-value-phase-unverified":"FAILED");
            next_sample=elapsed;
        }
        if(stage>=plan.size()) return {128,128,128,128,0};
        if(elapsed>=next_sample) {
            next_sample=elapsed+0.5; Sample();
            for(int id:baseline_animations) animations.erase(id);
        }
        const auto& s=plan[stage];
        // Never attack, sweep the camera or perform gap corrections during the transition hold.
        if(s.transition) return {128,128,128,128,0};
        if(gaps<2 && local>=10*(gaps+1)) {
            ++gaps; float p[3]{}, me[3]{};
            if(BbWorldWarperObserve::Position(target,p)&&BbWorldWarperObserve::Position(player,me)&&
               std::fabs(p[1]-me[1])<=1.5f&&std::fabs(std::hypot(p[0]-me[0],p[2]-me[2])-s.range)>5)
                BbWorldWarperObserve::ApproachBoss(player,target,entity,spec->name,s.range);
        }
        const bool hit=s.provoke&&((local>=8&&local<8.2)||(local>=10&&local<10.2));
        return {128,128,local>s.seconds-4?180:128,128,hit?0x800u:0u};
    }
};
}
