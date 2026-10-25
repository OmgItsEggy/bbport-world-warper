#pragma once
#include "bbport_world_warper_encounter_plan.h"
#include "bbport_world_warper_observe.h"
#include "bbport_world_warper_compile.h"
#include "bbport_world_warper_events.h"
#include "bbport_world_warper_walk.h"
#include "bbport_world_warper_entry.h"
#include <set>
#include <string>
namespace BbWorldWarperEncounter {
inline std::set<std::string> attempted;
inline const char* state_path=nullptr;
inline void LoadState(const char* path) {
    state_path=path; attempted.clear();
    if(path) if(auto* f=std::fopen(path,"r")) {
        char name[64]{}; while(std::fscanf(f,"%63s",name)==1) attempted.insert(name); std::fclose(f);
    }
    for(const auto& b:bosses) std::printf("[warper] boss-roster boss=%s previous-attempt=%s special=%s\n",
        b.name,attempted.count(b.name)?"yes":"no",b.special?"group-or-scripted":"no");
}
class Run {
    using Input=BbWorldWarperMovement::Input;
    static constexpr Input neutral{128,128,128,128,0};
    const BossSpec* spec=nullptr;
    const Script* script=nullptr;
    std::uint64_t player=0,world=0,target=0;
    unsigned lamp=0,entity=0,stage=0;
    bool activated=false,done=false,failed=false,phases=false,stage_started=false,reported=false;
    bool unverified=false,transition_confirmed=false,transition_preexisting=false;
    double stage_began=0,observe_began=-1,next_scan=0,next_sample=0,missing_since=-1;
    Walker walker;
    FloorGuard floor;
    const BbWorldWarperEntry::Gate* gate=nullptr;
    unsigned entry_step=0;
    double entry_at=0,wake_at=-1,hit_at=-1,activation_at=-1;
    double wake_progress_at=-1;
    float wake_nearest=100000;
    int wake_logged=-1;
    bool first_seen=false,hit_preexisting=false,entry_only=false;
    std::vector<Stage> plan;
    std::set<int> animations,baseline;
    BbWorldWarperCompile::Snapshot counts{};
    template<class T> static T Read(std::uint64_t a) { return BbWorldWarperObserve::Read<T>(a); }
    static bool Pos(std::uint64_t chr,Point* p) {
        float v[3]{}; if(!BbWorldWarperObserve::Position(chr,v)) return false;
        *p={v[0],v[1],v[2]}; return true;
    }
    static std::uint64_t Damage(std::uint64_t chr,unsigned id) {
        if(!chr||Read<unsigned>(chr+0x1f0)!=id) return 0;
        const auto mods=Read<std::uint64_t>(chr+0x3b0);
        return mods?Read<std::uint64_t>(mods+0x20):0;
    }
    static std::uint64_t Actor(unsigned id) {
        for(const auto& a:BbWorldWarperObserve::actors)
            if(a.entity==id && Read<unsigned>(a.chr+0x1f0)==id && (id==3500905 || Damage(a.chr,id))) return a.chr;
        return 0;
    }
    bool Flag(unsigned id,bool* value) {
        if(BbWorldWarperEvents::Read(id,value)) return true;
        Abort("event-flag-read-unavailable"); return false;
    }
    bool SelectTarget() {
        unsigned id=script->boss;
        if(script->swap_flag) {
            bool swapped=false; if(!Flag(script->swap_flag,&swapped)) return false;
            if(swapped) id=script->second_actor;
        }
        // Shared health dummy is not the on-screen Living Failure.
        if(id==3500850) {
            id=3500851;
            for(unsigned candidate=3500851;candidate<=3500854;++candidate) {
                auto chr=Actor(candidate); auto dm=Damage(chr,candidate);
                if(dm && Read<int>(dm+0xf8)>0) { id=candidate; break; }
            }
        }
        target=Actor(id); entity=id;
        const auto dm=Damage(target,entity);
        return dm && Read<int>(dm+0xf8)>0;
    }
    void Sample() {
        if(!target||Read<unsigned>(target+0x1f0)!=entity) return;
        const auto mods=Read<std::uint64_t>(target+0x3b0);
        const auto ta=mods?Read<std::uint64_t>(mods+0x18):0;
        if(ta) for(unsigned i=0;i<10;++i) {
            const int anim=Read<int>(ta+0x20+0x10ull*i);
            if(anim>0 && !baseline.count(anim)) animations.insert(anim);
        }
    }
    void Report() {
        if(!stage_started || stage>=plan.size()) return;
        const auto c=BbWorldWarperCompile::Read(); const auto dm=Damage(target,entity);
        std::printf("[warper] boss-stage-result boss=%s stage=%s animations=%zu pipelines=%llu compiles=%llu hp=%d event=%u event-observed=%s; shader/attack coverage unverified\n",
            spec->name,plan[stage].name,animations.size(),
            static_cast<unsigned long long>(c.pipelines-counts.pipelines),
            static_cast<unsigned long long>(c.events-counts.events),dm?Read<int>(dm+0xf8):-1,
            plan[stage].event_flag,transition_confirmed?"yes":transition_preexisting?"pre-existing":"unverified");
        std::printf("[warper] boss-animation-ids boss=%s stage=%s",spec->name,plan[stage].name);
        for(int id:animations) std::printf(" %d",id);
        std::printf("\n"); stage_started=false;
    }
    bool SetHealth(unsigned id,int percent) {
        const auto chr=Actor(id),dm=Damage(chr,id);
        const int maximum=dm?Read<int>(dm+0xfc):0,current=dm?Read<int>(dm+0xf8):0;
        if(current<=0 || maximum<current || maximum<=1 || maximum>10000000) return false;
        const int desired=TargetHp(maximum,percent);
        if(current<=desired) return true; // never heal or kill
        return BbWorldWarperMemory::Write(dm+0xf8,&desired,sizeof desired)&&Read<int>(dm+0xf8)==desired;
    }
    bool RequestHealth(int percent) {
        if(!phases || !activated) return false;
        // Match both the redirected health owner and the current form's ratio
        // used by EMEVD. An inactive actor is NEVER used as a position target.
        bool ok=SetHealth(script->health_owner,percent);
        if(entity!=script->health_owner) ok=SetHealth(entity,percent)&&ok;
        return ok;
    }
    void Next(double now) {
        Report(); ++stage; stage_began=now; observe_began=-1; walker.Reset();
        if(stage>=plan.size()) done=true;
    }
    Input Walk(double now,Point me,Point goal,float radius,bool strict) {
        if(Distance(me,goal)>100 || std::fabs(me.y-goal.y)>12) {
            if(strict) Abort("walk-target-outside-local-arena");
            return neutral;
        }
        auto input=walker.Toward(now,me,goal,radius);
        if(walker.stalled && strict) Abort("entrance-walk-stalled-no-random-reposition");
        return done?neutral:input;
    }
    void EntryStep(unsigned next,double now) {
        entry_step=next; entry_at=now; walker.Reset();
        std::printf("[warper] boss-entry boss=%s step=%u first-event=%s\n",spec->name,next,first_seen?"already-seen-fog-entry":"new-first-entry");
    }
    Input EnterGate(double now,Point me) {
        using namespace BbWorldWarperEntry;
        const Point outside=GatePoint(*gate,-3),near=GatePoint(*gate,-1.6f),inside=GatePoint(*gate,5);
        if(entry_step==0) {
            if(GateSide(*gate,me)>-1.f) {
                // A boss-defeat lamp can be behind a one-way fog. One fixed
                // setup OUTSIDE the authored door; never a boss-relative hop.
                if(!SetupEntryStart(player,gate->boss)) { Abort("authored-outside-entry-setup-failed"); return neutral; }
                EntryStep(1,now); return neutral;
            }
            EntryStep(2,now);
        }
        if(entry_step==1) {
            if(now-entry_at<0.6) return neutral;
            if(Distance(me,outside)>0.8f || std::fabs(me.y-outside.y)>1.25f) {
                Abort("authored-entry-setup-did-not-settle"); return neutral;
            }
            EntryStep(2,now);
        }
        if(entry_step==2) {
            if(Distance(me,near)>0.6f) return Walk(now,me,near,0.6f,true);
            EntryStep(first_seen?3:4,now);
        }
        if(entry_step==3) {
            // The game, not a forced flag, performs FaceEntityAndForceAnimation
            // and crosses the gate after its normal Cross interaction.
            if(now-entry_at<3) return {128,128,128,128,now-entry_at<0.3?0x4000u:0u};
            EntryStep(4,now);
        }
        if(entry_step==4) {
            if(Distance(me,inside)>0.8f) return Walk(now,me,inside,0.8f,true);
            EntryStep(5,now);
        }
        if(entry_step==5) {
            if(!first_seen && script->region && Distance(me,script->entry)>0.8f)
                return Walk(now,me,script->entry,0.8f,true);
            // First-entry animations (Cleric Beast: 110 script frames) still
            // run after reaching the trigger. Travel must not consume their wait.
            EntryStep(6,now);
            std::printf("[warper] boss-entry-trigger boss=%s player=%.2f,%.2f,%.2f waiting-after-arrival=yes\n",spec->name,me.x,me.y,me.z);
        }
        if(now-entry_at>10) Abort("entrance-crossed-but-battle-event-not-observed");
        return neutral;
    }
    Input WakeByHit(double now,Point me,Point boss) {
        const auto event=BbWorldWarperEntry::HitEvent(script->boss);
        bool hit=false; if(!Flag(event,&hit)) return neutral;
        if(hit && !hit_preexisting) {
            if(hit_at<0) {
                hit_at=now;
                std::printf("[warper] boss-contact boss=%s hit-event=%u observed=newly-set\n",spec->name,event);
            }
            if(now-hit_at>10) Abort("hit-confirmed-but-battle-activation-missing");
            return neutral;
        }
        const float distance=Distance(me,boss);
        if(distance>100 || std::fabs(me.y-boss.y)>8) { Abort("wake-target-outside-local-floor"); return neutral; }
        if(wake_progress_at<0 || distance<wake_nearest-0.05f) { wake_nearest=distance; wake_progress_at=now; }
        // Body contact is expected here, but keep updating the measured stick
        // basis if the follow-camera changes while we retry a real attack.
        if(walker.stalled && distance<=8) walker.PermitContact(now);
        auto drive=walker.Toward(now,me,boss,0.5f);
        if(distance>8) {
            if(walker.stalled || now>35) Abort("wake-contact-approach-not-reached");
            return done?neutral:drive;
        }
        if(wake_at<0) wake_at=now;
        const double local=now-wake_at;
        const int attempt=static_cast<int>(local/2.4);
        if(attempt>=6) { Abort("wake-attacks-did-not-trigger-battle"); return neutral; }
        const double phase=local-attempt*2.4;
        // Walk into contact and turn naturally through input, rather than a
        // raw yaw write plus an arbitrary 3.5 m stand-off. Release between hits.
        if(phase<1) return drive;
        float facing=-1;
        const bool aligned=BbWorldWarperObserve::Facing(player,target,&facing)&&facing>0.75f;
        const bool contact_range=distance<=2.f || (walker.Calibrated() && now-wake_progress_at>=0.6);
        if(wake_logged!=attempt) {
            wake_logged=attempt;
            std::printf("[warper] boss-wake-attempt boss=%s attempt=%d distance=%.2f facing=%.2f attack=%s player=%.2f,%.2f,%.2f target=%.2f,%.2f,%.2f hit-event-at-start=%s\n",
                spec->name,attempt+1,distance,facing,!aligned?"withheld-not-facing":contact_range?"R1-request":"withheld-still-approaching",me.x,me.y,me.z,boss.x,boss.y,boss.z,hit_preexisting?"already-set":"not-set");
        }
        if(phase<1.35) { if(aligned && contact_range) drive.buttons=0x800; return drive; }
        return neutral;
    }
public:
    bool Active() const { return spec!=nullptr; }
    bool Done() const { return done; }
    bool Failed() const { return failed; }
    double Seconds() const { return done?0:900; } // boot uses Done(), never a nominal phase timer
    unsigned TargetEntity() const { return entity; }
    void AfterCutscene() { walker.Reset(); floor.AfterCutscene(); }
    void Abort(const char* reason) {
        if(done) return;
        failed=true; done=true;
        std::printf("[warper] boss-failure boss=%s stage=%s reason=%s\n",spec?spec->name:"unknown",
            !activated?"entrance":stage<plan.size()?plan[stage].name:"finished",reason);
        std::fflush(stdout);
    }
    void Begin(unsigned stop,std::uint64_t local_player,bool allow_phases,std::uint64_t local_world=0,unsigned expected_boss=0,bool activation_only=false) {
        *this=Run{}; lamp=stop; player=local_player; world=local_world; phases=allow_phases;
        entry_only=activation_only;
        for(const auto& b:bosses) if(b.ids[0]==expected_boss) { spec=&b; break; }
        if(!spec || attempted.count(spec->name)) { spec=nullptr; return; }
        script=ScriptFor(expected_boss);
        if(!script) { Abort("no-audited-entry-script"); return; }
        Point me{};
        if(!Pos(player,&me)) { Abort("player-position-unavailable"); return; }
        floor.Begin(me,*script); plan=BuildPlan(*spec);
        bool dead=false;
        if(!Flag(script->dead,&dead)) return;
        if(dead) { Abort("boss-defeated-in-seed-save"); return; }
        gate=BbWorldWarperEntry::GateFor(expected_boss);
        if(gate && !Flag(gate->first_event,&first_seen)) return;
        if(auto event=BbWorldWarperEntry::HitEvent(expected_boss)) {
            if(!Flag(event,&hit_preexisting)) return;
            // Initial Ebrietas needs a real hit; only a repeat visit uses fog.
            if(expected_boss==2420800 && !first_seen) gate=nullptr;
        }
        std::printf("[warper] boss-entry-state boss=%s first-event=%s hit-event=%s player=%.2f,%.2f,%.2f\n",
            spec->name,first_seen?"already-set":"not-set",hit_preexisting?"already-set":"not-set",me.x,me.y,me.z);
        std::printf("[warper] boss-run boss=%s stages=%zu activation-event=%u entry-region=%u movement=grounded-walk-only hp-stages=%s\n",
            spec->name,plan.size(),script->battle,script->region,phases?"enabled-test-only":"disabled");
    }
    void Finish() {
        if(!spec || reported) return;
        Report(); reported=true;
        std::printf("[warper] boss-attempt boss=%s result=%s; NOT exhaustive coverage\n",spec->name,
            failed?"incomplete":entry_only?"activation-confirmed":unverified||spec->special?"observed-phase-coverage-unverified":"script-events-observed-not-exhaustive");
        attempted.insert(spec->name);
        if(state_path) if(auto* f=std::fopen(state_path,"a")) { std::fprintf(f,"%s\n",spec->name); std::fclose(f); }
        std::fflush(stdout);
    }
    Input Tick(double now) {
        if(!spec || done) return neutral;
        if(!std::isfinite(now)||now<0||now>900) { Abort("encounter-time-budget-or-invalid-clock"); return neutral; }
        Point me{};
        if(!Pos(player,&me) || !floor.Check(now,me)) { Abort("player-fall-or-outside-floor-envelope"); return neutral; }
        if(world && now>=next_scan) {
            next_scan=now+1; BbWorldWarperObserve::Census(world,player,lamp,"start-refresh",true);
        }
        bool active=false;
        if(!Flag(script->battle,&active)) return neutral;
        if(!activated) {
            if(now>90) { Abort("battle-event-not-triggered"); return neutral; }
            if(active) {
                activated=true; stage_began=activation_at=now; walker.Reset();
                std::printf("[warper] boss-activation boss=%s event=%u observed=yes\n",spec->name,script->battle);
            } else {
                if(gate) return EnterGate(now,me);
                Point goal=script->entry;
                if(script->interaction) {
                    const auto chair=Actor(script->interaction);
                    if(!chair || !Pos(chair,&goal)) {
                        if(now>15) Abort("initial-interaction-actor-unavailable");
                        return neutral;
                    }
                    if(Distance(me,goal)>1.5f) return Walk(now,me,goal,1.5f,true);
                    BbWorldWarperObserve::Face(player,chair);
                    return {128,128,128,128,std::fmod(now,2)<0.15?0x4000u:0u};
                }
                if(script->region) {
                    const auto input=Walk(now,me,goal,0.8f,true);
                    if(done) return neutral;
                    if(Distance(me,goal)>0.8f) return input;
                    return neutral; // wait for the actual trigger/cinematic
                }
                if(!SelectTarget() || !Pos(target,&goal)) {
                    if(now>15) Abort("activation-target-unavailable");
                    return neutral;
                }
                if(BbWorldWarperEntry::HitEvent(script->boss)) return WakeByHit(now,me,goal);
                const float distance=Distance(me,goal);
                const float radius=10.f;
                if(distance>radius) return Walk(now,me,goal,radius,true);
                if(script->boss==2300810) return neutral; // proximity activation
                return neutral;
            }
        }
        if(!active) { Abort("battle-event-lost"); return neutral; }
        if(entry_only) {
            if(now-activation_at>=3) done=true;
            return neutral;
        }
        if(!SelectTarget()) {
            if(done) return neutral;
            if(missing_since<0) missing_since=now;
            if(now-missing_since>15) Abort("expected-live-form-unavailable");
            return neutral;
        }
        missing_since=-1;
        if(stage>=plan.size()) { done=true; return neutral; }
        const auto& s=plan[stage];
        if(!stage_started) {
            stage_started=true; stage_began=now; observe_began=-1; walker.Reset();
            transition_confirmed=transition_preexisting=false;
            counts=BbWorldWarperCompile::Read(); animations.clear(); baseline.clear(); Sample(); baseline=animations; animations.clear();
            if(s.event_flag && !Flag(s.event_flag,&transition_preexisting)) return neutral;
            if(s.transition && !RequestHealth(s.hp_percent)) { Abort("health-owner-or-live-form-write-failed"); return neutral; }
            std::printf("[warper] boss-stage boss=%s stage=%s range=%.1f movement=walking hp-percent=%d event=%u event-at-start=%s\n",
                spec->name,s.name,s.range,s.hp_percent,s.event_flag,transition_preexisting?"already-set":"not-set");
            next_sample=now;
        }
        if(now>=next_sample) { next_sample=now+0.5; Sample(); }
        const double local=now-stage_began;
        if(s.transition) {
            // No attacks or locomotion while the game executes a form change.
            if(s.event_flag) {
                bool observed=false; if(!Flag(s.event_flag,&observed)) return neutral;
                if(observed && local>=2) {
                    transition_confirmed=!transition_preexisting;
                    unverified|=transition_preexisting;
                    std::printf("[warper] boss-phase-event boss=%s event=%u observed=%s\n",spec->name,s.event_flag,
                        transition_preexisting?"pre-existing":"newly-set");
                    Next(now);
                } else if(local>45) Abort("phase-event-not-observed-no-later-phase-credit");
            } else if(local>=s.seconds) { unverified=true; Next(now); }
            return neutral;
        }
        Point boss{};
        const bool positioned=Pos(target,&boss) && Distance(me,boss)<100 && std::fabs(me.y-boss.y)<6;
        const float distance=positioned?Distance(me,boss):0;
        if(observe_began<0) {
            if(positioned && std::fabs(distance-s.range)>2 && local<18 && !walker.stalled) {
                // Approach/retreat on the existing floor. No strafing circles,
                // gap teleports or following an airborne/off-map actor.
                const float n=std::max(distance,0.01f);
                const Point goal{boss.x+(me.x-boss.x)*s.range/n,me.y,boss.z+(me.z-boss.z)*s.range/n};
                return Walk(now,me,goal,1,false);
            }
            observe_began=now; walker.Reset();
            std::printf("[warper] boss-observe boss=%s stage=%s range=%.1f desired=%.1f range-reached=%s\n",
                spec->name,s.name,distance,s.range,positioned&&std::fabs(distance-s.range)<=3?"yes":"no");
            if(!positioned) { Abort("live-form-outside-grounded-arena"); return neutral; }
        }
        const double viewing=now-observe_began;
        if(viewing>=s.seconds) { Next(now); return neutral; }
        const bool hit=s.provoke && positioned && distance<7 && ((viewing>=6&&viewing<6.15)||(viewing>=10&&viewing<10.15));
        if(hit) BbWorldWarperObserve::Face(player,target);
        // A short look-around only after approach; Walker recalibrates next stage.
        return {128,128,viewing>s.seconds-3?200:128,128,hit?0x800u:0u};
    }
};
}
