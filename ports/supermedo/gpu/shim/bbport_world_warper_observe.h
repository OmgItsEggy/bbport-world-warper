#pragma once
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_memory.h"
#include <algorithm>
#include <cstdio>
#include <set>

// Read-only, bounded census adapted from the pinned reference party_balance.cpp.
// Loaded entities are NOT evidence that their materials or attack effects were drawn.
namespace BbWorldWarperObserve {
template<class T> T Read(std::uint64_t address) {
    T value{}; BbWorldWarperMemory::Get(address, &value); return value;
}
inline bool CutscenePlaying() {
    const auto remo = Read<std::uint64_t>(BbWorldWarperImage::Address(0x5540058));
    const auto player = remo ? Read<std::uint64_t>(remo + 8) : 0;
    return player && (Read<unsigned char>(player + 0x168) & 1);
}
inline void Census(std::uint64_t world, std::uint64_t player, unsigned lamp, const char* phase) {
    using u64 = std::uint64_t;
    const auto manager = Read<u64>(world + 8);
    const int raw_sets = manager ? Read<int>(manager + 0x18) : -2;
    if (raw_sets < -1 || raw_sets > 126) {
        std::printf("[warper] evidence lamp=%u phase=%s enemy census unavailable\n", lamp, phase);
        return;
    }
    std::set<u64> seen;
    unsigned living = 0, dead = 0;
    for (int set = 0; set < std::min(raw_sets + 2, 64); ++set) {
        const auto tab = Read<u64>(world + 0x850 + 8ull*set);
        if (!tab) continue;
        const int count = Read<int>(tab);
        const auto entries = Read<u64>(tab + 8);
        if (!entries || count <= 0 || count > 4096) continue;
        for (int i=0; i<count; ++i) {
            const auto chr = Read<u64>(entries + 0x38ull*i);
            if (!chr || chr == player || !seen.insert(chr).second) continue;
            const auto npc_ref = Read<u64>(chr + 0x3c8);
            const auto npc = npc_ref ? Read<u64>(npc_ref + 8) : 0;
            if (!npc || Read<unsigned char>(npc + 0x12f) != 23) continue;
            const auto modules = Read<u64>(chr + 0x3b0);
            const auto damage = modules ? Read<u64>(modules + 0x20) : 0;
            if (!damage) continue;
            const int hp = Read<int>(damage + 0xf8);
            if (hp < 0) continue;
            if (hp > 0) ++living; else ++dead;
            std::printf("[warper] entity lamp=%u phase=%s entity=%u handle=%u hp=%d loaded-only\n",
                lamp, phase, Read<unsigned>(chr+0x1f0), Read<unsigned>(chr+0x3b8), hp);
        }
    }
    std::printf("[warper] evidence lamp=%u phase=%s living=%u dead=%u; visibility/actions unverified\n",
                lamp, phase, living, dead);
    std::fflush(stdout);
}
}
