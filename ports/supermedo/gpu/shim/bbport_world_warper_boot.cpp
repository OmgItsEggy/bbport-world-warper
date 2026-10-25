// SPDX-License-Identifier: GPL-3.0-or-later
#include "bbport_world_warper_image.h"
#include "bbport_world_warper_hook.h"
#include "bbport_world_warper_memory.h"
#include "bbport_world_warper_movement.h"
#include "bbport_world_warper_route.h"
#include "bbport_world_warper_replay.h"
#include "bbport_world_warper_gear.h"
#include "bbport_world_warper_observe.h"
#include "bbport_world_warper_compile.h"
#include "bbport_world_warper_encounter.h"
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
const BbWorldWarperRoute::Stop* route_stops = BbWorldWarperRoute::stops;
unsigned route_count = BbWorldWarperRoute::count;
const char* state_path = nullptr;
Clock::time_point boot_time;
Clock::time_point finish_time;
double last_confirm = -10;
bool automatic = false;
bool entered_world = false;
bool exit_sent = false;
bool record_exit = false;
bool combat_done = false;
BbWorldWarperReplay::Route cathedral_route;
Clock::time_point settle_until;
bool gear_enabled = false;
bool gear_started = false;
bool gear_equipped = false;
bool gear_ok = false;
unsigned gear_row = 0;
double gear_elapsed = 0;
unsigned gear_failures = 0;
const char* gear_state_path = nullptr;
bool cinematic_wait = false;
BbWorldWarperCompile::Snapshot compile_start{};
std::uint64_t compile_serial = 0;
Clock::time_point compile_quiet_since;
bool encounter_watch = false;
BbWorldWarperEncounter::Run boss_run;
bool boss_health_stages = false;
bool boss_runs_enabled = false;

bool GearTick(Clock::time_point now) {
    if (!gear_enabled || gear_row >= BbWorldWarperGear::rows) return false;
    runtime_pad_director_sticks(128,128,128,128);
    runtime_pad_director_press(0);
    if (!gear_started) {
        gear_started = true;
        previous = now;
        std::printf("[warper] combined gear pass starting before lamp tour\n");
    }
    if (!gear_equipped) {
        gear_ok = BbWorldWarperGear::EquipRow(gear_row);
        if (!gear_ok) ++gear_failures;
        if (gear_row < 15 && gear_ok) BbWorldWarperGear::RefillBullets();
        gear_equipped = true;
        gear_elapsed = 0;
        previous = now;
    }
    gear_elapsed += std::chrono::duration<double>(now - previous).count();
    previous = now;
    const bool actions = gear_ok && BbWorldWarperGear::HasWeaponActions(gear_row);
    const double duration = actions ? 25 : 4;
    // Allow the equipment model to load, then use the tested combat part of the routine.
    if (actions && gear_elapsed >= 3 && gear_elapsed < 25) {
        const auto input = BbWorldWarperMovement::At(18 + gear_elapsed - 3);
        runtime_pad_director_press(input.buttons);
    }
    if (gear_elapsed >= duration) {
        runtime_pad_director_press(0);
        ++gear_row;
        gear_equipped = false;
        if (gear_row == BbWorldWarperGear::rows) {
            if (gear_state_path) if (auto* file = std::fopen(gear_state_path, "w")) {
                std::fprintf(file, "attempted %u failures %u\n", gear_row, gear_failures);
                std::fclose(file);
            }
            combat_done = true;
            started = false;
            elapsed = 0;
            previous = now;
            std::printf("[warper] gear pass finished: %u rows failed equip verification; starting map coverage\n", gear_failures);
            std::fflush(stdout);
        }
    }
    return true;
}

void RecordStop(std::uint32_t lamp) {
    if (!state_path) return;
    if (auto* file = std::fopen(state_path, "a")) {
        std::fprintf(file, "done %u\n", lamp);
        std::fclose(file);
    }
}

