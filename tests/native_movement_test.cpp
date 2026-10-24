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
    assert(At(18).ly == 128 && At(18).rx == 255);
    assert(At(59.9).lx == 128);
    assert(At(60).lx == -1);
    assert(At(-1).lx == -1);
    assert(At(std::numeric_limits<double>::infinity()).lx == -1);
    for (const auto& stop : BbWorldWarperRoute::stops) {
        assert(coop::lamptravel::LampOf(stop.lamp, nullptr));
        assert(coop::lamptravel::MapOf(stop.lamp) == stop.map);
    }
}
