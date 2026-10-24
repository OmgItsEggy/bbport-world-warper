// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace BbWorldWarperHook {

using TickCallback = void (*)();

// Installs one byte-verified Bloodborne 1.09 SprjFlipper::Update entry hook.
// The callback runs on the game's main thread before the original function.
bool InstallMainThreadTick(TickCallback callback);
bool MainThreadTickInstalled();

} // namespace BbWorldWarperHook
