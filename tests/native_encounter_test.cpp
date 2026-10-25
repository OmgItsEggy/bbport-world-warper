#include "bbport_world_warper_encounter.h"
#include <cassert>
#include <map>
static std::map<std::uint64_t,unsigned char> memory;
static std::map<unsigned,bool> flags;
static std::set<std::uint64_t> written;
static unsigned entry_setups=0;
static bool entry_setup_success=false;
namespace BbWorldWarperMemory {
bool Read(std::uint64_t address,void* output,std::size_t size) {
    auto* bytes=static_cast<unsigned char*>(output);
    for(std::size_t i=0;i<size;++i) { auto it=memory.find(address+i); if(it==memory.end()) return false; bytes[i]=it->second; }
    return true;
}
bool Write(std::uint64_t address,const void* input,std::size_t size) {
    written.insert(address); const auto* bytes=static_cast<const unsigned char*>(input);
    for(std::size_t i=0;i<size;++i) memory[address+i]=bytes[i];
    return true;
}
}
namespace BbWorldWarperImage {
std::uint64_t Address(std::uint64_t offset) { return offset; }
std::uint64_t Size() { return 0; }
bool Matches(std::uint64_t,std::initializer_list<std::uint8_t>) { return false; }
}
template<class T> void Put(std::uint64_t at,T value) {
    const auto* bytes=reinterpret_cast<const unsigned char*>(&value);
    for(unsigned i=0;i<sizeof value;++i) memory[at+i]=bytes[i];
}
void Position(std::uint64_t chr,BbWorldWarperEncounter::Point p) {
    const auto base=chr+0x100000;
    Put<std::uint64_t>(chr+0x58,base); Put<std::uint64_t>(base+8,base+0x1000);
    Put<std::uint64_t>(base+0x1000+0x3b0,base+0x2000); Put<std::uint64_t>(base+0x2000+0x68,base+0x3000);
    Put<float>(base+0x31e0,p.x); Put<float>(base+0x31e4,p.y); Put<float>(base+0x31e8,p.z);
    for(int i=0;i<4;++i) Put<float>(base+0x31d0+i*4,0);
}
namespace BbWorldWarperEntry {
bool SetupEntryStart(std::uint64_t player,unsigned boss) {
    ++entry_setups;
    if(!entry_setup_success) return false;
    const auto* gate=GateFor(boss); if(!gate) return false;
    auto p=GatePoint(*gate,-3); p.y+=0.05f; ::Position(player,p); return true;
}
}
void Actor(std::uint64_t chr,unsigned entity,BbWorldWarperEncounter::Point p) {
    Put<unsigned>(chr+0x1f0,entity); Put<std::uint64_t>(chr+0x3b0,chr+0x4000);
    Put<std::uint64_t>(chr+0x4020,chr+0x5000); Put<int>(chr+0x50f8,1000); Put<int>(chr+0x50fc,1000);
    Position(chr,p); BbWorldWarperObserve::actors.push_back({chr,entity});
}
void Reset() {
    memory.clear(); flags.clear(); written.clear(); entry_setups=0; entry_setup_success=false; BbWorldWarperObserve::actors.clear();
    BbWorldWarperEncounter::attempted.clear();
    BbWorldWarperEvents::reader=[](unsigned id,bool* v) { *v=flags[id]; return true; };
}
int main() {
    using namespace BbWorldWarperEncounter;
    constexpr std::uint64_t player=0x2000,first=0x10000,second=0x30000;
    Reset(); const auto* cleric_gate=BbWorldWarperEntry::GateFor(2410800);
    Position(player,BbWorldWarperEntry::GatePoint(*cleric_gate,-3));
    Run run; run.Begin(2412952,player,true,0,2410800);
    assert(run.Active()&&!run.Done());
    // No actor needs to be enabled before we walk across its entrance trigger.
    const auto entry=run.Tick(0); assert(entry.ly!=128 && written.empty());
    run.Tick(6); assert(run.Failed()&&run.Done()&&written.empty()); // blocked walk, never teleport

    // Repeat encounters interact with the fog from OUTSIDE, not the initial
    // one-shot trigger deep inside the room. Cross is actually requested.
    Reset(); flags[cleric_gate->first_event]=true;
    Position(player,BbWorldWarperEntry::GatePoint(*cleric_gate,-1.6f));
    run.Begin(2412952,player,false,0,2410800,true);
    auto fog=run.Tick(0);
    assert(fog.buttons==0x4000 && entry_setups==0 && written.empty());
    flags[12414702]=true; run.Tick(1); run.Tick(4);
    assert(run.Done()&&!run.Failed()&&written.empty()); // activation-only: no HP/stage tour

    // Long travel cannot consume the intro animation's AFTER-arrival wait.
    Reset(); Position(player,BbWorldWarperEntry::GatePoint(*cleric_gate,-3));
    run.Begin(2412952,player,false,0,2410800,true); run.Tick(0);
    Position(player,BbWorldWarperEntry::GatePoint(*cleric_gate,-1.6f)); run.Tick(1);
    Position(player,BbWorldWarperEntry::GatePoint(*cleric_gate,5)); run.Tick(2);
    Position(player,ScriptFor(2410800)->entry); run.Tick(22);
    assert(!run.Done()&&!run.Failed());
    flags[12414702]=true; run.Tick(25); run.Tick(28);
    assert(run.Done()&&!run.Failed()&&written.empty());

    // A player behind the one-way gate gets only one allowlisted setup attempt.
    Reset(); Position(player,{-140,-27,50});
    run.Begin(2412952,player,false,0,2410800,true); run.Tick(0); run.Tick(1);
    assert(run.Done()&&run.Failed()&&entry_setups==1);

    // Rom: walking/real R1 input, not an HP edit or raw yaw write, precedes
    // an observed hit event. Only the actual battle event confirms activation.
    Reset(); Position(player,{0,-271,1.8f}); Actor(first,3200800,{0,-271,0});
    run.Begin(3202952,player,false,0,3200800,true);
    assert(run.Tick(0).buttons==0);
    auto swing=run.Tick(1.01); assert(swing.buttons==0x800 && written.empty());
    auto recovery=run.Tick(1.5); assert(recovery.buttons==0 && !run.Done());
    flags[13201802]=true; assert(run.Tick(2).buttons==0 && !run.Done());
    flags[13204802]=true; run.Tick(3); run.Tick(6);
    assert(run.Done()&&!run.Failed()&&written.empty());

    // Ebrietas: no R1 credit while facing away, and no fake activation after
    // six missed/blocked attempts. Initial entry must not use her fog setup.
    Reset(); Position(player,{0,4,1.8f}); Actor(first,2420800,{0,4,0});
    Put<float>(player+0x1031d4,3.14159265f);
    run.Begin(2422951,player,false,0,2420800,true); run.Tick(0);
    assert(run.Tick(1.01).buttons==0 && entry_setups==0 && written.empty());
    run.Tick(15); assert(run.Done()&&run.Failed()&&written.empty());

    // An inactive encounter must not receive any HP requests, even after 80s.
    Reset(); const auto* orphan=ScriptFor(3600800);
    Position(player,orphan->entry); Actor(first,3600800,{orphan->entry.x,orphan->entry.y,orphan->entry.z-12});
    Actor(second,3600801,{1000,-1000,1000}); // disabled form in a holding location
    run.Begin(3602952,player,true,0,3600800); run.Tick(0); run.Tick(80);
    assert(written.empty() && !run.Done()); run.Tick(91); assert(run.Failed());

    // Active Orphan: phase HP goes to BOTH current ratio and redirected owner.
    Reset(); Position(player,orphan->entry);
    Actor(first,3600800,{orphan->entry.x,orphan->entry.y,orphan->entry.z-12});
    Actor(second,3600801,{1000,-1000,1000}); flags[orphan->battle]=true;
    run.Begin(3602952,player,true,0,3600800);
    run.Tick(0); run.Tick(20); // ranged window
    Position(player,{orphan->entry.x,orphan->entry.y,orphan->entry.z-9});
    run.Tick(20.1); run.Tick(40.1); run.Tick(40.2); // close, then transition
    assert(BbWorldWarperObserve::Read<int>(first+0x50f8)==450);
    assert(BbWorldWarperObserve::Read<int>(second+0x50f8)==450);
    assert(written==std::set<std::uint64_t>({first+0x50f8,second+0x50f8}));
    auto input=run.Tick(60); assert(input.ly==128 && input.rx==128 && input.buttons==0 && !run.Done());
    // A timer/HP write is not transition evidence. Switch only when the script completes.
    flags[orphan->swap_flag]=true;
    Position(first,{1000,-1000,1000}); Position(second,{orphan->entry.x,orphan->entry.y,orphan->entry.z-12});
    run.Tick(61); assert(run.TargetEntity()==3600801 && !run.Failed());
    Position(player,{orphan->entry.x,orphan->entry.y-8,orphan->entry.z});
    run.Tick(61.1); assert(run.Done()&&run.Failed()); // fall: immediately stops all actions/writes

    // Missing transition times out rather than running imaginary later phases.
    Reset(); Position(player,orphan->entry);
    Actor(first,3600800,{orphan->entry.x,orphan->entry.y,orphan->entry.z-12});
    Actor(second,3600801,{1000,-1000,1000}); flags[orphan->battle]=true;
    run.Begin(3602952,player,true,0,3600800); run.Tick(0); run.Tick(20);
    Position(player,{orphan->entry.x,orphan->entry.y,orphan->entry.z-9});
    run.Tick(20.1); run.Tick(40.1); run.Tick(40.2); run.Tick(86);
    assert(run.Failed()&&run.Done()&&run.TargetEntity()==3600800);
    Reset(); Position(player,orphan->entry); flags[orphan->dead]=true;
    run.Begin(3602952,player,true,0,3600800); assert(run.Failed()&&written.empty());
    Reset(); Position(player,orphan->entry);
    BbWorldWarperEvents::reader=[](unsigned,bool*) { return false; };
    run.Begin(3602952,player,true,0,3600800); assert(run.Failed()&&written.empty());

    // Closed-loop simulations: actual emitted sticks drive the player against
    // boss-body collision; an R1 request only produces a hit at the body.
    for(unsigned id:{3200800u,2420800u}) for(float camera:{0.f,0.7f,3.14f,4.7f}) for(float hand:{-1.f,1.f}) {
        Reset(); Point p{20,0,20}; Position(player,p); Actor(first,id,{0,0,0});
        run.Begin(0,player,false,0,id,true);
        bool hit=false; double hit_time=-1;
        const float body=id==3200800?2.7f:5.5f;
        for(int frame=0;frame<3600 && !run.Done();++frame) {
            const double now=frame/60.;
            const float fx=std::sin(camera+now*0.1),fz=-std::cos(camera+now*0.1);
            auto input=run.Tick(now);
            const float side=(input.lx-128)/96.f,forward=(128-input.ly)/96.f;
            const float dx=(forward*fx-hand*side*fz)*3/60,dz=(forward*fz+hand*side*fx)*3/60;
            Point next{p.x+dx,0,p.z+dz};
            const float d=Distance(next,{0,0,0});
            if(d<body) { next.x=next.x/d*body; next.z=next.z/d*body; }
            p=next; Position(player,p);
            if(std::hypot(dx,dz)>0.001f) Put<float>(player+0x1031d4,std::atan2(-dx,-dz));
            if((input.buttons&0x800) && Distance(p,{0,0,0})<body+0.1f) {
                hit=true; if(hit_time<0) hit_time=now; flags[BbWorldWarperEntry::HitEvent(id)]=true;
            }
            if(hit_time>=0 && now-hit_time>0.3) flags[ScriptFor(id)->battle]=true;
        }
        assert(hit && run.Done() && !run.Failed() && written.empty() && entry_setups==0);
    }

    // Gate routes execute from behind the door, for both first and repeat
    // entry. Repeated entry cannot cross a solid fog without requesting Cross.
    for(unsigned id:{2410800u,2410810u}) for(bool seen:{false,true}) for(float camera:{0.f,2.f}) for(float hand:{-1.f,1.f}) {
        Reset(); entry_setup_success=true;
        const auto* g=BbWorldWarperEntry::GateFor(id); const auto* s=ScriptFor(id);
        Position(player,BbWorldWarperEntry::GatePoint(*g,12)); flags[g->first_event]=seen;
        run.Begin(0,player,false,0,id,true); bool crossed=false;
        const float fx=std::sin(camera),fz=-std::cos(camera);
        for(int frame=0;frame<3600 && !run.Done();++frame) {
            auto input=run.Tick(frame/60.);
            float raw[3]{}; assert(BbWorldWarperObserve::Position(player,raw));
            Point p{raw[0],raw[1],raw[2]};
            const float side=(input.lx-128)/96.f,forward=(128-input.ly)/96.f;
            Point next{p.x+(forward*fx-hand*side*fz)*3/60,p.y,p.z+(forward*fz+hand*side*fx)*3/60};
            if(input.buttons&0x4000) crossed=true;
            if(seen && !crossed && BbWorldWarperEntry::GateSide(*g,next)>0) next=p;
            Position(player,next);
            if((seen && crossed && BbWorldWarperEntry::GateSide(*g,next)>0.75f) ||
               (!seen && Distance(next,s->entry)<1)) flags[s->battle]=true;
        }
        assert(run.Done()&&!run.Failed()&&entry_setups==1&&written.empty());
        assert(crossed==seen);
    }
}
