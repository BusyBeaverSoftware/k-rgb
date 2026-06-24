#include "mainwindow.h"

#include "keyboardcontroller.h"
#include "core/aw410k_device.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include <KColorButton>
#include <KLocalizedString>
#include <KStatusNotifierItem>

using krgb::Mode;
using krgb::Speed;
using krgb::Direction;

MainWindow::MainWindow(KeyboardController* controller, QWidget* parent)
    : KMainWindow(parent), controller_(controller) {
    setWindowTitle(i18n("k-rgb — Alienware AW410K"));
    populateModes();
    buildUi();
    setupTray();
    loadSettings();
    resize(440, 320);
    setAutoSaveSettings();

    connect(controller_, &KeyboardController::connectionChanged,
            this, &MainWindow::onConnectionChanged);
    connect(controller_, &KeyboardController::error,
            this, &MainWindow::onError);

    onConnectionChanged(controller_->isConnected(), controller_->devicePath());
    onModeChanged();
}

void MainWindow::populateModes() {
    //          name                       value                          solid  rnbw   color  speed  dir
    modes_ = {
        { i18n("Solid colour"),     static_cast<int>(Mode::Direct),      true,  false, true,  false, false },
        { i18n("Rainbow (static)"), 0,                                   false, true,  false, false, false },
        { i18n("Static"),           static_cast<int>(Mode::Static),      false, false, true,  false, false },
        { i18n("Breathing"),        static_cast<int>(Mode::Breathing),   false, false, true,  true,  false },
        { i18n("Pulse"),            static_cast<int>(Mode::Pulse),       false, false, true,  true,  false },
        { i18n("Spectrum"),         static_cast<int>(Mode::Spectrum),    false, false, false, true,  false },
        { i18n("Single wave"),      static_cast<int>(Mode::SingleWave),  false, false, true,  true,  true  },
        { i18n("Rainbow wave"),     static_cast<int>(Mode::RainbowWave), false, false, false, true,  true  },
        { i18n("Scanner"),          static_cast<int>(Mode::Scanner),     false, false, true,  true,  false },
    };
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* outer = new QVBoxLayout(central);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);

    modeCombo_ = new QComboBox(central);
    for(const ModeEntry& m : modes_) {
        modeCombo_->addItem(m.name);
    }
    form->addRow(i18n("Mode:"), modeCombo_);

    colorButton_ = new KColorButton(QColor(0, 170, 255), central);
    form->addRow(i18n("Colour:"), colorButton_);

    speedCombo_ = new QComboBox(central);
    speedCombo_->addItem(i18n("Slow"),   static_cast<int>(Speed::Slowest));
    speedCombo_->addItem(i18n("Normal"), static_cast<int>(Speed::Normal));
    speedCombo_->addItem(i18n("Fast"),   static_cast<int>(Speed::Fastest));
    speedCombo_->setCurrentIndex(1);
    form->addRow(i18n("Speed:"), speedCombo_);

    directionCombo_ = new QComboBox(central);
    directionCombo_->addItem(i18n("Left"),  static_cast<int>(Direction::Left));
    directionCombo_->addItem(i18n("Right"), static_cast<int>(Direction::Right));
    directionCombo_->addItem(i18n("Up"),    static_cast<int>(Direction::Up));
    directionCombo_->addItem(i18n("Down"),  static_cast<int>(Direction::Down));
    form->addRow(i18n("Direction:"), directionCombo_);

    auto* brightnessRow = new QHBoxLayout();
    brightnessSlider_ = new QSlider(Qt::Horizontal, central);
    brightnessSlider_->setRange(0, 100);
    brightnessSlider_->setValue(100);
    brightnessValue_ = new QLabel(QStringLiteral("100%"), central);
    brightnessValue_->setMinimumWidth(40);
    brightnessRow->addWidget(brightnessSlider_);
    brightnessRow->addWidget(brightnessValue_);
    form->addRow(i18n("Brightness:"), brightnessRow);

    outer->addLayout(form);
    outer->addStretch();

    autostartCheck_ = new QCheckBox(i18n("Restore lighting at login"), central);
    autostartCheck_->setChecked(QFile::exists(autostartFilePath()));
    outer->addWidget(autostartCheck_);

    auto* buttons = new QHBoxLayout();
    offButton_ = new QPushButton(QIcon::fromTheme(QStringLiteral("system-shutdown")),
                                 i18n("Turn Off"), central);
    applyButton_ = new QPushButton(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")),
                                   i18n("Apply"), central);
    applyButton_->setDefault(true);
    buttons->addWidget(offButton_);
    buttons->addStretch();
    buttons->addWidget(applyButton_);
    outer->addLayout(buttons);

    setCentralWidget(central);

    // Connection status lives compactly in the status bar.
    statusLabel_ = new QLabel(this);
    statusLabel_->setTextFormat(Qt::RichText);
    statusBar()->addPermanentWidget(statusLabel_);

    connect(modeCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::onModeChanged);
    connect(brightnessSlider_, &QSlider::valueChanged, this, &MainWindow::onBrightnessChanged);
    connect(applyButton_, &QPushButton::clicked, this, &MainWindow::onApply);
    connect(offButton_, &QPushButton::clicked, this, &MainWindow::onOff);
    connect(autostartCheck_, &QCheckBox::toggled, this, &MainWindow::onAutostartToggled);

    // Live feedback: re-apply when the colour changes or the brightness slider
    // is released, so the keyboard tracks the controls without hitting Apply.
    connect(colorButton_, &KColorButton::changed, this, [this]() {
        if(!loading_ && controller_->isConnected()) {
            onApply();
        }
    });
    connect(brightnessSlider_, &QSlider::sliderReleased, this, [this]() {
        if(!loading_ && controller_->isConnected()) {
            onApply();
        }
    });
}

