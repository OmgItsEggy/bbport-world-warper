#pragma once
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_memory.h"
#include "bbport_world_warper_boss_catalog.h"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <set>
#include <vector>

// Read-only, bounded census adapted from the pinned reference party_balance.cpp.
// Loaded entities are NOT evidence that their materials or attack effects were drawn.
namespace BbWorldWarperObserve {
inline std::uint64_t ebrietas = 0;
inline std::uint64_t logarius = 0;
struct Actor { std::uint64_t chr; unsigned entity; };
inline std::vector<Actor> actors;
// Bloodborne character forward is (-sin(yaw), -cos(yaw)).
inline float FaceTarget(float from_x, float from_z, float target_x, float target_z) {
    return std::atan2(from_x-target_x, from_z-target_z);
}
template<class T> T Read(std::uint64_t address) {
    T value{}; BbWorldWarperMemory::Get(address, &value); return value;
}
inline bool CutscenePlaying() {
    const auto remo = Read<std::uint64_t>(BbWorldWarperImage::Address(0x5540058));
    const auto player = remo ? Read<std::uint64_t>(remo + 8) : 0;
    return player && (Read<unsigned char>(player + 0x168) & 1);
}
inline bool Position(std::uint64_t chr, float (&pos)[3]) {
    auto ptr=chr;
    for (auto offset : {0x58,8,0x3b0,0x68}) {
        ptr=ptr ? Read<std::uint64_t>(ptr+offset) : 0;
    }
    return ptr && BbWorldWarperMemory::Read(ptr+0x1e0,pos,sizeof pos) &&
           std::isfinite(pos[0]) && std::isfinite(pos[1]) && std::isfinite(pos[2]);
}
// Rotation only: NEVER invoke Chr::Warp or derive a player height from a boss.
inline bool Facing(std::uint64_t player,std::uint64_t target,float* dot) {
    float me[3]{},p[3]{};
    if(!Position(player,me)||!Position(target,p)) return false;
    auto ptr=player;
    for(auto offset:{0x58,8,0x3b0,0x68}) ptr=ptr?Read<std::uint64_t>(ptr+offset):0;
    float rot[4]{};
    const float distance=std::hypot(p[0]-me[0],p[2]-me[2]);
    if(!ptr || distance<0.01f || !BbWorldWarperMemory::Read(ptr+0x1d0,rot,sizeof rot) || !std::isfinite(rot[1])) return false;
    *dot=(-std::sin(rot[1])*(p[0]-me[0])-std::cos(rot[1])*(p[2]-me[2]))/distance;
    return true;
}
inline bool Face(std::uint64_t player,std::uint64_t target) {
    float me[3]{},p[3]{};
    if(!Position(player,me)||!Position(target,p)) return false;
    auto ptr=player;
    for(auto offset:{0x58,8,0x3b0,0x68}) ptr=ptr?Read<std::uint64_t>(ptr+offset):0;
    const float rot[4]={0,FaceTarget(me[0],me[2],p[0],p[2]),0,0};
    return ptr && BbWorldWarperMemory::Write(ptr+0x1d0,rot,sizeof rot);
}
inline bool Census(std::uint64_t world, std::uint64_t player, unsigned lamp, const char* phase,bool quiet=false) {
    using u64 = std::uint64_t;
    const auto manager = Read<u64>(world + 8);
    const int raw_sets = manager ? Read<int>(manager + 0x18) : -2;
    if (raw_sets < -1 || raw_sets > 126) {
        if(!quiet) std::printf("[warper] evidence lamp=%u phase=%s enemy census unavailable\n", lamp, phase);
        return false;
    }
    std::set<u64> seen;
    unsigned living = 0, dead = 0;
    if (phase[0]=='s') { ebrietas=0; logarius=0; actors.clear(); }
    static std::set<unsigned> observed;
    bool watch=false;
    float origin[3]{};
    const bool have_origin=Position(player,origin);
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
            const auto entity=Read<unsigned>(chr+0x1f0);
            bool known=entity==3500905; // Maria's initial chair interaction
            for(const auto& boss:BbWorldWarperEncounter::bosses) known|=BbWorldWarperEncounter::Matches(boss,entity);
            // Shared health owners and disabled form actors need not have enemy team 23.
            if (!known && (!npc || Read<unsigned char>(npc + 0x12f) != 23)) continue;
            const auto modules = Read<u64>(chr + 0x3b0);
            const auto damage = modules ? Read<u64>(modules + 0x20) : 0;
            if (!damage && entity!=3500905) continue;
            const int hp = damage?Read<int>(damage + 0xf8):0;
            if (hp < 0) continue;
            if (hp > 0) ++living; else ++dead;
            if (phase[0]=='s' && (hp>0 || entity==3500905) && entity && entity!=~0u) actors.push_back({chr,entity});
            if (entity==2420800 && hp>0) ebrietas=chr;
            if (entity==2500800 && hp>0) logarius=chr;
            float pos[3]{};
            if (hp>0 && entity && have_origin && Position(chr,pos)) {
                const float dx=pos[0]-origin[0], dy=pos[1]-origin[1], dz=pos[2]-origin[2];
                if (dx*dx+dy*dy+dz*dz <= 30*30 && phase[0]=='s' && observed.insert(entity).second)
                    watch=true;
            }
            if(!quiet) std::printf("[warper] entity lamp=%u phase=%s entity=%u handle=%u hp=%d loaded-only\n",
                lamp, phase, Read<unsigned>(chr+0x1f0), Read<unsigned>(chr+0x3b8), hp);
        }
    }
    if(!quiet) std::printf("[warper] evidence lamp=%u phase=%s living=%u dead=%u; visibility/actions unverified\n",
                lamp, phase, living, dead);
    if(!quiet) std::fflush(stdout);
    return watch;
}
}
