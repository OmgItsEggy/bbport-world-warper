// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace BbWorldWarperGear {
inline constexpr unsigned rows = 34;
bool EquipRow(unsigned row);
bool HasWeaponActions(unsigned row);
bool RefillBullets();
}