void MainWindow::setupTray() {
    tray_ = new KStatusNotifierItem(QStringLiteral("krgb"), this);
    tray_->setTitle(i18n("k-rgb"));
    tray_->setIconByName(QStringLiteral("input-keyboard"));
    tray_->setToolTip(QStringLiteral("input-keyboard"), i18n("k-rgb"),
                      i18n("Alienware AW410K lighting"));
    tray_->setCategory(KStatusNotifierItem::ApplicationStatus);
    tray_->setStatus(KStatusNotifierItem::Active);
    tray_->setStandardActionsEnabled(false);

    // Left-click toggles the window.
    connect(tray_, &KStatusNotifierItem::activateRequested, this, [this](bool, const QPoint&) {
        if(isVisible() && !isMinimized()) {
            hide();
        } else {
            show();
            setWindowState(windowState() & ~Qt::WindowMinimized);
            raise();
            activateWindow();
        }
    });

    // Right-click: quick lighting controls.
    auto* menu = new QMenu(this);
    menu->addSection(i18n("Quick lighting"));

    QAction* offAction = menu->addAction(QIcon::fromTheme(QStringLiteral("system-shutdown")), i18n("Off"));
    connect(offAction, &QAction::triggered, this, [this] { controller_->applyOff(); });

    QAction* rainbowAction = menu->addAction(i18n("Rainbow (static)"));
    connect(rainbowAction, &QAction::triggered, this,
            [this] { controller_->applyRainbow(brightnessSlider_->value()); });

    QAction* spectrumAction = menu->addAction(i18n("Spectrum"));
    connect(spectrumAction, &QAction::triggered, this, [this] {
        controller_->applyEffect(static_cast<int>(Mode::Spectrum),
                                 static_cast<int>(Speed::Normal),
                                 static_cast<int>(Direction::Left),
                                 Qt::white, brightnessSlider_->value());
    });

    menu->addSection(i18n("Solid colour"));
    const struct { QString name; QColor color; } presets[] = {
        { i18n("Red"),        QColor(255, 0, 0)     },
        { i18n("Green"),      QColor(0, 255, 0)     },
        { i18n("Blue"),       QColor(0, 90, 255)    },
        { i18n("Warm white"), QColor(255, 160, 70)  },
        { i18n("White"),      QColor(255, 255, 255) },
    };
    for(const auto& p : presets) {
        const QColor c = p.color;
        QAction* a = menu->addAction(p.name);
        connect(a, &QAction::triggered, this,
                [this, c] { controller_->applySolid(c, brightnessSlider_->value()); });
    }

    menu->addSeparator();
    QAction* showAction = menu->addAction(QIcon::fromTheme(QStringLiteral("settings-configure")),
                                          i18n("Open k-rgb…"));
    connect(showAction, &QAction::triggered, this, [this] {
        show();
        raise();
        activateWindow();
    });
    QAction* quitAction = menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    tray_->setContextMenu(menu);
}

