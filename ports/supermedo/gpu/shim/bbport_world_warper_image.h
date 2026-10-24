// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <initializer_list>

namespace BbWorldWarperImage {

// Bind once from bbgpu_patch_image, before any game-specific hook is installed.
bool Bind(unsigned char* image, std::uint64_t size);
void Reset();
bool IsBound();
std::uint64_t Size();
std::uint64_t Address(std::uint64_t offset);

// Every patch site must be checked against Bloodborne 1.09 bytes before writing.
bool Matches(std::uint64_t offset, std::initializer_list<std::uint8_t> expected);

} // namespace BbWorldWarperImage
