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
    const krgb::KeyboardModel* model = nullptr;
    const std::string p = AW410KDevice::findDevice(&model);
    const QString qp = QString::fromStdString(p);
    const bool nowConnected = !p.empty();

    if(nowConnected != connected_ || qp != path_) {
        connected_ = nowConnected;
        path_ = qp;
        modelName_ = model ? QString::fromLatin1(model->name) : QString();
        modelBit_ = model ? model->bit : krgb::kAllModels;
        if(!connected_ && device_.isOpen()) {
            device_.close();
        }
        Q_EMIT connectionChanged(connected_, path_);
    }
}

namespace {
// Bit identifying the keys the currently-open device supports.
std::uint8_t activeBit(const krgb::AW410KDevice& dev) {
    return dev.model() ? dev.model()->bit : krgb::kAllModels;
}
} // namespace

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
    const std::uint8_t bit = activeBit(device_);
    const std::size_t total = krgb::modelKeyCount(bit);
    std::vector<krgb::KeyColor> keys;
    keys.reserve(total);
    std::size_t n = 0;
    for(std::size_t i = 0; i < krgb::kKeyCount; ++i) {
        if(!(krgb::kKeyMap[i].models & bit)) {
            continue;  // key not present on this model
        }
        const qreal hue = total ? static_cast<qreal>(n++) / total : 0.0;
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

bool KeyboardController::applyPerKey(const QHash<QString, QColor>& keyColors, int brightnessPct) {
    if(!ensureOpen()) {
        return false;
    }
    const int pct = qBound(0, brightnessPct, 100);
    const std::uint8_t bit = activeBit(device_);
    std::vector<krgb::KeyColor> keys;
    keys.reserve(krgb::kKeyCount);
    for(std::size_t i = 0; i < krgb::kKeyCount; ++i) {
        if(!(krgb::kKeyMap[i].models & bit)) {
            continue;  // key not present on this model
        }
        const QString name = QString::fromLatin1(krgb::kKeyMap[i].name);
        const QColor c = keyColors.value(name, QColor(0, 0, 0));
        keys.push_back({krgb::kKeyMap[i].idx,
                        static_cast<std::uint8_t>(c.red() * pct / 100),
                        static_cast<std::uint8_t>(c.green() * pct / 100),
                        static_cast<std::uint8_t>(c.blue() * pct / 100)});
    }
    if(!device_.setPerKey(keys)) {
        Q_EMIT error(i18n("Failed to set per-key colours."));
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
