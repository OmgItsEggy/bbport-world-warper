// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_memory.h"
namespace BbWorldWarperEvents {
// Pinned 1.09 read-only binding, also used by the reference party game_state.cpp.
// Never set progression/entrance/phase flags to manufacture a successful fight.
inline bool NativeRead(unsigned flag,bool* value) {
    std::uint64_t man=0;
    if(!flag || !value || !BbWorldWarperMemory::Get(BbWorldWarperImage::Address(0x553b100),&man) || !man ||
       !BbWorldWarperImage::Matches(0x13cfd80,{0x55,0x41,0x57,0x41,0x56,0x53,0x41,0x89,0xd0,0x44,0x8b,0x4f,0x1c})) return false;
    using Fn=unsigned(__attribute__((sysv_abi)) *)(std::uint64_t,unsigned,unsigned);
    *value=reinterpret_cast<Fn>(BbWorldWarperImage::Address(0x13cfd80))(man,flag,1)!=0;
    return true;
}
using Reader=bool(*)(unsigned,bool*);
inline Reader reader=NativeRead; // injectable for deterministic native tests
inline bool Read(unsigned flag,bool* value) { return reader(flag,value); }
}
