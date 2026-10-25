// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cmath>

namespace BbWorldWarperMovement {
struct Input { int lx, ly, rx, ry; unsigned buttons = 0; };
inline Input At(double elapsed, bool combat = true) {
    if (!std::isfinite(elapsed) || elapsed < 0 || elapsed >= (combat ? 60 : 38)) return {-1,-1,-1,-1};
    if (!combat && elapsed >= 18) elapsed += 22;
    // Three short out-and-back loops. Keep the camera still while moving so the
    // intended return direction does not rotate during the loop.
    if (elapsed < 18) {
        const double phase = std::fmod(elapsed, 6.0);
        if (phase < 2) return {128,0,128,128};
        if (phase < 4) return {128,255,128,128};
        if (phase < 5) return {0,128,128,128};
        return {255,128,128,128};
    }
    if (elapsed < 40) {
        const double t = elapsed - 18;
        unsigned buttons = 0;
        if (t < 0.2 || (t >= 1.5 && t < 1.7)) buttons = 0x800; // R1
        else if (t >= 3 && t < 4.6) buttons = 0x200; // charged R2
        else if (t >= 6 && t < 6.2) buttons = 0x400; // transform
        else if (t >= 8 && t < 8.2) buttons = 0x800;
        else if (t >= 10 && t < 11.6) buttons = 0x200;
        else if (t >= 13 && t < 13.2) buttons = 0x400; // transform back
        else if (t >= 15 && t < 15.2) buttons = 0x100; // firearm
        else if (t >= 17 && t < 17.2) buttons = 0x2000; // dodge
        return {128,128,128,128,buttons};
    }
    return {128,128,255,128,0};
}
}
