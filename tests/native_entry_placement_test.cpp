#include "bbport_world_warper_entry.h"
#include "bbport_world_warper_observe.h"
#include <cassert>
#include <map>
static std::map<std::uint64_t,unsigned char> memory;
static unsigned calls=0;
static const BbWorldWarperEntry::Gate* expected=nullptr;
static void __attribute__((sysv_abi)) Place(std::uint64_t,const float* p,const float* r,std::uint64_t) {
    ++calls; const auto start=BbWorldWarperEntry::GatePoint(*expected,-3);
    assert(std::fabs(p[0]-start.x)<0.001f && std::fabs(p[1]-start.y-0.05f)<0.001f && std::fabs(p[2]-start.z)<0.001f);
    assert(std::fabs(r[1]-expected->yaw_degrees*0.01745329251994329577f)<0.001f);
}
namespace BbWorldWarperMemory {
bool Read(std::uint64_t at,void* out,std::size_t size) {
    auto* p=static_cast<unsigned char*>(out);
    for(std::size_t i=0;i<size;++i) { auto it=memory.find(at+i); if(it==memory.end()) return false; p[i]=it->second; }
    return true;
}
bool Write(std::uint64_t,const void*,std::size_t) { assert(false); return false; }
}
namespace BbWorldWarperImage {
std::uint64_t Address(std::uint64_t offset) { return reinterpret_cast<std::uint64_t>(&Place)-0x100+offset; }
std::uint64_t Size() { return 0x10000; }
}
template<class T> void Put(std::uint64_t at,T value) {
    const auto* bytes=reinterpret_cast<const unsigned char*>(&value);
    for(unsigned i=0;i<sizeof value;++i) memory[at+i]=bytes[i];
}
int main() {
    constexpr std::uint64_t player=0x1000;
    Put<std::uint64_t>(player+0x58,0x10000); Put<std::uint64_t>(0x10008,0x20000);
    Put<std::uint64_t>(0x203b0,0x30000); Put<std::uint64_t>(0x30068,0x40000);
    Put<std::uint64_t>(player,0x50000); Put<std::uint64_t>(0x505f8,reinterpret_cast<std::uint64_t>(&Place));
    for(const auto& gate:BbWorldWarperEntry::gates) {
        expected=&gate;
        Put<float>(0x401e0,gate.door.x); Put<float>(0x401e4,gate.door.y); Put<float>(0x401e8,gate.door.z);
        const unsigned before=calls;
        assert(BbWorldWarperEntry::SetupEntryStart(player,gate.boss) && calls==before+1);
        Put<float>(0x401e4,gate.door.y+10);
        assert(!BbWorldWarperEntry::SetupEntryStart(player,gate.boss) && calls==before+1);
    }
    assert(!BbWorldWarperEntry::SetupEntryStart(player,3200800)); // Rom never uses entry placement
    assert(!BbWorldWarperEntry::SetupEntryStart(player,1234567));
    expected=&BbWorldWarperEntry::gates[0];
    Put<float>(0x401e0,expected->door.x+200); Put<float>(0x401e4,expected->door.y);
    assert(!BbWorldWarperEntry::SetupEntryStart(player,expected->boss));
}
