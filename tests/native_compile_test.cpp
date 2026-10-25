#include "bbport_world_warper_compile.h"
#include <cassert>
int main() {
    using namespace BbWorldWarperCompile;
    const auto before=Read();
    assert(before.active==0);
    {
        Scope outer;
        assert(Read().active==1);
        { Scope inner; assert(Read().active==2); }
        assert(Read().events==before.events+1);
    }
    const auto after=Read();
    assert(after.active==0 && after.events==before.events+2);
    assert(after.serial==before.serial+4);
    assert(after.pipelines==before.pipelines);
}
