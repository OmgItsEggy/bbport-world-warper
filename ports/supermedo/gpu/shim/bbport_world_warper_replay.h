// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdio>
#include <vector>
#include <algorithm>

namespace BbWorldWarperReplay {
struct Sample { unsigned ms, buttons, lx, ly, rx, ry, l2, r2; };
class Route {
    std::vector<Sample> samples;
public:
    bool Load(const char* path) {
        samples.clear();
        auto* file = path ? std::fopen(path, "r") : nullptr;
        if (!file) return false;
        Sample sample{};
        bool valid = true;
        int fields;
        while ((fields = std::fscanf(file, "%u %u %u %u %u %u %u %u", &sample.ms,
                &sample.buttons, &sample.lx, &sample.ly, &sample.rx, &sample.ry, &sample.l2, &sample.r2)) == 8) {
            if (sample.ms > 600000 || (!samples.empty() && sample.ms < samples.back().ms) ||
                sample.lx > 255 || sample.ly > 255 || sample.rx > 255 || sample.ry > 255 ||
                sample.l2 > 255 || sample.r2 > 255 || samples.size() >= 100000) { valid = false; break; }
            samples.push_back(sample);
        }
        valid = valid && fields == EOF && samples.size() >= 2 && samples.back().ms > 0;
        std::fclose(file);
        if (!valid) samples.clear();
        return valid;
    }
    double Seconds() const { return samples.empty() ? 0 : samples.back().ms / 1000.0 + 0.2; }
    Sample At(double seconds) const {
        if (samples.empty() || seconds < 0 || seconds >= Seconds()) return {0,0,128,128,128,128,0,0};
        const auto ms = static_cast<unsigned>(seconds * 1000);
        auto next = std::upper_bound(samples.begin(), samples.end(), ms,
            [](unsigned time, const Sample& sample) { return time < sample.ms; });
        return next == samples.begin() ? Sample{0,0,128,128,128,128,0,0} : *--next;
    }
};
}