void SkipStop(Clock::time_point now, const char* reason) {
    runtime_pad_director_sticks(-1,-1,-1,-1);
    runtime_pad_director_press(0);
    std::printf("[warper] skipped lamp %u: %s\n", target_lamp, reason);
    // Keep skip separate from done: a failed destination has no coverage claim.
    if (state_path) if (auto* file = std::fopen(state_path, "a")) {
        std::fprintf(file, "skip %u\n", target_lamp); std::fclose(file);
    }
    // Restart the process after a failed load; a half-loaded world cannot safely warp again.
    finished = true; finish_time = now;
    std::fflush(stdout);
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
    if(tour && boss_runs_enabled && route_index<route_count && !route_stops[route_index].lamp) {
        std::printf("[warper] boss-unavailable boss=%s reason=ending-arena-entry-not-implemented; no ordinary Dream warp substituted\n",route_stops[route_index].name);
        if(state_path) if(auto* file=std::fopen(state_path,"a")) { std::fprintf(file,"skip 0\n"); std::fclose(file); }
        ++route_index;
        started=false; warp_requested=false; saw_loading=false; elapsed=0;
        if(route_index<route_count) target_lamp=route_stops[route_index].lamp;
        else { finished=true; finish_time=now; }
        std::fflush(stdout);
        return;
    }
    if (target_lamp && warp_requested && std::chrono::duration<double>(now - warp_time).count() > 120) {
        SkipStop(now, "arrival timeout");
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
                SkipStop(now, "no loading transition");
            }
            return;
        }
        const auto history = Read<std::uint64_t>(player + 0x400);
        const auto map = history ? Read<std::uint32_t>(history + 0x48) : 0;
        const auto expected = tour ? route_stops[route_index].map : 0x18010000u;
        if ((map & 0xffff0000u) != expected) return;
        target_lamp = 0;
        settle_until = now + std::chrono::seconds(3);
        std::printf("[warper] arrived: %s (map %08x)\n",
                    tour ? route_stops[route_index].name : "Central Yharnam", map);
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
    const bool cinematic = BbWorldWarperObserve::CutscenePlaying();
    if (cinematic != cinematic_wait) {
        cinematic_wait = cinematic;
        std::printf("[warper] cutscene %s: coverage clock %s\n",
                    cinematic ? "started" : "ended", cinematic ? "paused" : "resuming");
        std::fflush(stdout);
    }
    if (cinematic) {
        runtime_pad_director_press(0);
        runtime_pad_director_sticks(128,128,128,128);
        previous = now;
        return;
    }
    if (GearTick(now)) return;
    if (!started) {
        started = true;
        previous = now;
        encounter_watch = BbWorldWarperObserve::Census(world, player, tour ? route_stops[route_index].lamp : 2412950, "start");
        if(boss_runs_enabled) boss_run.Begin(tour ? route_stops[route_index].lamp : 0,player,boss_health_stages,world,tour?route_stops[route_index].boss:0);
        compile_start = BbWorldWarperCompile::Read();
        compile_serial = compile_start.serial;
        compile_quiet_since = now;
        if (encounter_watch && !boss_run.Active()) std::printf("[warper] new living entities within 30m: adding 20s protected observation; attacks/phases unverified\n");
        std::printf("[warper] coverage pass: movement 18s, combat %s, camera sweep 20s\n",
                    combat_done ? "already covered; skipped" : "22s");
        std::fflush(stdout);
    }
    elapsed += std::chrono::duration<double>(now - previous).count();
    previous = now;
    const bool replaying = tour && route_stops[route_index].lamp == 2402950 && cathedral_route.Seconds() > 0;
    const bool targeted_stop=boss_runs_enabled && tour && route_stops[route_index].boss;
    const double stop_seconds = boss_run.Active() ? boss_run.Seconds() : targeted_stop ? 0 : (replaying ? cathedral_route.Seconds() + 20 : (combat_done ? 38 : 60));
    const auto counters = BbWorldWarperCompile::Read();
    if (counters.active || counters.serial != compile_serial) {
        compile_serial = counters.serial;
        compile_quiet_since = now;
    }
    const double coverage_seconds = stop_seconds + (encounter_watch && !boss_run.Active() && !targeted_stop ? 20 : 0);
    const bool quiet = !counters.active && std::chrono::duration<double>(now-compile_quiet_since).count() >= 3;
    if (elapsed >= coverage_seconds && (quiet || elapsed >= coverage_seconds + 15)) {
        boss_run.Finish();
        std::printf("[warper] compilation evidence: +%llu successful new pipelines, +%llu shader/pipeline compile scopes; quiet=%s; exhaustive coverage NOT claimed\n",
                    static_cast<unsigned long long>(counters.pipelines-compile_start.pipelines),
                    static_cast<unsigned long long>(counters.events-compile_start.events), quiet ? "yes" : "no (settling limit)");
        BbWorldWarperObserve::Census(world, player, tour ? route_stops[route_index].lamp : 2412950, "end");
        if (!replaying) combat_done = true;
        runtime_pad_director_press(0);
        runtime_pad_director_sticks(-1,-1,-1,-1);
        if (tour) {
            RecordStop(route_stops[route_index].lamp);
            std::printf("[warper] area %u/%u complete: %s\n", route_index + 1,
                        route_count, route_stops[route_index].name);
            ++route_index;
            if (route_index < route_count) {
                target_lamp = route_stops[route_index].lamp;
                started = false;
                warp_requested = false;
                saw_loading = false;
                elapsed = 0;
                previous = now;
                std::fflush(stdout);
                return;
            }
            std::printf("[warper] lamp tour finished; see done/skip progress for coverage\n");
        }
        finished = true;
        finish_time = now;
        std::printf("[warper] movement and sweep complete after %.1f active seconds; quit normally\n", elapsed);
        std::fflush(stdout);
        return;
    }
    if (boss_run.Active()) {
        const auto input=boss_run.Tick(elapsed);
        runtime_pad_director_press(input.buttons);
        runtime_pad_director_sticks(input.lx,input.ly,input.rx,input.ry);
        return;
    }
    if(targeted_stop) {
        runtime_pad_director_press(0);
        runtime_pad_director_sticks(128,128,128,128);
        return;
    }
    if (elapsed >= stop_seconds) {
        runtime_pad_director_press(0);
        runtime_pad_director_sticks(128,128,255,128);
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
    const auto input = BbWorldWarperMovement::At(elapsed, !combat_done);
    runtime_pad_director_sticks(input.lx, input.ly, input.rx, input.ry);
    runtime_pad_director_press(input.buttons);
}
}

