// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_entry.h"
#include "bbport_world_warper_observe.h"
namespace BbWorldWarperEntry {
bool SetupEntryStart(std::uint64_t player,unsigned boss) {
    const auto* gate=GateFor(boss);
    float raw[3]{};
    if(!gate || !BbWorldWarperObserve::Position(player,raw)) return false;
    const Point me{raw[0],raw[1],raw[2]},start=GatePoint(*gate,-3.f);
    // Refuse cross-map/cross-floor setup, or positioning from a stale player.
    if(BbWorldWarperEncounter::Distance(me,gate->door)>100 || std::fabs(me.y-start.y)>4) return false;
    const auto table=BbWorldWarperObserve::Read<std::uint64_t>(player);
    const auto fn=table?BbWorldWarperObserve::Read<std::uint64_t>(table+0x5f8):0;
    const auto base=BbWorldWarperImage::Address(0);
    if(!base || fn<base || fn-base>=BbWorldWarperImage::Size()) return false;
    alignas(16) const float pos[4]={start.x,start.y+0.05f,start.z,1};
    alignas(16) const float rot[4]={0,gate->yaw_degrees*0.01745329251994329577f,0,0};
    using Place=void(__attribute__((sysv_abi)) *)(std::uint64_t,const float*,const float*,std::uint64_t);
    reinterpret_cast<Place>(fn)(player,pos,rot,0);
    return true; // caller MUST check the settled position before any movement
}
}
