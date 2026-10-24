// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace BbWorldWarperRoute {
struct Stop { std::uint32_t lamp, map; const char* name; };
inline constexpr Stop stops[] = {
    {2412950, 0x18010000, "Central Yharnam"},
    {2402950, 0x18000000, "Cathedral Ward"},
    {2302950, 0x17000000, "Old Yharnam"},
};
inline constexpr unsigned count = sizeof(stops) / sizeof(stops[0]);
}
