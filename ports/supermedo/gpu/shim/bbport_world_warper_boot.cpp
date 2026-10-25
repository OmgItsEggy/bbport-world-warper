// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_hook.h"
#include "bbport_world_warper_memory.h"
#include "bbport_world_warper_movement.h"
#include "bbport_world_warper_route.h"
#include "bbport_world_warper_replay.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <SDL3/SDL.h>

extern "C" void runtime_pad_director_sticks(int, int, int, int);
extern "C" void runtime_pad_director_press(std::uint32_t);

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
Clock::time_point boot_time;
Clock::time_point finish_time;
double last_confirm = -10;
bool automatic = false;
bool entered_world = false;
bool exit_sent = false;
bool record_exit = false;
BbWorldWarperReplay::Route cathedral_route;
Clock::time_point settle_until;

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
    const auto now = Clock::now();
    if (finished) {
        runtime_pad_director_press(0);
        if (automatic && !exit_sent && finish_time != Clock::time_point{} &&
            std::chrono::duration<double>(now - finish_time).count() >= 10) {
            exit_sent = true;
            std::printf("[warper] automatic exit after cache settling period\n");
            std::fflush(stdout);
            SDL_Event quit{};
            quit.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit);
        }
        return;
    }
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
    if (player && in_frame && !loading) entered_world = true;
    if (automatic && !entered_world) {
        const double seconds = std::chrono::duration<double>(now - boot_time).count();
        runtime_pad_director_press(0);
        if (seconds > 180) {
            std::printf("[warper] automatic Continue timed out; close and inspect the title screen\n");
            std::fflush(stdout);
            finished = true;
            return;
        }
        if (!loading && seconds > 8) {
            if (seconds - last_confirm >= 3) {
                last_confirm = seconds;
                std::printf("[warper] confirming title/Continue\n");
                std::fflush(stdout);
            }
            if (seconds - last_confirm < 0.15) runtime_pad_director_press(0x4000);
        }
    } else {
        runtime_pad_director_press(0);
    }
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
        settle_until = now + std::chrono::seconds(3);
        std::printf("[warper] arrived: %s (map %08x)\n",
                    tour ? BbWorldWarperRoute::stops[route_index].name : "Central Yharnam", map);
        std::fflush(stdout);
    }
    if (record_exit && !target_lamp) {
        runtime_pad_director_sticks(-1,-1,-1,-1);
        runtime_pad_director_press(0);
        return;
    }
    if (now < settle_until) {
        runtime_pad_director_sticks(128,128,128,128);
        return;
    }
    if (!started) {
        started = true;
        previous = now;
        std::printf("[warper] coverage pass: movement 18s, combat inputs 22s, camera sweep 20s\n");
        std::fflush(stdout);
    }
    elapsed += std::chrono::duration<double>(now - previous).count();
    previous = now;
    const bool replaying = tour && route_index == 1 && cathedral_route.Seconds() > 0;
    const double stop_seconds = replaying ? cathedral_route.Seconds() + 20 : 60;
    if (elapsed >= stop_seconds) {
        runtime_pad_director_press(0);
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
        finish_time = now;
        std::printf("[warper] movement and sweep complete after %.1f active seconds; quit normally\n", elapsed);
        std::fflush(stdout);
        return;
    }
    if (replaying && elapsed < cathedral_route.Seconds()) {
        const auto sample = cathedral_route.At(elapsed);
        runtime_pad_director_sticks(sample.lx,sample.ly,sample.rx,sample.ry);
        runtime_pad_director_press(sample.buttons | (sample.l2 > 127 ? 0x100 : 0) | (sample.r2 > 127 ? 0x200 : 0));
        return;
    }
    runtime_pad_director_press(0);
    if (replaying) { runtime_pad_director_sticks(128,128,255,128); return; }
    const auto input = BbWorldWarperMovement::At(elapsed);
    runtime_pad_director_sticks(input.lx, input.ly, input.rx, input.ry);
    runtime_pad_director_press(input.buttons);
}
}

extern "C" void bbgpu_world_warper_set_image(unsigned char* image, std::uint64_t size) {
    if (!BbWorldWarperImage::Bind(image, size)) return;
    const char* mode = std::getenv("BB_WORLD_WARPER");
    if (!mode || std::strcmp(mode, "smoke") != 0) return;
    // The pinned 1.09 Player No Dead patch from upstream Bloodborne.xml:
    // XML guest VA 0x0593e888 minus the 0x400000 patch base.
    const std::uint32_t no_death = 1;
    if (!BbWorldWarperMemory::Write(BbWorldWarperImage::Address(0x553e888), &no_death, sizeof no_death)) {
        std::printf("[warper] cannot enable test-character death protection; not starting\n");
        return;
    }
    std::printf("[warper] test-character Player No Dead enabled\n");
    boot_time = Clock::now();
    const char* auto_mode = std::getenv("BB_WORLD_WARPER_AUTO");
    automatic = auto_mode && std::strcmp(auto_mode, "1") == 0;
    const char* replay_path = std::getenv("BB_WORLD_WARPER_CATHEDRAL_ROUTE");
    if (replay_path) {
        if (cathedral_route.Load(replay_path))
            std::printf("[warper] Cathedral route loaded: %.1f seconds\n", cathedral_route.Seconds());
        else std::printf("[warper] Cathedral route invalid; using generic movement\n");
    }
    const char* recording = std::getenv("BB_WORLD_WARPER_RECORD");
    record_exit = recording && std::strcmp(recording, "cathedral-exit") == 0;
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
        if (record_exit) route_index = 1;
        target_lamp = BbWorldWarperRoute::stops[route_index].lamp;
        std::printf("[warper] three-area tour starting at stop %u\n", route_index + 1);
    }
    if (!BbWorldWarperHook::InstallMainThreadTick(Tick)) {
        std::printf("[warper] smoke mode could not install the main-thread hook\n");
    }
}
