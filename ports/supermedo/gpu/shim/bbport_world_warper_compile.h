#pragma once
#include <atomic>
#include <cstdint>
namespace BbWorldWarperCompile {
inline std::atomic<std::uint64_t> events{0}, pipelines{0}, serial{0};
inline std::atomic<unsigned> active{0};
struct Scope {
    Scope() { active.fetch_add(1); serial.fetch_add(1); }
    ~Scope() { events.fetch_add(1); serial.fetch_add(1); active.fetch_sub(1); }
};
struct Snapshot { std::uint64_t events, pipelines, serial; unsigned active; };
inline Snapshot Read() { return {events.load(),pipelines.load(),serial.load(),active.load()}; }
}
