#ifndef UISETTINGS_H
#define UISETTINGS_H

#include <QSettings>
#include <QSize>
#include <QPoint>
#include <QWidget>
#include <memory>

namespace uniter::staticwdg {

class UISettings {
public:
    static UISettings* instance();

    // Удаляем конструкторы
    UISettings(const UISettings&) = delete;
    UISettings(UISettings&&) = delete;
    UISettings& operator=(const UISettings&) = delete;
    UISettings& operator=(UISettings&&) = delete;

    // Управление главным окном
    void applyMainWinSettings(QWidget* window);
    void saveMainWinState(QWidget* window);

private:
    UISettings();
    ~UISettings() = default;

    std::unique_ptr<QSettings> settings;

    struct {
        QSize minSize = QSize(800, 500);
        QSize maxSize = QSize(1920, 1440);
    } MainWinConstr;
};

} // uniter::staticwdg

#endif
