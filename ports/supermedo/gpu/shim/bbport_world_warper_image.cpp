// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_image.h"

#include <atomic>
#include <cstddef>
#include <mutex>

namespace BbWorldWarperImage {
namespace {

std::atomic<unsigned char*> g_image{nullptr};
std::atomic<std::uint64_t> g_size{0};
std::mutex g_bind_mutex;

bool RangeFits(std::uint64_t offset, std::uint64_t length, std::uint64_t size) {
    return offset <= size && length <= size - offset;
}

} // namespace

bool Bind(unsigned char* image, std::uint64_t size) {
    if (!image || !size) return false;

    std::lock_guard<std::mutex> lock(g_bind_mutex);
    auto* current = g_image.load(std::memory_order_acquire);
    if (current) return current == image && g_size.load(std::memory_order_acquire) == size;

    // Publish the size before the base. Readers acquire the base first, so they cannot
    // observe a bound image with an unpublished size.
    g_size.store(size, std::memory_order_relaxed);
    g_image.store(image, std::memory_order_release);
    return true;
}

void Reset() {
    std::lock_guard<std::mutex> lock(g_bind_mutex);
    g_image.store(nullptr, std::memory_order_release);
    g_size.store(0, std::memory_order_relaxed);
}

bool IsBound() {
    return g_image.load(std::memory_order_acquire) != nullptr;
}

std::uint64_t Size() {
    return g_size.load(std::memory_order_acquire);
}

std::uint64_t Address(std::uint64_t offset) {
    auto* image = g_image.load(std::memory_order_acquire);
    const auto size = g_size.load(std::memory_order_acquire);
    if (!image || !RangeFits(offset, 1, size)) return 0;
    return reinterpret_cast<std::uint64_t>(image + offset);
}

bool Matches(std::uint64_t offset, std::initializer_list<std::uint8_t> expected) {
    auto* image = g_image.load(std::memory_order_acquire);
    const auto size = g_size.load(std::memory_order_acquire);
    if (!image || expected.size() == 0 || !RangeFits(offset, expected.size(), size)) return false;

    std::size_t index = 0;
    for (const auto byte : expected) {
        if (image[offset + index] != byte) return false;
        ++index;
    }
    return true;
}

} // namespace BbWorldWarperImage
