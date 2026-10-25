#include "bbport_world_warper_encounter.h"
#include <cassert>
#include <map>
static std::map<std::uint64_t,unsigned char> memory;
static unsigned writes=0;
static float placed_x=0;
static unsigned placements=0;
static void __attribute__((sysv_abi)) FakeWarp(std::uint64_t,const float* pos,const float* rot,std::uint64_t) {
    placed_x=pos[0]; ++placements;
    assert(std::fabs(-std::sin(rot[1])+1)<0.0001f); // faces boss at x=0
}
namespace BbWorldWarperMemory {
bool Read(std::uint64_t address,void* output,std::size_t size) {
    auto* bytes=static_cast<unsigned char*>(output);
    for (std::size_t i=0;i<size;++i) {
        auto it=memory.find(address+i); if (it==memory.end()) return false; bytes[i]=it->second;
    }
    return true;
}
bool Write(std::uint64_t address,const void* input,std::size_t size) {
    ++writes; const auto* bytes=static_cast<const unsigned char*>(input);
    for (std::size_t i=0;i<size;++i) memory[address+i]=bytes[i];
    return true;
}
}
namespace BbWorldWarperImage {
std::uint64_t Address(std::uint64_t offset) { return reinterpret_cast<std::uint64_t>(&FakeWarp)-0x100+offset; }
std::uint64_t Size() { return 0x10000; }
}
template<class T> void Put(std::uint64_t address,T value) {
    const auto* bytes=reinterpret_cast<const unsigned char*>(&value);
    for (unsigned i=0;i<sizeof value;++i) memory[address+i]=bytes[i];
}
void Position(std::uint64_t chr,std::uint64_t base,float x) {
    Put<std::uint64_t>(chr+0x58,base);
    Put<std::uint64_t>(base+8,base+0x1000);
    Put<std::uint64_t>(base+0x1000+0x3b0,base+0x2000);
    Put<std::uint64_t>(base+0x2000+0x68,base+0x3000);
    Put<float>(base+0x3000+0x1e0,x);
    Put<float>(base+0x3000+0x1e4,0);
    Put<float>(base+0x3000+0x1e8,0);
}
int main() {
    constexpr std::uint64_t boss=0x1000,player=0x2000,damage=0x9000;
    Put<unsigned>(boss+0x1f0,2500800);
    Put<std::uint64_t>(boss+0x3b0,0x8000);
    Put<std::uint64_t>(0x8020,damage);
    Put<int>(damage+0xf8,800); // partly damaged: percentages must use max, not starting HP
    Put<int>(damage+0xfc,1000);
    Position(boss,0x10000,0); Position(player,0x20000,30);
    Put<std::uint64_t>(player,0x5000);
    Put<std::uint64_t>(0x55f8,reinterpret_cast<std::uint64_t>(&FakeWarp));
    BbWorldWarperObserve::actors={{boss,2500800}};
    BbWorldWarperEncounter::attempted.clear();
    BbWorldWarperEncounter::Run run;
    run.Begin(2502952,player,true);
    assert(run.Active() && run.Seconds()==105);
    run.Tick(0); assert(writes==0 && placements==1 && placed_x==14);
    run.Tick(20); assert(writes==0 && placements==2 && placed_x==3);
    auto input=run.Tick(40);
    assert(BbWorldWarperObserve::Read<int>(damage+0xf8)==650 && writes==1);
    const auto transition_placements=placements;
    input=run.Tick(50);
    assert(input.buttons==0 && input.rx==128 && placements==transition_placements);
    run.Tick(55); assert(writes==1 && placed_x==14);
    run.Tick(80); assert(writes==1 && placed_x==3);
    run.Tick(105); run.Finish();
    run.Begin(2502952,player,true); assert(!run.Active()); // no duplicate completed attempts
    BbWorldWarperEncounter::attempted.clear();
    Put<int>(damage+0xf8,1000);
    run.Begin(2502952,player,false); run.Tick(40);
    assert(BbWorldWarperObserve::Read<int>(damage+0xf8)==1000 && writes==1);
    Put<unsigned>(boss+0x1f0,123);
    run.Tick(55); assert(writes==1); // stale reference cannot write HP
}