extern "C" void bbgpu_world_warper_set_image(unsigned char* image, std::uint64_t size) {
    if (!BbWorldWarperImage::Bind(image, size)) return;
    const char* mode = std::getenv("BB_WORLD_WARPER");
    if (!mode || std::strcmp(mode, "smoke") != 0) return;
    const char* gear = std::getenv("BB_WORLD_WARPER_GEAR");
    gear_enabled = gear && std::strcmp(gear,"1") == 0;
    gear_state_path = std::getenv("BB_WORLD_WARPER_GEAR_STATE");
    const char* health_stages=std::getenv("BB_WORLD_WARPER_BOSS_HEALTH_STAGES");
    boss_health_stages=health_stages && std::strcmp(health_stages,"1")==0;
    const char* boss_runs=std::getenv("BB_WORLD_WARPER_BOSS_RUNS");
    boss_runs_enabled=boss_runs && std::strcmp(boss_runs,"1")==0;
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
    const bool encounters = route && std::strcmp(route,"encounters") == 0;
    const bool bosses = route && std::strcmp(route,"bosses") == 0;
    tour = route && (std::strcmp(route, "three-area") == 0 || std::strcmp(route, "all-lamps") == 0 || encounters || bosses);
    if(bosses) {
        route_stops=BbWorldWarperRoute::boss_stops.data();
        route_count=static_cast<unsigned>(BbWorldWarperRoute::boss_stops.size());
        gear_enabled=false; combat_done=true;
    }
    if (encounters) {
        route_stops = BbWorldWarperRoute::encounter_stops;
        route_count = 3;
        gear_enabled = false;
        combat_done = true;
    }
    if (route && std::strcmp(route, "all-lamps") == 0) {
        route_stops = BbWorldWarperRoute::all_stops;
        route_count = BbWorldWarperRoute::all_count;
    }
    if (tour) {
        state_path = std::getenv("BB_WORLD_WARPER_STATE");
        if (state_path) {
            if (auto* file = std::fopen(state_path, "r")) {
                unsigned lamp_done = 0;
                char action[8]{};
                while (std::fscanf(file, "%7s %u\n", action, &lamp_done) == 2) {
                    if ((std::strcmp(action,"done") == 0 || std::strcmp(action,"skip") == 0) &&
                        route_index < route_count && lamp_done == route_stops[route_index].lamp) ++route_index;
                }
                std::fclose(file);
            }
        }
        if (route_index == route_count) {
            route_index = 0;
            if (state_path) { if (auto* file = std::fopen(state_path, "w")) std::fclose(file); }
            if (gear_state_path) { if (auto* file = std::fopen(gear_state_path, "w")) std::fclose(file); }
            const char* boss_state=std::getenv("BB_WORLD_WARPER_BOSS_STATE");
            if(boss_state) { if(auto* file=std::fopen(boss_state,"w")) std::fclose(file); }
        }
        if (gear_enabled && gear_state_path) if (auto* file = std::fopen(gear_state_path, "r")) {
            unsigned attempted = 0, failures = 0;
            const bool complete = std::fscanf(file, "attempted %u failures %u", &attempted, &failures) == 2 &&
                                  attempted == BbWorldWarperGear::rows;
            std::fclose(file);
            if (complete) {
                gear_row = BbWorldWarperGear::rows;
                combat_done = true;
                std::printf("[warper] resuming after completed gear pass (%u verification failures); not repeating gear\n", failures);
            }
        }
        if (record_exit) route_index = route_count == BbWorldWarperRoute::count ? 1 : 4;
        target_lamp = route_stops[route_index].lamp;
        std::printf("[warper] lamp tour starting at stop %u of %u\n", route_index + 1, route_count);
    }
    if(boss_runs_enabled) BbWorldWarperEncounter::LoadState(std::getenv("BB_WORLD_WARPER_BOSS_STATE"));
    if (!BbWorldWarperHook::InstallMainThreadTick(Tick)) {
        std::printf("[warper] smoke mode could not install the main-thread hook\n");
    }
}
