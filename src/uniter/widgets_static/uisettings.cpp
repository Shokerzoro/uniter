#include "uisettings.h"
#include "appfuncs.h"
#include <QApplication>
#include <QScreen>

namespace uniter::staticwdg {

UISettings* UISettings::instance() {
    static UISettings s_instance;
    return &s_instance;
}

UISettings::UISettings() {
    auto env = common::appfuncs::get_env_data();
    settings = std::make_unique<QSettings>(env.appname, "Application");
}

void UISettings::applyMainWinSettings(QWidget* window) {
    if (!window || !settings) return;

    window->setMinimumSize(MainWinConstr.minSize);
    window->setMaximumSize(MainWinConstr.maxSize);

    QPoint pos = settings->value("MainWindow/pos", QPoint(100, 100)).toPoint();
    QSize size = settings->value("MainWindow/size", MainWinConstr.minSize).toSize();
    bool isMaximized = settings->value("MainWindow/maximized", false).toBool();

    // Валидация прямо здесь
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();

    if (!screenRect.contains(pos)) {
        pos = screenRect.center();
        pos.setX(pos.x() - size.width() / 2);
        pos.setY(pos.y() - size.height() / 2);
    }

    if (size.width() < MainWinConstr.minSize.width()) {
        size.setWidth(MainWinConstr.minSize.width());
    }
    if (size.height() < MainWinConstr.minSize.height()) {
        size.setHeight(MainWinConstr.minSize.height());
    }

    window->move(pos);
    window->resize(size);

    if (isMaximized) {
        window->showMaximized();
    }
}

void UISettings::saveMainWinState(QWidget* window) {
    if (!window || !settings) return;

    if (!window->isMaximized()) {
        settings->setValue("MainWindow/pos", window->pos());
        settings->setValue("MainWindow/size", window->size());
    }
    settings->setValue("MainWindow/maximized", window->isMaximized());
    settings->sync();
}

} // uniter::staticwdg
