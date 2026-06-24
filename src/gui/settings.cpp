#include "settings.h"

#include "keyboardcontroller.h"

#include <KConfigGroup>
#include <KSharedConfig>

void LightingSettings::save() const {
    KConfigGroup g(KSharedConfig::openConfig(), QStringLiteral("Lighting"));
    g.writeEntry("kind", static_cast<int>(kind));
    g.writeEntry("effectMode", effectMode);
    g.writeEntry("color", color);
    g.writeEntry("speed", speed);
    g.writeEntry("direction", direction);
    g.writeEntry("brightness", brightness);
    g.sync();
}

LightingSettings LightingSettings::load() {
    LightingSettings s;
    const KConfigGroup g(KSharedConfig::openConfig(), QStringLiteral("Lighting"));
    s.kind       = static_cast<Kind>(g.readEntry("kind", static_cast<int>(s.kind)));
    s.effectMode = g.readEntry("effectMode", s.effectMode);
    s.color      = g.readEntry("color", s.color);
    s.speed      = g.readEntry("speed", s.speed);
    s.direction  = g.readEntry("direction", s.direction);
    s.brightness = g.readEntry("brightness", s.brightness);
    return s;
}

bool LightingSettings::apply(KeyboardController& controller) const {
    switch(kind) {
        case Solid:   return controller.applySolid(color, brightness);
        case Rainbow: return controller.applyRainbow(brightness);
        case Effect:  return controller.applyEffect(effectMode, speed, direction, color, brightness);
    }
    return false;
}
