#include "bbport_world_warper_movement.h"
#include "bbport_world_warper_route.h"
#include "../src/native/lamp_travel_rules.h"
#include <cassert>
#include <limits>
int main() {
    using BbWorldWarperMovement::At;
    assert(At(0).ly == 0);
    assert(At(2).ly == 255);
    assert(At(4).lx == 0);
    assert(At(5).lx == 255);
    assert(At(6).ly == 0);
    assert(At(17).lx == 255);
    assert(At(18).buttons == 0x800);
    assert(At(21.5).buttons == 0x200);
    assert(At(24.1).buttons == 0x400);
    assert(At(33.1).buttons == 0x100);
    assert(At(35.1).buttons == 0x2000);
    assert(At(40).rx == 255 && At(40).buttons == 0);
    assert(At(59.9).lx == 128);
    assert(At(60).lx == -1);
    assert(At(-1).lx == -1);
    assert(At(std::numeric_limits<double>::infinity()).lx == -1);
    for (double seconds = 0; seconds < 38; seconds += 0.05) {
        assert(At(seconds, false).buttons == 0);
    }
    assert(At(18, false).rx == 255);
    assert(At(37.9, false).rx == 255);
    assert(At(38, false).lx == -1);
    for (const auto& stop : BbWorldWarperRoute::stops) {
        assert(coop::lamptravel::LampOf(stop.lamp, nullptr));
        assert(coop::lamptravel::MapOf(stop.lamp) == stop.map);
    }
    assert(BbWorldWarperRoute::all_count == coop::lamptravel::kLampCount);
    for (unsigned i = 0; i < BbWorldWarperRoute::all_count; ++i) {
        const auto& stop = BbWorldWarperRoute::all_stops[i];
        assert(coop::lamptravel::LampOf(stop.lamp, nullptr));
        assert(coop::lamptravel::MapOf(stop.lamp) == stop.map);
        for (unsigned j = 0; j < i; ++j) assert(stop.lamp != BbWorldWarperRoute::all_stops[j].lamp);
    }
}
