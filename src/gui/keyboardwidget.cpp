#include "keyboardwidget.h"

#include "core/keymap.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace {
constexpr double kMargin   = 6.0;   // px around the whole layout
constexpr double kKeyInset = 2.0;   // px gap between adjacent keys
const QColor     kUnset(45, 45, 48);
const QColor     kBackground(28, 28, 30);
const QColor     kSelect(80, 170, 255);

// Friendlier glyphs for a few keys whose names are verbose.
QString labelFor(const QString& name) {
    static const QHash<QString, QString> pretty = {
        {QStringLiteral("BACKSPACE"), QStringLiteral("⌫")},
        {QStringLiteral("ENTER"), QStringLiteral("↵")},
        {QStringLiteral("NPENTER"), QStringLiteral("↵")},
        {QStringLiteral("LSHIFT"), QStringLiteral("⇧")},
        {QStringLiteral("RSHIFT"), QStringLiteral("⇧")},
        {QStringLiteral("CAPS"), QStringLiteral("Caps")},
        {QStringLiteral("TAB"), QStringLiteral("Tab")},
        {QStringLiteral("MINUS"), QStringLiteral("-")},
        {QStringLiteral("EQUALS"), QStringLiteral("=")},
        {QStringLiteral("NUMLK"), QStringLiteral("Num")},
        {QStringLiteral("SCRLK"), QStringLiteral("Scrl")},
        {QStringLiteral("PRTSC"), QStringLiteral("Prt")},
        {QStringLiteral("PAUSE"), QStringLiteral("Pause")},
        {QStringLiteral("UP"), QStringLiteral("↑")},
        {QStringLiteral("DOWN"), QStringLiteral("↓")},
        {QStringLiteral("LEFT"), QStringLiteral("←")},
        {QStringLiteral("RIGHT"), QStringLiteral("→")},
        {QStringLiteral("LWIN"), QStringLiteral("❖")},
        {QStringLiteral("MUTE"), QStringLiteral("\U0001f507")},
        {QStringLiteral("VOLDN"), QStringLiteral("\U0001f509")},
        {QStringLiteral("VOLUP"), QStringLiteral("\U0001f50a")},
    };
    const auto it = pretty.constFind(name);
    if(it != pretty.constEnd()) {
        return it.value();
    }
    if(name.startsWith(QStringLiteral("NP"))) {
        return name.mid(2);  // numpad: show just the glyph
    }
    return name;
}
} // namespace

KeyboardWidget::KeyboardWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(false);
    setMinimumSize(480, 150);
    setFocusPolicy(Qt::ClickFocus);
}

int KeyboardWidget::heightForWidth(int w) const {
    const double inner = w - 2 * kMargin;
    return static_cast<int>(inner * krgb::kLayoutHeight / krgb::kLayoutWidth + 2 * kMargin);
}

void KeyboardWidget::setKeyColors(const QHash<QString, QColor>& colors) {
    colors_ = colors;
    update();
}

void KeyboardWidget::setModelBit(quint8 bit) {
    if(bit == modelBit_) {
        return;
    }
    modelBit_ = bit;
    selected_.clear();
    update();
    Q_EMIT selectionChanged(0);
}

QStringList KeyboardWidget::selectedKeys() const {
    QStringList list(selected_.cbegin(), selected_.cend());
    list.sort();
    return list;
}

void KeyboardWidget::recomputeLayout() {
    rects_.clear();
    rects_.reserve(static_cast<int>(krgb::kKeyCount));

    const double availW = width() - 2 * kMargin;
    const double availH = height() - 2 * kMargin;
    const double unit = qMin(availW / krgb::kLayoutWidth, availH / krgb::kLayoutHeight);

    // Centre the layout within the widget.
    const double originX = (width() - unit * krgb::kLayoutWidth) / 2.0;
    const double originY = (height() - unit * krgb::kLayoutHeight) / 2.0;

    for(std::size_t i = 0; i < krgb::kKeyCount; ++i) {
        const krgb::KeyDef& k = krgb::kKeyMap[i];
        if(!(k.models & modelBit_)) {
            continue;  // key absent on the active model
        }
        const QRectF cell(originX + k.x * unit + kKeyInset / 2.0,
                          originY + k.y * unit + kKeyInset / 2.0,
                          k.w * unit - kKeyInset,
                          k.h * unit - kKeyInset);
        rects_.push_back({QString::fromLatin1(k.name), cell});
    }
}

