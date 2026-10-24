// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cmath>

namespace BbWorldWarperMovement {
struct Input { int lx, ly, rx, ry; };
inline Input At(double elapsed) {
    if (!std::isfinite(elapsed) || elapsed < 0 || elapsed >= 60) return {-1,-1,-1,-1};
    // Three short out-and-back loops. Keep the camera still while moving so the
    // intended return direction does not rotate during the loop.
    if (elapsed < 18) {
        const double phase = std::fmod(elapsed, 6.0);
        if (phase < 2) return {128,0,128,128};
        if (phase < 4) return {128,255,128,128};
        if (phase < 5) return {0,128,128,128};
        return {255,128,128,128};
    }
    return {128,128,255,128};
}
}
