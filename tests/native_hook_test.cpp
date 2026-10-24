#include "bbport_world_warper_image.h"
#include "bbport_world_warper_hook.h"
#include <windows.h>
#include <cassert>
#include <cstdint>
#include <cstring>

static unsigned char* image;
static int calls;
extern "C" void* runtime_low_map(std::size_t, int) { return image + 0x2100000; }
static void Callback() { ++calls; }

int main() {
    image = static_cast<unsigned char*>(VirtualAlloc(nullptr, 0x2200000,
        MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    assert(image);
    constexpr std::uint64_t offset = 0x2034770;
    // The exact game prologue followed by a synthetic return that undoes its stack frame.
    const unsigned char function[] = {
        0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x38,
        0x48,0x83,0xc4,0x38,0x5b,0x41,0x5c,0x41,0x5d,0x41,0x5e,0x41,0x5f,0x5d,0xc3};
    std::memcpy(image + offset, function, sizeof(function));
    assert(BbWorldWarperImage::Bind(image, 0x2100000));
    assert(!BbWorldWarperHook::InstallMainThreadTick(nullptr));
    image[offset] = 0x90;
    assert(!BbWorldWarperHook::InstallMainThreadTick(Callback));
    assert(image[offset] == 0x90);
    image[offset] = function[0];
    assert(BbWorldWarperHook::InstallMainThreadTick(Callback));
    assert(BbWorldWarperHook::InstallMainThreadTick(Callback));
    assert(BbWorldWarperHook::MainThreadTickInstalled());
    using Guest = void (__attribute__((sysv_abi)) *)();
    auto entry = reinterpret_cast<Guest>(image + offset);
    for (int index = 0; index < 100; ++index) entry();
    assert(calls == 100);
    VirtualFree(image, 0, MEM_RELEASE);
}
