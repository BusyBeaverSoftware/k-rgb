// krgb-cli — exercises the AW410K core engine on real hardware.
// Mirrors tools/aw410k.py so the C++ port can be validated against it.
#include "core/aw410k_device.h"
#include "core/keymap.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>

#include <string>
#include <vector>

using namespace krgb;

namespace {

void hsv(double h, double s, double v, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
    const double i = std::floor(h * 6.0);
    const double f = h * 6.0 - i;
    const double p = v * (1.0 - s);
    const double q = v * (1.0 - f * s);
    const double t = v * (1.0 - (1.0 - f) * s);
    double rr = v, gg = t, bb = p;
    switch(static_cast<int>(i) % 6) {
        case 0: rr = v; gg = t; bb = p; break;
        case 1: rr = q; gg = v; bb = p; break;
        case 2: rr = p; gg = v; bb = t; break;
        case 3: rr = p; gg = q; bb = v; break;
        case 4: rr = t; gg = p; bb = v; break;
        default: rr = v; gg = p; bb = q; break;
    }
    r = static_cast<std::uint8_t>(rr * 255);
    g = static_cast<std::uint8_t>(gg * 255);
    b = static_cast<std::uint8_t>(bb * 255);
}

void usage() {
    std::printf(
        "krgb-cli — Alienware AW410K RGB control\n"
        "  krgb-cli info\n"
        "  krgb-cli solid R G B        whole keyboard, direct\n"
        "  krgb-cli static R G B       whole keyboard, hardware static\n"
        "  krgb-cli off\n"
        "  krgb-cli spectrum           hardware rainbow spectrum\n"
        "  krgb-cli breathing R G B\n"
        "  krgb-cli wave R G B         single-colour wave\n"
        "  krgb-cli rainbow            per-key static rainbow\n"
        "  krgb-cli key LABEL R G B    light one key (see keymap.h)\n");
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> a(argv + 1, argv + argc);
    if(a.empty()) {
        usage();
        return 2;
    }
    const std::string cmd = a[0];

    if(cmd == "info") {
        const std::string p = AW410KDevice::findDevicePath();
        std::printf("AW410K device : %s\n", p.empty() ? "NOT FOUND" : p.c_str());
        std::printf("keys          : %zu\n", kKeyCount);
        return 0;
    }

    AW410KDevice dev;
    std::string err;
    if(!dev.open(&err)) {
        std::fprintf(stderr, "error: %s\n", err.c_str());
        return 1;
    }

    auto U = [&](std::size_t i) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::strtol(a[i].c_str(), nullptr, 0));
    };

    bool ok = true;
    if(cmd == "solid" && a.size() >= 4) {
        ok = dev.setSolid(U(1), U(2), U(3));
    } else if(cmd == "static" && a.size() >= 4) {
        ok = dev.setStatic(U(1), U(2), U(3));
    } else if(cmd == "off") {
        ok = dev.setOff();
    } else if(cmd == "spectrum") {
        ok = dev.setEffect(Mode::Spectrum, Speed::Normal, Direction::Left, ColorMode::Rainbow, 0, 0, 0);
    } else if(cmd == "breathing" && a.size() >= 4) {
        ok = dev.setEffect(Mode::Breathing, Speed::Normal, Direction::Right, ColorMode::Single, U(1), U(2), U(3));
    } else if(cmd == "wave" && a.size() >= 4) {
        ok = dev.setEffect(Mode::SingleWave, Speed::Normal, Direction::Left, ColorMode::Single, U(1), U(2), U(3));
    } else if(cmd == "rainbow") {
        std::vector<KeyColor> keys;
        keys.reserve(kKeyCount);
        for(std::size_t i = 0; i < kKeyCount; ++i) {
            std::uint8_t r, g, b;
            hsv(static_cast<double>(i) / kKeyCount, 1.0, 1.0, r, g, b);
            keys.push_back({kKeyMap[i].idx, r, g, b});
        }
        ok = dev.setPerKey(keys);
    } else if(cmd == "key" && a.size() >= 5) {
        std::uint8_t idx = 0;
        bool found = false;
        for(std::size_t i = 0; i < kKeyCount; ++i) {
            if(strcasecmp(kKeyMap[i].name, a[1].c_str()) == 0) {
                idx = kKeyMap[i].idx;
                found = true;
                break;
            }
        }
        if(!found) {
            std::fprintf(stderr, "unknown key label: %s\n", a[1].c_str());
            return 2;
        }
        ok = dev.setPerKey({{idx, U(2), U(3), U(4)}});
    } else {
        usage();
        return 2;
    }

    if(!ok) {
        std::fprintf(stderr, "error: device write failed\n");
        return 1;
    }
    return 0;
}
