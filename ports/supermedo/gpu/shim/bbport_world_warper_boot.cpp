// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_hook.h"
#include "bbport_world_warper_memory.h"
#include "bbport_world_warper_movement.h"
#include "bbport_world_warper_route.h"
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
Clock::time_point warp_time;
bool warp_requested = false;
bool saw_loading = false;
std::uint32_t target_lamp = 0;
unsigned route_index = 0;
bool tour = false;
const char* state_path = nullptr;

void RecordStop(std::uint32_t lamp) {
    if (!state_path) return;
    if (auto* file = std::fopen(state_path, "a")) {
        std::fprintf(file, "done %u\n", lamp);
        std::fclose(file);
    }
}

template <typename T> T Read(std::uint64_t address) {
    T value{};
    BbWorldWarperMemory::Get(address, &value);
    return value;
}

void Tick() {
    if (finished) return;
    const auto now = Clock::now();
    if (target_lamp && warp_requested && std::chrono::duration<double>(now - warp_time).count() > 120) {
        finished = true;
        runtime_pad_director_sticks(-1,-1,-1,-1);
        std::printf("[warper] arrival timed out for lamp %u; stopped\n", target_lamp);
        std::fflush(stdout);
        return;
    }
    const auto world = Read<std::uint64_t>(BbWorldWarperImage::Address(0x553e878));
    const auto player = world ? Read<std::uint64_t>(world + 0x60) : 0;
    const auto helper = Read<std::uint64_t>(BbWorldWarperImage::Address(0x553e8b8));
    const bool in_frame = helper && (Read<unsigned char>(helper + 0x50) || Read<unsigned char>(helper + 0x51));
    const auto loading = Read<unsigned char>(BbWorldWarperImage::Address(0x556286b));
    if (warp_requested && (loading || !player)) saw_loading = true;
    if (!player || !in_frame || loading) {
        runtime_pad_director_sticks(-1,-1,-1,-1);
        previous = now;
        return;
    }
    if (target_lamp && !warp_requested) {
        if (!started) { started = true; previous = now; return; }
        if (std::chrono::duration<double>(now - previous).count() < 5) return;
        constexpr std::uint64_t lamp_warp = 0x13cdf30;
        if (!BbWorldWarperImage::Matches(lamp_warp,
                {0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x48,0x83,0xec,0x38})) {
            finished = true;
            std::printf("[warper] lamp warp signature mismatch; stopped\n");
            std::fflush(stdout);
            return;
        }
        warp_requested = true;
        warp_time = now;
        using Warp = void (__attribute__((sysv_abi)) *)(std::uint32_t);
        std::printf("[warper] requesting lamp %u from disposable character\n", target_lamp);
        std::fflush(stdout);
        reinterpret_cast<Warp>(BbWorldWarperImage::Address(lamp_warp))(target_lamp);
        started = false;
        previous = now;
        return;
    }
    if (target_lamp && warp_requested) {
        if (!saw_loading) {
            if (std::chrono::duration<double>(now - warp_time).count() > 30) {
                finished = true;
                std::printf("[warper] no loading transition after warp request; stopped\n");
                std::fflush(stdout);
            }
            return;
        }
        const auto history = Read<std::uint64_t>(player + 0x400);
        const auto map = history ? Read<std::uint32_t>(history + 0x48) : 0;
        const auto expected = tour ? BbWorldWarperRoute::stops[route_index].map : 0x18010000u;
        if ((map & 0xffff0000u) != expected) return;
        target_lamp = 0;
        std::printf("[warper] arrived: %s (map %08x)\n",
                    tour ? BbWorldWarperRoute::stops[route_index].name : "Central Yharnam", map);
        std::fflush(stdout);
    }
    if (!started) {
        started = true;
        previous = now;
        std::printf("[warper] movement pass started: 18 seconds of short loops, then camera sweep\n");
        std::fflush(stdout);
    }
    elapsed += std::chrono::duration<double>(now - previous).count();
    previous = now;
    if (elapsed >= 60) {
        runtime_pad_director_sticks(-1,-1,-1,-1);
        if (tour) {
            RecordStop(BbWorldWarperRoute::stops[route_index].lamp);
            std::printf("[warper] area %u/%u complete: %s\n", route_index + 1,
                        BbWorldWarperRoute::count, BbWorldWarperRoute::stops[route_index].name);
            ++route_index;
            if (route_index < BbWorldWarperRoute::count) {
                target_lamp = BbWorldWarperRoute::stops[route_index].lamp;
                started = false;
                warp_requested = false;
                saw_loading = false;
                elapsed = 0;
                previous = now;
                std::fflush(stdout);
                return;
            }
            std::printf("[warper] three-area tour complete\n");
        }
        finished = true;
        std::printf("[warper] movement and sweep complete after %.1f active seconds; quit normally\n", elapsed);
        std::fflush(stdout);
        return;
    }
    const auto input = BbWorldWarperMovement::At(elapsed);
    runtime_pad_director_sticks(input.lx, input.ly, input.rx, input.ry);
}
}

extern "C" void bbgpu_world_warper_set_image(unsigned char* image, std::uint64_t size) {
    if (!BbWorldWarperImage::Bind(image, size)) return;
    const char* mode = std::getenv("BB_WORLD_WARPER");
    if (!mode || std::strcmp(mode, "smoke") != 0) return;
    const char* lamp = std::getenv("BB_WORLD_WARPER_LAMP");
    // The prototype supports one verified route target only.
    if (lamp && std::strcmp(lamp, "2412950") == 0) target_lamp = 2412950;
    const char* route = std::getenv("BB_WORLD_WARPER_ROUTE");
    tour = route && std::strcmp(route, "three-area") == 0;
    if (tour) {
        state_path = std::getenv("BB_WORLD_WARPER_STATE");
        if (state_path) {
            if (auto* file = std::fopen(state_path, "r")) {
                unsigned lamp_done = 0;
                while (std::fscanf(file, "done %u\n", &lamp_done) == 1) {
                    if (route_index < BbWorldWarperRoute::count &&
                        lamp_done == BbWorldWarperRoute::stops[route_index].lamp) ++route_index;
                }
                std::fclose(file);
            }
        }
        if (route_index == BbWorldWarperRoute::count) {
            route_index = 0;
            if (state_path) { if (auto* file = std::fopen(state_path, "w")) std::fclose(file); }
        }
        target_lamp = BbWorldWarperRoute::stops[route_index].lamp;
        std::printf("[warper] three-area tour starting at stop %u\n", route_index + 1);
    }
    if (!BbWorldWarperHook::InstallMainThreadTick(Tick)) {
        std::printf("[warper] smoke mode could not install the main-thread hook\n");
    }
}