void MainWindow::loadSettings() {
    loading_ = true;
    const LightingSettings s = LightingSettings::load();

    int idx = 0;
    for(int i = 0; i < modes_.size(); ++i) {
        const ModeEntry& m = modes_.at(i);
        if(s.kind == LightingSettings::Solid && m.solid) { idx = i; break; }
        if(s.kind == LightingSettings::Rainbow && m.rainbow) { idx = i; break; }
        if(s.kind == LightingSettings::Effect && !m.solid && !m.rainbow && m.value == s.effectMode) {
            idx = i;
            break;
        }
    }
    modeCombo_->setCurrentIndex(idx);
    colorButton_->setColor(s.color);
    const int si = speedCombo_->findData(s.speed);
    if(si >= 0) {
        speedCombo_->setCurrentIndex(si);
    }
    const int di = directionCombo_->findData(s.direction);
    if(di >= 0) {
        directionCombo_->setCurrentIndex(di);
    }
    brightnessSlider_->setValue(s.brightness);
    brightnessValue_->setText(QStringLiteral("%1%").arg(s.brightness));

    loading_ = false;
}

LightingSettings MainWindow::currentSettings() const {
    LightingSettings s;
    const ModeEntry& m = modes_.at(modeCombo_->currentIndex());
    if(m.solid) {
        s.kind = LightingSettings::Solid;
    } else if(m.rainbow) {
        s.kind = LightingSettings::Rainbow;
    } else {
        s.kind = LightingSettings::Effect;
        s.effectMode = m.value;
    }
    s.color = colorButton_->color();
    s.speed = speedCombo_->currentData().toInt();
    s.direction = directionCombo_->currentData().toInt();
    s.brightness = brightnessSlider_->value();
    return s;
}

void MainWindow::onModeChanged() {
    const int idx = modeCombo_->currentIndex();
    if(idx < 0 || idx >= modes_.size()) {
        return;
    }
    const ModeEntry& m = modes_.at(idx);
    colorButton_->setEnabled(m.usesColor);
    speedCombo_->setEnabled(m.usesSpeed);
    directionCombo_->setEnabled(m.usesDirection);
}

void MainWindow::onBrightnessChanged(int value) {
    brightnessValue_->setText(QStringLiteral("%1%").arg(value));
}

void MainWindow::onApply() {
    const int idx = modeCombo_->currentIndex();
    if(idx < 0 || idx >= modes_.size()) {
        return;
    }
    const LightingSettings s = currentSettings();
    s.apply(*controller_);
    s.save();
}

void MainWindow::onOff() {
    controller_->applyOff();
}

void MainWindow::onAutostartToggled(bool checked) {
    const QString path = autostartFilePath();
    if(checked) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        const QString exec = QCoreApplication::applicationFilePath() + QStringLiteral(" --apply");
        QFile f(path);
        if(f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "[Desktop Entry]\n"
               << "Type=Application\n"
               << "Name=k-rgb (restore lighting)\n"
               << "Exec=" << exec << "\n"
               << "Icon=input-keyboard\n"
               << "Terminal=false\n"
               << "NoDisplay=true\n"
               << "X-GNOME-Autostart-enabled=true\n";
        } else {
            onError(i18n("Could not write autostart entry to %1", path));
        }
    } else {
        QFile::remove(path);
    }
}

QString MainWindow::autostartFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
           QStringLiteral("/autostart/krgb-restore.desktop");
}

void MainWindow::onConnectionChanged(bool connected, const QString& path) {
    if(connected) {
        statusLabel_->setText(i18n("<span style='color:#27ae60'>●</span> %1",
                                   path.isEmpty() ? i18n("AW410K") : path));
        statusLabel_->setToolTip(i18n("Connected to Alienware AW410K"));
    } else {
        statusLabel_->setText(i18n("<span style='color:#c0392b'>●</span> Not found"));
        statusLabel_->setToolTip(
            i18n("AW410K not found — check it's plugged in and the udev rule is installed."));
    }

    modeCombo_->setEnabled(connected);
    applyButton_->setEnabled(connected);
    offButton_->setEnabled(connected);
    if(connected) {
        onModeChanged();
    } else {
        colorButton_->setEnabled(false);
        speedCombo_->setEnabled(false);
        directionCombo_->setEnabled(false);
    }
}

void MainWindow::onError(const QString& message) {
    statusBar()->showMessage(i18n("Error: %1", message), 5000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Closing hides to the system tray; quit via the tray menu (or Ctrl+Q).
    if(QSystemTrayIcon::isSystemTrayAvailable()) {
        hide();
        event->ignore();
    } else {
        event->accept();
        qApp->quit();
    }
}
