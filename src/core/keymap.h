// AW410K key map: physical order, 107 keys. idx = hardware LED index used by
// the per-key lighting protocol. Labels match tools/aw410k.py for parity.
//
// x/y/w/h describe each key's position on a standard ANSI 100% layout (plus the
// three media keys above the numpad), measured in key-units (1u = one standard
// key). They drive the graphical per-key editor; the lighting protocol itself
// only uses idx, so the geometry is purely cosmetic.
#pragma once

#include <cstddef>
#include <cstdint>

namespace krgb {

// Supported keyboard models. The AW410K and AW510K share the exact same
// lighting protocol and LED index map; the AW510K simply lacks the discrete
// volume-down/volume-up keys. (Verified against the OpenRGB Alienware drivers.)
enum Model : std::uint8_t { ModelAW410K = 0, ModelAW510K = 1 };

constexpr std::uint8_t modelBit(Model m) { return static_cast<std::uint8_t>(1u << m); }
inline constexpr std::uint8_t kAllModels = 0xFF;

struct KeyDef {
    const char*  name;
    std::uint8_t idx;
    float        x;
    float        y;
    float        w      = 1.0f;
    float        h      = 1.0f;
    std::uint8_t models = kAllModels;  // bitmask of models that have this key
};

inline constexpr KeyDef kKeyMap[] = {
    // Function / media row (y = 0)
    {"ESC",0xB0, 0,0},{"F1",0x98, 2,0},{"F2",0x90, 3,0},{"F3",0x88, 4,0},{"F4",0x80, 5,0},
    {"F5",0x70, 6.5f,0},{"F6",0x68, 7.5f,0},{"F7",0x60, 8.5f,0},{"F8",0x58, 9.5f,0},
    {"F9",0x50, 11,0},{"F10",0x48, 12,0},{"F11",0x40, 13,0},{"F12",0x38, 14,0},
    {"PRTSC",0x30, 15.25f,0},{"SCRLK",0x28, 16.25f,0},{"PAUSE",0x20, 17.25f,0},
    {"MUTE",0x18, 18.5f,0},
    {"VOLDN",0x10, 19.5f,0, 1.0f,1.0f, modelBit(ModelAW410K)},  // AW510K has no
    {"VOLUP",0x08, 20.5f,0, 1.0f,1.0f, modelBit(ModelAW410K)},  // discrete vol keys

    // Number row (y = 1.25)
    {"`",0xB1, 0,1.25f},{"1",0xA1, 1,1.25f},{"2",0x99, 2,1.25f},{"3",0x91, 3,1.25f},
    {"4",0x89, 4,1.25f},{"5",0x81, 5,1.25f},{"6",0x79, 6,1.25f},{"7",0x71, 7,1.25f},
    {"8",0x69, 8,1.25f},{"9",0x61, 9,1.25f},{"0",0x59, 10,1.25f},{"MINUS",0x51, 11,1.25f},
    {"EQUALS",0x49, 12,1.25f},{"BACKSPACE",0x39, 13,1.25f, 2.0f},
    {"INS",0x31, 15.25f,1.25f},{"HOME",0x29, 16.25f,1.25f},{"PGUP",0x21, 17.25f,1.25f},
    {"NUMLK",0x19, 18.5f,1.25f},{"NP/",0x11, 19.5f,1.25f},{"NP*",0x09, 20.5f,1.25f},
    {"NP-",0x01, 21.5f,1.25f},

    // Top letter row (y = 2.25)
    {"TAB",0xB2, 0,2.25f, 1.5f},{"Q",0xA2, 1.5f,2.25f},{"W",0x9A, 2.5f,2.25f},
    {"E",0x92, 3.5f,2.25f},{"R",0x8A, 4.5f,2.25f},{"T",0x82, 5.5f,2.25f},{"Y",0x7A, 6.5f,2.25f},
    {"U",0x72, 7.5f,2.25f},{"I",0x6A, 8.5f,2.25f},{"O",0x62, 9.5f,2.25f},{"P",0x5A, 10.5f,2.25f},
    {"[",0x52, 11.5f,2.25f},{"]",0x4A, 12.5f,2.25f},{"\\",0x42, 13.5f,2.25f, 1.5f},
    {"DEL",0x32, 15.25f,2.25f},{"END",0x2A, 16.25f,2.25f},{"PGDN",0x22, 17.25f,2.25f},
    {"NP7",0x1A, 18.5f,2.25f},{"NP8",0x12, 19.5f,2.25f},{"NP9",0x0A, 20.5f,2.25f},
    {"NP+",0x03, 21.5f,2.25f, 1.0f,2.0f},

    // Home row (y = 3.25)
    {"CAPS",0xB3, 0,3.25f, 1.75f},{"A",0xA3, 1.75f,3.25f},{"S",0x9B, 2.75f,3.25f},
    {"D",0x93, 3.75f,3.25f},{"F",0x8B, 4.75f,3.25f},{"G",0x83, 5.75f,3.25f},{"H",0x7B, 6.75f,3.25f},
    {"J",0x73, 7.75f,3.25f},{"K",0x6B, 8.75f,3.25f},{"L",0x63, 9.75f,3.25f},{";",0x5B, 10.75f,3.25f},
    {"'",0x53, 11.75f,3.25f},{"ENTER",0x43, 12.75f,3.25f, 2.25f},
    {"NP4",0x1B, 18.5f,3.25f},{"NP5",0x13, 19.5f,3.25f},{"NP6",0x0B, 20.5f,3.25f},

    // Bottom letter row (y = 4.25)
    {"LSHIFT",0xB4, 0,4.25f, 2.25f},{"Z",0xA4, 2.25f,4.25f},{"X",0x9C, 3.25f,4.25f},
    {"C",0x94, 4.25f,4.25f},{"V",0x8C, 5.25f,4.25f},{"B",0x84, 6.25f,4.25f},{"N",0x7C, 7.25f,4.25f},
    {"M",0x74, 8.25f,4.25f},{",",0x6C, 9.25f,4.25f},{".",0x64, 10.25f,4.25f},{"/",0x5C, 11.25f,4.25f},
    {"RSHIFT",0x4C, 12.25f,4.25f, 2.75f},{"UP",0x2C, 16.25f,4.25f},
    {"NP1",0x1C, 18.5f,4.25f},{"NP2",0x14, 19.5f,4.25f},{"NP3",0x0C, 20.5f,4.25f},
    {"NPENTER",0x05, 21.5f,4.25f, 1.0f,2.0f},

    // Modifier / space row (y = 5.25)
    {"LCTRL",0xB5, 0,5.25f, 1.25f},{"LWIN",0xAD, 1.25f,5.25f, 1.25f},{"LALT",0xA5, 2.5f,5.25f, 1.25f},
    {"SPACE",0x85, 3.75f,5.25f, 6.25f},{"RALT",0x65, 10,5.25f, 1.25f},{"RFUNC",0x5D, 11.25f,5.25f, 1.25f},
    {"MENU",0x55, 12.5f,5.25f, 1.25f},{"RCTRL",0x4D, 13.75f,5.25f, 1.25f},
    {"LEFT",0x35, 15.25f,5.25f},{"DOWN",0x2D, 16.25f,5.25f},{"RIGHT",0x25, 17.25f,5.25f},
    {"NP0",0x1D, 18.5f,5.25f, 2.0f},{"NP.",0x0D, 20.5f,5.25f},
};

inline constexpr std::size_t kKeyCount = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

// Total layout extent in key-units (for the editor to scale to fit). Both
// supported models share the same physical layout.
inline constexpr float kLayoutWidth  = 22.5f;
inline constexpr float kLayoutHeight = 6.25f;

// --- Supported keyboards ----------------------------------------------------
// A model is identified by USB vendor/product id on its lighting interface.
// `bit` selects which kKeyMap keys belong to it (see KeyDef::models).
struct KeyboardModel {
    const char*   name;
    std::uint16_t vendorId;
    std::uint16_t productId;
    int           lightingInterface;
    std::uint8_t  bit;
};

inline constexpr KeyboardModel kModels[] = {
    { "Alienware AW410K", 0x04F2, 0x1968, 2, modelBit(ModelAW410K) },
    { "Alienware AW510K", 0x04F2, 0x1830, 2, modelBit(ModelAW510K) },
};
inline constexpr std::size_t kModelCount = sizeof(kModels) / sizeof(kModels[0]);

// Number of addressable keys for a given model (kKeyMap entries it includes).
inline std::size_t modelKeyCount(std::uint8_t bit) {
    std::size_t n = 0;
    for(std::size_t i = 0; i < kKeyCount; ++i) {
        if(kKeyMap[i].models & bit) {
            ++n;
        }
    }
    return n;
}

} // namespace krgb
