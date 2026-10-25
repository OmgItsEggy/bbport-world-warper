// SPDX-License-Identifier: GPL-3.0-or-later
// Solo adaptation of the pinned reference game_state.cpp equipment operations.
#include "bbport_world_warper_gear.h"
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_memory.h"
#include <cstdio>

namespace BbWorldWarperGear {
namespace {
using u64 = std::uint64_t;
using u32 = std::uint32_t;
#define WARPER_ARRAY(name, count) constexpr u32 name[count]
#include "bbport_world_warper_content.inc"
#undef WARPER_ARRAY
template<class T> T Read(u64 address) {
    T value{}; BbWorldWarperMemory::Get(address, &value); return value;
}
bool Signatures() {
    using BbWorldWarperImage::Matches;
    return Matches(0x14ccb90, {0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x28,0x89,0xcb}) &&
           Matches(0x14d9e80, {0x53,0x81,0xe6,0x00,0x00,0x00,0xf0,0x81,0xe2,0xff,0xff,0xff,0x0f}) &&
           Matches(0x131cb70, {0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec,0x68,0x89,0xcb,0x41,0x89,0xd6,0x41,0x89,0xf7});
}
u64 Record() {
    const auto manager = Read<u64>(BbWorldWarperImage::Address(0x553b130));
    return manager ? Read<u64>(manager + 8) : 0;
}
bool Give(u32 type, u32 id, int count) {
    if (!Record() || !Read<u64>(BbWorldWarperImage::Address(0x553d6e0))) return false;
    using Fn = void (__attribute__((sysv_abi)) *)(u64,u32,u32,int);
    reinterpret_cast<Fn>(BbWorldWarperImage::Address(0x131cb70))(0,type,id,count);
    return true;
}
bool Equip(unsigned slot, u32 type, u32 id) {
    const auto rec = Record();
    if (!rec) return false;
    const auto list = rec + 0x328;
    if (!Read<u64>(list+0x68) || !Read<u64>(list+0x78)) return false;
    using Find = int (__attribute__((sysv_abi)) *)(u64,u32,u32);
    const auto find = reinterpret_cast<Find>(BbWorldWarperImage::Address(0x14d9e80));
    int index = find(list,type,id);
    if (index < 0) {
        if (!Give(type,id,1)) return false;
        index = find(list,type,id);
    }
    if (index < 0 || index > Read<int>(list+0x88)) return false;
    const auto split = Read<u32>(list+0x24);
    const auto i = static_cast<u32>(index);
    const auto base = Read<u64>(list+(i < split ? 0x58 : 0x48));
    if (!base) return false;
    const auto entry = base + 16ull*(i < split ? i : i-split);
    auto handle = Read<u32>(entry);
    if (!handle) return false;
    using Set = void (__attribute__((sysv_abi)) *)(u64,u32,u32*,u32,u32);
    reinterpret_cast<Set>(BbWorldWarperImage::Address(0x14ccb90))(rec+0x1d0,slot,&handle,i,1);
    return Read<u32>(rec+0x264+slot*4ull) == handle;
}
}
bool HasWeaponActions(unsigned row) { return row < 26; }
bool EquipRow(unsigned row) {
    static const bool signatures_ok = Signatures();
    if (!signatures_ok || row >= rows) return false;
    bool ok = true;
    if (row < 26) ok = Equip(1,0,kWarperRightWeapons[row]) && ok;
    if (row < 15) ok = Equip(0,0,kWarperLeftWeapons[row]) && ok;
    const auto armor = kWarperArmorSets[row];
    for (unsigned part=0; part<4; ++part) ok = Equip(8+part,0x10000000,armor+part*1000) && ok;
    std::printf("[warper] gear row %u/%u right=%u left=%u armor=%u equip=%s\n",
                row+1,rows,row<26?kWarperRightWeapons[row]:0,row<15?kWarperLeftWeapons[row]:0,
                armor,ok?"verified":"FAILED (coverage not confirmed)");
    std::fflush(stdout);
    return ok;
}
bool RefillBullets() {
    static const bool signatures_ok = Signatures();
    return signatures_ok && Give(0x40000000,900,20);
}
}
