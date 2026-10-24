// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_memory.h"

extern "C" {
#include "../../../src/runtime.h"
}

#include <cstring>

namespace BbWorldWarperMemory {

bool Plausible(std::uint64_t address, std::size_t size) {
    constexpr std::uint64_t kLowestUserAddress = 0x10000;
    constexpr std::uint64_t kHighestUserAddress = 1ull << 47;
    return address >= kLowestUserAddress && address + size >= address && address + size < kHighestUserAddress;
}

__attribute__((noinline)) bool Read(std::uint64_t address, void* output, std::size_t size) {
    if (!output || !Plausible(address, size)) return false;

    sigjmp_buf* const previous = runtime_fault_recover;
    sigjmp_buf recovery;
    if (sigsetjmp(recovery, 1)) {
        runtime_fault_recover = previous;
        return false;
    }
    runtime_fault_recover = &recovery;
    std::memcpy(output, reinterpret_cast<const void*>(address), size);
    runtime_fault_recover = previous;
    return true;
}

__attribute__((noinline)) bool Write(std::uint64_t address, const void* input, std::size_t size) {
    if (!input || !Plausible(address, size)) return false;

    sigjmp_buf* const previous = runtime_fault_recover;
    sigjmp_buf recovery;
    if (sigsetjmp(recovery, 1)) {
        runtime_fault_recover = previous;
        return false;
    }
    runtime_fault_recover = &recovery;
    auto* destination = reinterpret_cast<volatile unsigned char*>(address);
    const auto* source = static_cast<const unsigned char*>(input);
    for (std::size_t index = 0; index < size; ++index) destination[index] = source[index];
    runtime_fault_recover = previous;
    return true;
}

} // namespace BbWorldWarperMemory

