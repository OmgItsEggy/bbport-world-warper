#include "bbport_world_warper_replay.h"
#include <cassert>
int main(int argc, char** argv) {
    assert(argc == 2);
    BbWorldWarperReplay::Route route;
    assert(!route.Load(nullptr));
    assert(route.Load(argv[1]));
    assert(route.Seconds() > 32 && route.Seconds() < 34);
    const auto first = route.At(0);
    assert(first.lx == 125 && first.ly == 131 && first.rx == 130 && first.ry == 123);
    const auto end = route.At(route.Seconds() - 0.01);
    assert(end.lx == 128 && end.ly == 128 && end.rx == 125 && end.ry == 122);
    assert(route.At(-1).lx == 128);
    assert(route.At(route.Seconds()).buttons == 0);
    assert(route.At(route.Seconds()).rx == 128);
    assert(!route.Load("missing-route.pad"));
    assert(route.Seconds() == 0);
}
