// SPDX-License-Identifier: GPL-3.0-or-later
// Minimal, solo-only detour derived from the GPL world-warper reference hook helper.
#include "bbport_world_warper_hook.h"

#include "bbport_world_warper_image.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <mutex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

extern "C" void* runtime_low_map(std::size_t size, int prot);

namespace BbWorldWarperHook {
namespace {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

constexpr u64 kFlipperUpdate = 0x2034770;
constexpr std::array<u8, 17> kFlipperPrologue = {
    0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x41, 0x56, 0x41,
    0x55, 0x41, 0x54, 0x53, 0x48, 0x83, 0xec, 0x38,
};
constexpr std::size_t kStubPageSize = 16384;

std::mutex g_install_mutex;
u8* g_stub_page = nullptr;
std::size_t g_stub_used = 0;
TickCallback g_callback = nullptr;
bool g_installed = false;

bool Rel32(u64 from_next, u64 to, std::int32_t* output) {
    const auto distance = static_cast<std::int64_t>(to) - static_cast<std::int64_t>(from_next);
    if (distance < std::numeric_limits<std::int32_t>::min() ||
        distance > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    *output = static_cast<std::int32_t>(distance);
    return true;
}

bool StubWritable(bool writable) {
#ifdef _WIN32
    DWORD unused = 0;
    const DWORD protection = writable ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    if (!VirtualProtect(g_stub_page, kStubPageSize, protection, &unused)) return false;
    if (!writable) FlushInstructionCache(GetCurrentProcess(), g_stub_page, kStubPageSize);
    return true;
#else
    return mprotect(g_stub_page, kStubPageSize,
                    writable ? PROT_READ | PROT_WRITE | PROT_EXEC : PROT_READ | PROT_EXEC) == 0;
#endif
}

bool EnsureStubPage() {
    if (g_stub_page) return true;
    auto* page = static_cast<u8*>(runtime_low_map(kStubPageSize, 3 /* read | write */));
    if (!page) return false;

    std::int32_t ignored = 0;
    const u64 hook = BbWorldWarperImage::Address(kFlipperUpdate);
    if (!hook || !Rel32(hook + 5, reinterpret_cast<u64>(page), &ignored)) {
        std::printf("[warper] main-thread hook: stub memory is outside rel32 reach\n");
        return false;
    }
    g_stub_page = page;
    return true;
}

struct Emitter {
    std::array<u8, 384> code{};
    std::size_t size = 0;
    u64 at = 0;
    bool ok = true;

    void Byte(u8 byte) {
        if (size < code.size()) code[size++] = byte;
        else ok = false;
    }
    void Bytes(std::initializer_list<u8> bytes) {
        for (const auto byte : bytes) Byte(byte);
    }
    void Raw(const u8* bytes, std::size_t count) {
        for (std::size_t index = 0; index < count; ++index) Byte(bytes[index]);
    }
    void U32(u32 value) {
        u8 bytes[4];
        std::memcpy(bytes, &value, sizeof(bytes));
        Raw(bytes, sizeof(bytes));
    }
    void U64(u64 value) {
        u8 bytes[8];
        std::memcpy(bytes, &value, sizeof(bytes));
        Raw(bytes, sizeof(bytes));
    }
    void JmpTo(u64 target) {
        Byte(0xe9);
        std::int32_t relative = 0;
        ok &= Rel32(at + size + 4, target, &relative);
        U32(static_cast<u32>(relative));
    }
    void CallHost(const void* function) {
        // MinGW's callback uses the Windows x64 ABI: reserve its 32-byte shadow space.
        Bytes({0x48, 0x83, 0xec, 0x20});
        Bytes({0x48, 0xb8});
        U64(reinterpret_cast<u64>(function));
        Bytes({0xff, 0xd0});
        Bytes({0x48, 0x83, 0xc4, 0x20});
    }
    void SkipGuestRedZone() { Bytes({0x48, 0x8d, 0xa4, 0x24, 0x80, 0xff, 0xff, 0xff}); }
    void RestoreGuestRedZone() { Bytes({0x48, 0x8d, 0xa4, 0x24, 0x80, 0x00, 0x00, 0x00}); }
    void SaveVolatile() {
        Bytes({0x50, 0x51, 0x52, 0x56, 0x57, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53});
        Bytes({0x41, 0x54});
        Bytes({0x49, 0x89, 0xe4});
        Bytes({0x48, 0x83, 0xe4, 0xf0});
        Bytes({0x48, 0x81, 0xec, 0x80, 0x00, 0x00, 0x00});
        for (u8 index = 0; index < 8; ++index) {
            Bytes({0xf3, 0x0f, 0x7f, static_cast<u8>(0x44 | (index << 3)), 0x24,
                   static_cast<u8>(index * 16)});
        }
    }
    void RestoreVolatile() {
        for (u8 index = 0; index < 8; ++index) {
            Bytes({0xf3, 0x0f, 0x6f, static_cast<u8>(0x44 | (index << 3)), 0x24,
                   static_cast<u8>(index * 16)});
        }
        Bytes({0x4c, 0x89, 0xe4});
        Bytes({0x41, 0x5c});
        Bytes({0x41, 0x5b, 0x41, 0x5a, 0x41, 0x59, 0x41, 0x58, 0x5f, 0x5e, 0x5a, 0x59, 0x58});
    }
};

u64 PlaceStub(const Emitter& emitter) {
    if (!emitter.ok || !g_stub_page ||
        emitter.at != reinterpret_cast<u64>(g_stub_page + g_stub_used) ||
        g_stub_used + emitter.size > kStubPageSize) {
        return 0;
    }
    if (!StubWritable(true)) return 0;
    auto* destination = g_stub_page + g_stub_used;
    std::memcpy(destination, emitter.code.data(), emitter.size);
    g_stub_used += (emitter.size + 15) & ~std::size_t(15);
    if (!StubWritable(false)) return 0;
    return reinterpret_cast<u64>(destination);
}

bool WriteEntryBranch(u64 stub) {
    const u64 address = BbWorldWarperImage::Address(kFlipperUpdate);
    std::int32_t relative = 0;
    if (!address || !stub || !Rel32(address + 5, stub, &relative)) return false;

    std::array<u8, kFlipperPrologue.size()> patch{};
    patch.fill(0x90);
    patch[0] = 0xe9;
    std::memcpy(patch.data() + 1, &relative, sizeof(relative));
    auto* destination = reinterpret_cast<void*>(address);
#ifdef _WIN32
    DWORD old = 0;
    if (!VirtualProtect(destination, patch.size(), PAGE_EXECUTE_READWRITE, &old)) return false;
    std::memcpy(destination, patch.data(), patch.size());
    DWORD unused = 0;
    VirtualProtect(destination, patch.size(), old, &unused);
    FlushInstructionCache(GetCurrentProcess(), destination, patch.size());
#else
    const long page_size = sysconf(_SC_PAGESIZE);
    const auto page = address & ~static_cast<u64>(page_size - 1);
    if (mprotect(reinterpret_cast<void*>(page), static_cast<std::size_t>(page_size),
                 PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return false;
    }
    std::memcpy(destination, patch.data(), patch.size());
    __builtin___clear_cache(reinterpret_cast<char*>(destination),
                            reinterpret_cast<char*>(destination) + patch.size());
#endif
    return true;
}

} // namespace

bool InstallMainThreadTick(TickCallback callback) {
    std::lock_guard<std::mutex> lock(g_install_mutex);
    if (g_installed) return callback && callback == g_callback;
    if (!callback || !BbWorldWarperImage::IsBound()) return false;
    if (!BbWorldWarperImage::Matches(
            kFlipperUpdate,
            {0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x41, 0x56, 0x41,
             0x55, 0x41, 0x54, 0x53, 0x48, 0x83, 0xec, 0x38})) {
        std::printf("[warper] main-thread hook: Bloodborne 1.09 prologue differs; not installed\n");
        return false;
    }
    if (!EnsureStubPage()) return false;

    Emitter emitter;
    emitter.at = reinterpret_cast<u64>(g_stub_page + g_stub_used);
    emitter.SkipGuestRedZone();
    emitter.SaveVolatile();
    emitter.CallHost(reinterpret_cast<const void*>(callback));
    emitter.RestoreVolatile();
    emitter.RestoreGuestRedZone();
    emitter.Raw(kFlipperPrologue.data(), kFlipperPrologue.size());
    emitter.JmpTo(BbWorldWarperImage::Address(kFlipperUpdate + kFlipperPrologue.size()));

    const u64 stub = PlaceStub(emitter);
    if (!stub || !WriteEntryBranch(stub)) {
        std::printf("[warper] main-thread hook: failed to place the verified detour\n");
        return false;
    }
    g_callback = callback;
    g_installed = true;
    std::printf("[warper] main-thread tick installed at guest +0x%llx\n",
                static_cast<unsigned long long>(kFlipperUpdate));
    return true;
}

bool MainThreadTickInstalled() {
    std::lock_guard<std::mutex> lock(g_install_mutex);
    return g_installed;
}

} // namespace BbWorldWarperHook
