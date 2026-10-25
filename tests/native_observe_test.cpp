#include "bbport_world_warper_observe.h"
#include <cassert>
#include <cstring>
#include <map>
static std::map<std::uint64_t, unsigned char> memory;
static unsigned reads;
namespace BbWorldWarperMemory {
bool Read(std::uint64_t address, void* output, std::size_t size) {
    ++reads;
    auto* bytes = static_cast<unsigned char*>(output);
    for (std::size_t i=0; i<size; ++i) {
        auto it=memory.find(address+i);
        if (it==memory.end()) return false;
        bytes[i]=it->second;
    }
    return true;
}
}
namespace BbWorldWarperImage {
std::uint64_t Address(std::uint64_t offset) { return offset; }
std::uint64_t Size() { return 0; }
}
template<class T> void Put(std::uint64_t address, T value) {
    const auto* bytes=reinterpret_cast<unsigned char*>(&value);
    for (unsigned i=0; i<sizeof value; ++i) memory[address+i]=bytes[i];
}
int main() {
    using namespace BbWorldWarperObserve;
    // The forward vector must point toward the target from every quadrant.
    for (float x : {-4.f,0.f,4.f}) for (float z : {-4.f,0.f,4.f}) {
        if (x==0 && z==0) continue;
        const float yaw=FaceTarget(x,z,0,0);
        const float distance=std::sqrt(x*x+z*z);
        assert(std::fabs(-std::sin(yaw)+x/distance)<0.0001f);
        assert(std::fabs(-std::cos(yaw)+z/distance)<0.0001f);
    }
    assert(!CutscenePlaying());
    assert(!ApproachEbrietas(0));
    Put<std::uint64_t>(0x5540058,0x1000);
    Put<std::uint64_t>(0x1008,0x2000);
    Put<unsigned char>(0x2168,1);
    assert(CutscenePlaying());
    Put<unsigned char>(0x2168,2); // skippable alone does not mean playing
    assert(!CutscenePlaying());
    Put<std::uint64_t>(0x3008,0x4000);
    Put<int>(0x4018,1000000);
    reads=0;
    Census(0x3000,0,1,"test");
    assert(reads==2); // invalid counts never walk arbitrary memory
    Put<int>(0x4018,126);
    reads=0;
    Census(0x3000,0,1,"test");
    assert(reads==66); // at most 64 sets
}
