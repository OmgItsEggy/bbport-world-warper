#pragma once
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_memory.h"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <set>

// Read-only, bounded census adapted from the pinned reference party_balance.cpp.
// Loaded entities are NOT evidence that their materials or attack effects were drawn.
namespace BbWorldWarperObserve {
inline std::uint64_t ebrietas = 0;
inline std::uint64_t logarius = 0;
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
inline bool ApproachBoss(std::uint64_t player, std::uint64_t target, unsigned entity,
                         const char* name, float range, bool refresh=false) {
    float boss[3]{}, me[3]{};
    if (!target || Read<unsigned>(target+0x1f0)!=entity ||
        !Position(target,boss) || !Position(player,me)) return false;
    float dx=me[0]-boss[0], dz=me[2]-boss[2];
    const float distance=std::sqrt(dx*dx+dz*dz);
    // Refuse a cross-floor or long-range placement from another part of the map.
    if (!std::isfinite(distance) || distance>100 || std::fabs(me[1]-boss[1])>8) return false;
    // During the fight only close a large gap on approximately the same floor.
    // Do not follow a leap into the air or repeatedly reposition a nearby player.
    if (refresh && (distance<10 || std::fabs(me[1]-boss[1])>1.5f)) return false;
    if (distance<0.01f) { dx=0; dz=1; } else { dx/=distance; dz/=distance; }
    alignas(16) float pos[4]={boss[0]+dx*range, boss[1]+0.2f, boss[2]+dz*range,1};
    alignas(16) float rot[4]={0,FaceTarget(pos[0],pos[2],boss[0],boss[2]),0,0};
    const auto table=Read<std::uint64_t>(player);
    const auto fn=table ? Read<std::uint64_t>(table+0x5f8) : 0;
    const auto base=BbWorldWarperImage::Address(0);
    if (!base || fn<base || fn-base>=BbWorldWarperImage::Size()) return false;
    using Warp=void (__attribute__((sysv_abi)) *)(std::uint64_t,const float*,const float*,std::uint64_t);
    reinterpret_cast<Warp>(fn)(player,pos,rot,0);
    std::printf("[warper] %s proximity placement requested: %.1fm -> %.1fm; activation NOT yet verified\n",name,distance,range);
    return true;
}
inline bool ApproachEbrietas(std::uint64_t player) {
    return ApproachBoss(player,ebrietas,2420800,"Ebrietas",4);
}
inline bool ApproachLogarius(std::uint64_t player, bool refresh=false) {
    return ApproachBoss(player,logarius,2500800,"Logarius",3,refresh);
}
inline bool Census(std::uint64_t world, std::uint64_t player, unsigned lamp, const char* phase) {
    using u64 = std::uint64_t;
    const auto manager = Read<u64>(world + 8);
    const int raw_sets = manager ? Read<int>(manager + 0x18) : -2;
    if (raw_sets < -1 || raw_sets > 126) {
        std::printf("[warper] evidence lamp=%u phase=%s enemy census unavailable\n", lamp, phase);
        return false;
    }
    std::set<u64> seen;
    unsigned living = 0, dead = 0;
    if (phase[0]=='s') { ebrietas=0; logarius=0; }
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
            if (!npc || Read<unsigned char>(npc + 0x12f) != 23) continue;
            const auto modules = Read<u64>(chr + 0x3b0);
            const auto damage = modules ? Read<u64>(modules + 0x20) : 0;
            if (!damage) continue;
            const int hp = Read<int>(damage + 0xf8);
            if (hp < 0) continue;
            if (hp > 0) ++living; else ++dead;
            const auto entity=Read<unsigned>(chr+0x1f0);
            if (entity==2420800 && hp>0) ebrietas=chr;
            if (entity==2500800 && hp>0) logarius=chr;
            float pos[3]{};
            if (hp>0 && entity && have_origin && Position(chr,pos)) {
                const float dx=pos[0]-origin[0], dy=pos[1]-origin[1], dz=pos[2]-origin[2];
                if (dx*dx+dy*dy+dz*dz <= 30*30 && phase[0]=='s' && observed.insert(entity).second)
                    watch=true;
            }
            std::printf("[warper] entity lamp=%u phase=%s entity=%u handle=%u hp=%d loaded-only\n",
                lamp, phase, Read<unsigned>(chr+0x1f0), Read<unsigned>(chr+0x3b8), hp);
        }
    }
    std::printf("[warper] evidence lamp=%u phase=%s living=%u dead=%u; visibility/actions unverified\n",
                lamp, phase, living, dead);
    std::fflush(stdout);
    return watch;
}
}
