#include "keyboardcontroller.h"

#include "core/keymap.h"

#include <QTimer>
#include <KLocalizedString>

using krgb::AW410KDevice;
using krgb::Mode;
using krgb::Speed;
using krgb::Direction;
using krgb::ColorMode;

namespace {

QColor scaled(const QColor& c, int pct) {
    pct = qBound(0, pct, 100);
    return QColor(c.red() * pct / 100, c.green() * pct / 100, c.blue() * pct / 100);
}

} // namespace

KeyboardController::KeyboardController(QObject* parent)
    : QObject(parent) {
    pollTimer_ = new QTimer(this);
    pollTimer_->setInterval(2000);
    connect(pollTimer_, &QTimer::timeout, this, &KeyboardController::refresh);
    pollTimer_->start();
    refresh();
}

KeyboardController::~KeyboardController() = default;

void KeyboardController::refresh() {
    const std::string p = AW410KDevice::findDevicePath();
    const QString qp = QString::fromStdString(p);
    const bool nowConnected = !p.empty();

    if(nowConnected != connected_ || qp != path_) {
        connected_ = nowConnected;
        path_ = qp;
        if(!connected_ && device_.isOpen()) {
            device_.close();
        }
        Q_EMIT connectionChanged(connected_, path_);
    }
}

bool KeyboardController::ensureOpen() {
    if(device_.isOpen()) {
        return true;
    }
    std::string err;
    if(!device_.open(&err)) {
        Q_EMIT error(QString::fromStdString(err));
        return false;
    }
    return true;
}

bool KeyboardController::applySolid(const QColor& color, int brightnessPct) {
    if(!ensureOpen()) {
        return false;
    }
    const QColor c = scaled(color, brightnessPct);
    if(!device_.setSolid(c.red(), c.green(), c.blue())) {
        Q_EMIT error(i18n("Failed to set solid colour."));
        return false;
    }
    return true;
}

bool KeyboardController::applyRainbow(int brightnessPct) {
    if(!ensureOpen()) {
        return false;
    }
    const qreal v = qBound(0, brightnessPct, 100) / 100.0;
    std::vector<krgb::KeyColor> keys;
    keys.reserve(krgb::kKeyCount);
    for(std::size_t i = 0; i < krgb::kKeyCount; ++i) {
        const qreal hue = static_cast<qreal>(i) / krgb::kKeyCount;
        const QColor c = QColor::fromHsvF(hue, 1.0, v);
        keys.push_back({krgb::kKeyMap[i].idx,
                        static_cast<std::uint8_t>(c.red()),
                        static_cast<std::uint8_t>(c.green()),
                        static_cast<std::uint8_t>(c.blue())});
    }
    if(!device_.setPerKey(keys)) {
        Q_EMIT error(i18n("Failed to set rainbow."));
        return false;
    }
    return true;
}

bool KeyboardController::applyEffect(int modeValue, int speedValue, int directionValue,
                                     const QColor& color, int brightnessPct) {
    if(!ensureOpen()) {
        return false;
    }
    const auto mode = static_cast<Mode>(modeValue);
    ColorMode cm = ColorMode::Single;
    if(mode == Mode::Spectrum || mode == Mode::RainbowWave) {
        cm = ColorMode::Rainbow;
    }
    const QColor c = scaled(color, brightnessPct);
    const bool ok = device_.setEffect(mode,
                                      static_cast<Speed>(speedValue),
                                      static_cast<Direction>(directionValue),
                                      cm, c.red(), c.green(), c.blue());
    if(!ok) {
        Q_EMIT error(i18n("Failed to set effect."));
        return false;
    }
    return true;
}

bool KeyboardController::applyOff() {
    if(!ensureOpen()) {
        return false;
    }
    if(!device_.setOff()) {
        Q_EMIT error(i18n("Failed to turn lighting off."));
        return false;
    }
    return true;
}
