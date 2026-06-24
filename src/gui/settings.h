// LightingSettings — the persisted lighting choice (KConfig-backed).
//
// Shared by the GUI (save on apply / load on start) and the headless
// `krgb --apply` restore path used by the autostart entry.
#pragma once

#include <QColor>

#include "core/aw410k_device.h"

class KeyboardController;

struct LightingSettings {
    enum Kind { Solid, Rainbow, Effect };

    Kind   kind       = Solid;
    int    effectMode = 0;  // krgb::Mode value, used when kind == Effect
    QColor color      = QColor(0, 170, 255);
    int    speed      = static_cast<int>(krgb::Speed::Normal);
    int    direction  = static_cast<int>(krgb::Direction::Left);
    int    brightness = 100;

    void save() const;
    static LightingSettings load();
    bool apply(KeyboardController& controller) const;
};
