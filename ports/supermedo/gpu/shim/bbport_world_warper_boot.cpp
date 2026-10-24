// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_hook.h"
#include "bbport_world_warper_memory.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" void runtime_pad_director_sticks(int, int, int, int);

namespace {
using Clock = std::chrono::steady_clock;
bool finished = false;
bool started = false;
double elapsed = 0;
Clock::time_point previous;

template <typename T> T Read(std::uint64_t address) {
    T value{};
    BbWorldWarperMemory::Get(address, &value);
    return value;
}

void Tick() {
    if (finished) return;
    const auto now = Clock::now();
    const auto world = Read<std::uint64_t>(BbWorldWarperImage::Address(0x553e878));
    const auto player = world ? Read<std::uint64_t>(world + 0x60) : 0;
    const auto helper = Read<std::uint64_t>(BbWorldWarperImage::Address(0x553e8b8));
    const bool in_frame = helper && (Read<unsigned char>(helper + 0x50) || Read<unsigned char>(helper + 0x51));
    const auto loading = Read<unsigned char>(BbWorldWarperImage::Address(0x556286b));
    if (!player || !in_frame || loading) {
        runtime_pad_director_sticks(-1,-1,-1,-1);
        previous = now;
        return;
    }
    if (!started) {
        started = true;
        previous = now;
        std::printf("[warper] smoke sweep started: 60 seconds of active gameplay\n");
    }
    elapsed += std::chrono::duration<double>(now - previous).count();
    previous = now;
    if (elapsed >= 60) {
        runtime_pad_director_sticks(-1,-1,-1,-1);
        finished = true;
        std::printf("[warper] smoke sweep complete; quit normally to finish cache writes\n");
        std::fflush(stdout);
        return;
    }
    // Horizontal sweep only. Leave movement and vertical pitch to the game/player.
    runtime_pad_director_sticks(-1,-1,180,128);
}
}

extern "C" void bbgpu_world_warper_set_image(unsigned char* image, std::uint64_t size) {
    if (!BbWorldWarperImage::Bind(image, size)) return;
    const char* mode = std::getenv("BB_WORLD_WARPER");
    if (!mode || std::strcmp(mode, "smoke") != 0) return;
    if (!BbWorldWarperHook::InstallMainThreadTick(Tick)) {
        std::printf("[warper] smoke mode could not install the main-thread hook\n");
    }
}
