#include "bbport_world_warper_walk.h"
#include <cassert>
int main() {
    using namespace BbWorldWarperEncounter;
    // All camera directions, including initial motion away from the destination.
    for(float yaw:{0.f,0.7f,1.57f,3.14f,4.7f}) for(float hand:{-1.f,1.f}) {
        Walker walker; Point p{0,0,0},goal{10,0,10};
        const float fx=std::sin(yaw),fz=-std::cos(yaw);
        for(int frame=0;frame<1200 && Distance(p,goal)>1;++frame) {
            auto input=walker.Toward(frame/60.,p,goal);
            const float side=(input.lx-128)/96.f,forward=(128-input.ly)/96.f;
            p.x+=(forward*fx-hand*side*fz)*3/60; p.z+=(forward*fz+hand*side*fx)*3/60;
        }
        assert(Distance(p,goal)<=1.1f && !walker.stalled);
    }
    Walker blocked; blocked.Toward(0,{0,0,0},{10,0,10});
    auto input=blocked.Toward(6,{0,0,0},{10,0,10});
    assert(blocked.stalled && input.ly==128);
    FloorGuard guard; const auto* script=ScriptFor(2420810);
    guard.Begin({0,55,280},*script);
    assert(guard.Check(0,{0,55,280}));
    assert(guard.Check(1,{0,48,280})); // authored stairs, no arbitrary vertical teleport
    assert(!guard.Check(2,{0,40,280}));
    guard.Begin({0,55,280},*script);
    assert(guard.Check(0,{0,55,280}));
    assert(guard.Check(0.1,{0,53,280}));
    assert(guard.Check(0.2,{0,51,280}));
    assert(!guard.Check(0.3,{0,49,280})); // fast fall before leaving total height envelope
}
