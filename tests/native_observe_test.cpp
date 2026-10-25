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
}
template<class T> void Put(std::uint64_t address, T value) {
    const auto* bytes=reinterpret_cast<unsigned char*>(&value);
    for (unsigned i=0; i<sizeof value; ++i) memory[address+i]=bytes[i];
}
int main() {
    using namespace BbWorldWarperObserve;
    assert(!CutscenePlaying());
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
