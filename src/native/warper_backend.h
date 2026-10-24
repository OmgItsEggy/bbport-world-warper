// SPDX-License-Identifier: GPL-3.0-or-later
// The solo boundary between the portable world-warper state machine and Supermedo's runtime.
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace bbport::warper {

using Address = std::uint64_t;

struct GameState {
    bool world_up = false;
    bool loading = false;
    bool in_game_frame = false;
    Address world = 0;
    Address player = 0;
    Address player_record = 0;
    std::uint32_t map = 0xffffffffu;
};

struct Enemy {
    std::uint32_t entity = 0;
    std::uint32_t kind = 0;
    Address character = 0;
    bool boss = false;
};

struct CompileCounters {
    std::uint64_t created = 0;
    std::uint64_t skipped = 0;
    std::uint64_t waits = 0;
    std::uint64_t compile_us = 0;
    std::uint64_t frames = 0;
    std::uint64_t hitch_frames = 0;
    std::uint64_t hitch_us = 0;
    std::uint64_t shader_hitch_frames = 0;
    std::uint64_t shader_hitch_us = 0;
};

using TickCallback = void (*)();

class Backend {
public:
    virtual ~Backend() = default;

    // Runtime and hooks.
    virtual bool BindImage(unsigned char* image, std::uint64_t size) = 0;
    virtual Address ImageAddress(std::uint64_t offset) const = 0;
    virtual bool Matches109(std::uint64_t offset, std::initializer_list<std::uint8_t> bytes) const = 0;
    virtual bool InstallMainThreadTick(std::uint64_t offset, TickCallback callback) = 0;
    virtual bool SafeRead(Address address, void* output, std::size_t size) const = 0;
    virtual bool SafeWrite(Address address, const void* input, std::size_t size) const = 0;

    template <typename T>
    bool Read(Address address, T* value) const {
        return SafeRead(address, value, sizeof(T));
    }

    // Game lifecycle and input.
    virtual GameState ReadGameState() const = 0;
    virtual void SetButtons(std::uint32_t buttons) = 0;
    virtual void SetSticks(int left_x, int left_y, int right_x, int right_y) = 0;
    virtual bool Warp(std::uint32_t return_point) = 0;
    virtual bool ReturnPointSpot(std::uint32_t return_point, float output[4]) const = 0;
    virtual bool PlacePlayer(Address player, const float position[3], float yaw) = 0;
    virtual void KeepPlayerAlive(Address player) = 0;

    // Parameters and equipment/effects.
    virtual Address ParamRow(const char* table, const char* type, std::uint32_t id) const = 0;
    virtual std::vector<std::pair<std::uint32_t, Address>> ParamRows(const char* table, const char* type) const = 0;
    virtual int ItemCount(std::uint32_t type, std::uint32_t id) const = 0;
    virtual bool GiveItem(std::uint32_t type, std::uint32_t id, int count) = 0;
    virtual bool Equip(int slot, std::uint32_t type, std::uint32_t id) = 0;
    virtual bool SetQuickItem(int slot, std::uint32_t id, bool select) = 0;
    virtual std::uint32_t QuickItem(int slot, bool* selected) const = 0;
    virtual bool ApplyEffect(Address player, std::uint32_t effect) = 0;

    // Enemy and animation coverage.
    virtual std::vector<Enemy> LoadedEnemies() const = 0;
    virtual int EnemyHp(Address character) const = 0;
    virtual bool CharacterPosition(Address character, float output[3]) const = 0;
    virtual bool KillEnemy(const Enemy& enemy) = 0;
    virtual std::vector<int> RecentAnimations(Address character) const = 0;
    virtual bool GesturePlayed(Address character, std::uint32_t gesture, const std::vector<int>& before) const = 0;
    virtual bool ToolUsePlayed(Address character, std::uint32_t tool, int held_before, int bullets_before) const = 0;

    // Cutscenes and world variants.
    virtual bool CutscenePlaying() const = 0;
    virtual bool PlayCutscene(std::uint32_t remo, Address player) = 0;
    virtual bool RequestCutsceneSkip() = 0;
    virtual int TimeOfDay() const = 0;
    virtual bool SetTimeOfDay(int value) = 0;

    // Renderer telemetry and process lifecycle.
    virtual CompileCounters Counters() const = 0;
    virtual std::uint64_t TakeWorstFrameUs() = 0;
    virtual void Log(const std::string& message) = 0;
    [[noreturn]] virtual void ExitAndFlushCache(int code) = 0;
};

} // namespace bbport::warper