const QString KeyboardWidget::keyAt(const QPointF& p) const {
    for(const KeyRect& r : rects_) {
        if(r.cell.contains(p)) {
            return r.name;
        }
    }
    return QString();
}

void KeyboardWidget::paintEvent(QPaintEvent*) {
    recomputeLayout();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), kBackground);

    QFont font = painter.font();
    font.setPointSizeF(qMax(6.0, font.pointSizeF() - 1.0));
    painter.setFont(font);

    for(const KeyRect& r : rects_) {
        const QColor fill = colors_.value(r.name, kUnset);
        const bool sel = selected_.contains(r.name);

        painter.setBrush(fill);
        painter.setPen(QPen(sel ? kSelect : QColor(0, 0, 0, 160), sel ? 2.0 : 1.0));
        painter.drawRoundedRect(r.cell, 3.0, 3.0);

        // Label colour contrasts with the key fill.
        const double lum = 0.299 * fill.red() + 0.587 * fill.green() + 0.114 * fill.blue();
        painter.setPen(lum > 140 ? QColor(20, 20, 20) : QColor(225, 225, 225));
        painter.drawText(r.cell, Qt::AlignCenter, labelFor(r.name));
    }
}

void KeyboardWidget::mousePressEvent(QMouseEvent* ev) {
    if(ev->button() != Qt::LeftButton) {
        return;
    }
    const bool additive = ev->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    pressPos_  = ev->position();
    dragging_  = true;
    moved_     = false;

    const QString hit = keyAt(pressPos_);
    if(!additive) {
        selected_.clear();
        if(!hit.isEmpty()) {
            selected_.insert(hit);
        }
    } else if(!hit.isEmpty()) {
        if(selected_.contains(hit)) {
            selected_.remove(hit);
        } else {
            selected_.insert(hit);
        }
    }
    baseSelection_ = selected_;
    update();
    Q_EMIT selectionChanged(selected_.size());
}

void KeyboardWidget::mouseMoveEvent(QMouseEvent* ev) {
    if(!dragging_) {
        return;
    }
    const QPointF p = ev->position();
    if(!moved_ && (p - pressPos_).manhattanLength() < 4) {
        return;
    }
    moved_  = true;
    rubber_ = QRectF(pressPos_, p).normalized();

    selected_ = baseSelection_;
    for(const KeyRect& r : rects_) {
        if(rubber_.intersects(r.cell)) {
            selected_.insert(r.name);
        }
    }
    update();
    Q_EMIT selectionChanged(selected_.size());
}

void KeyboardWidget::mouseReleaseEvent(QMouseEvent* ev) {
    if(ev->button() != Qt::LeftButton) {
        return;
    }
    dragging_ = false;
    rubber_   = QRectF();
    update();
}

void KeyboardWidget::paintSelection(const QColor& color) {
    if(selected_.isEmpty() || !color.isValid()) {
        return;
    }
    for(const QString& name : selected_) {
        colors_.insert(name, color);
    }
    update();
    Q_EMIT changed();
}

void KeyboardWidget::clearSelection() {
    if(selected_.isEmpty()) {
        return;
    }
    for(const QString& name : selected_) {
        colors_.remove(name);  // unassigned == off
    }
    update();
    Q_EMIT changed();
}

void KeyboardWidget::selectAll() {
    selected_.clear();
    for(std::size_t i = 0; i < krgb::kKeyCount; ++i) {
        if(krgb::kKeyMap[i].models & modelBit_) {
            selected_.insert(QString::fromLatin1(krgb::kKeyMap[i].name));
        }
    }
    update();
    Q_EMIT selectionChanged(selected_.size());
}

void KeyboardWidget::fillAll(const QColor& color) {
    if(!color.isValid()) {
        return;
    }
    for(std::size_t i = 0; i < krgb::kKeyCount; ++i) {
        if(krgb::kKeyMap[i].models & modelBit_) {
            colors_.insert(QString::fromLatin1(krgb::kKeyMap[i].name), color);
        }
    }
    update();
    Q_EMIT changed();
}
