#include "bbport_world_warper_image.h"
#include <array>
#include <cassert>
#include <limits>

int main() {
    namespace Image = BbWorldWarperImage;
    std::array<unsigned char, 8> bytes{0x55, 0x48, 0x89, 0xe5};
    std::array<unsigned char, 8> other{};
    Image::Reset();
    assert(!Image::Bind(nullptr, 8));
    assert(!Image::Bind(bytes.data(), 0));
    assert(!Image::IsBound());
    assert(Image::Address(0) == 0);
    assert(Image::Bind(bytes.data(), bytes.size()));
    assert(Image::Bind(bytes.data(), bytes.size()));
    assert(!Image::Bind(other.data(), other.size()));
    assert(!Image::Bind(bytes.data(), 4));
    assert(Image::Size() == 8);
    assert(Image::Address(7) != 0);
    assert(Image::Address(8) == 0);
    assert(Image::Matches(0, {0x55, 0x48, 0x89, 0xe5}));
    assert(!Image::Matches(0, {0x90}));
    assert(!Image::Matches(0, {}));
    assert(!Image::Matches(7, {0, 0}));
    assert(!Image::Matches(std::numeric_limits<std::uint64_t>::max(), {0}));
    Image::Reset();
    assert(!Image::Matches(0, {0x55}));
    assert(Image::Bind(other.data(), other.size()));
    Image::Reset();
}
