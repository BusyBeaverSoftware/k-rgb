#pragma once

#include <KMainWindow>
#include <QVector>

#include "settings.h"

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class KColorButton;
class KStatusNotifierItem;
class KeyboardController;

class MainWindow : public KMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(KeyboardController* controller, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
    void onApply();
    void onOff();
    void onModeChanged();
    void onBrightnessChanged(int value);
    void onAutostartToggled(bool checked);
    void onConnectionChanged(bool connected, const QString& path);
    void onError(const QString& message);

private:
    struct ModeEntry {
        QString name;
        int     value;          // krgb::Mode value
        bool    solid;          // route through applySolid()
        bool    rainbow;        // route through applyRainbow() (per-key static rainbow)
        bool    usesColor;
        bool    usesSpeed;
        bool    usesDirection;
    };

    void             buildUi();
    void             setupTray();
    void             populateModes();
    void             loadSettings();
    LightingSettings currentSettings() const;
    QString          autostartFilePath() const;

    KeyboardController* controller_;
    QVector<ModeEntry>  modes_;
    bool                loading_ = false;

    QLabel*       statusLabel_      = nullptr;
    KColorButton* colorButton_      = nullptr;
    QComboBox*    modeCombo_        = nullptr;
    QComboBox*    speedCombo_       = nullptr;
    QComboBox*    directionCombo_   = nullptr;
    QSlider*      brightnessSlider_ = nullptr;
    QLabel*       brightnessValue_  = nullptr;
    QCheckBox*    autostartCheck_   = nullptr;
    QPushButton*  applyButton_      = nullptr;
    QPushButton*  offButton_        = nullptr;

    KStatusNotifierItem* tray_      = nullptr;
};
