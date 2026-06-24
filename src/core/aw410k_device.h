// AW410KDevice — low-level driver for the Alienware AW410K RGB keyboard.
//
// Talks directly to the keyboard's vendor HID interface (interface 2,
// usage page 0xFF00) via /dev/hidrawN. No external dependencies; Linux-only.
//
// This is a clean C++ implementation of the AW410K lighting protocol; packet
// layouts are validated against tools/aw410k.py.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace krgb {

struct KeyColor {
    std::uint8_t idx;
    std::uint8_t r, g, b;
};

enum class Mode : std::uint8_t {
    Off         = 0x00,
    Direct      = 0x01,
    Pulse       = 0x02,
    Morph       = 0x03,
    Breathing   = 0x07,
    Spectrum    = 0x08,
    SingleWave  = 0x0F,
    RainbowWave = 0x10,
    Scanner     = 0x11,
    Static      = 0x13,
};

enum class Speed : std::uint8_t { Slowest = 0x2D, Normal = 0x19, Fastest = 0x0A };
enum class Direction : std::uint8_t { Right = 0x01, Left = 0x02, Down = 0x03, Up = 0x04 };
enum class ColorMode : std::uint8_t { Single = 0x01, Two = 0x02, Rainbow = 0x03 };

class AW410KDevice {
public:
    static constexpr std::uint16_t kVendorId          = 0x04F2;
    static constexpr std::uint16_t kProductId         = 0x1968;
    static constexpr int           kLightingInterface = 2;
    static constexpr int           kReportLen         = 65;  // report-id byte + 64 payload

    AW410KDevice() = default;
    ~AW410KDevice();
    AW410KDevice(const AW410KDevice&) = delete;
    AW410KDevice& operator=(const AW410KDevice&) = delete;

    // Locate the hidraw node for the lighting interface; "" if not present.
    static std::string findDevicePath();

    bool open(std::string* err = nullptr);                        // auto-detect
    bool openPath(const std::string& path, std::string* err = nullptr);
    void close();
    bool isOpen() const { return fd_ >= 0; }
    const std::string& path() const { return path_; }

    // High-level operations.
    bool setSolid(std::uint8_t r, std::uint8_t g, std::uint8_t b);   // whole keyboard
    bool setStatic(std::uint8_t r, std::uint8_t g, std::uint8_t b);
    bool setOff();
    bool setEffect(Mode mode, Speed speed, Direction dir, ColorMode cm,
                   std::uint8_t r, std::uint8_t g, std::uint8_t b);
    bool setMorph(Speed speed,
                  std::uint8_t r1, std::uint8_t g1, std::uint8_t b1,
                  std::uint8_t r2, std::uint8_t g2, std::uint8_t b2);
    bool setPerKey(const std::vector<KeyColor>& keys);

    // Protocol primitives (exposed for the GUI / testing).
    bool commit();
    bool initialize();
    bool featureReport(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d);
    bool sendMode(Mode mode, Speed speed, Direction dir, ColorMode cm,
                  std::uint8_t r, std::uint8_t g, std::uint8_t b);

private:
    using Report = std::array<std::uint8_t, kReportLen>;
    bool writeReport(const Report& buf);
    bool sendFeature(const Report& buf);

    int         fd_ = -1;
    std::string path_;
};

} // namespace krgb
