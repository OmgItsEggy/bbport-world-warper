// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

namespace BbWorldWarperMemory {

bool Plausible(std::uint64_t address, std::size_t size);
bool Read(std::uint64_t address, void* output, std::size_t size);
bool Write(std::uint64_t address, const void* input, std::size_t size);

template <typename T>
bool Get(std::uint64_t address, T* value) {
    return Read(address, value, sizeof(T));
}

} // namespace BbWorldWarperMemory

