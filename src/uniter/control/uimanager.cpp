#include "uimanager.h"
#include "appfuncs.h"
#include <QApplication>
#include <QScreen>
#include <QLayout>
#include <QLabel>
#include <QPainter>

namespace uniter::control {

UIManager* UIManager::instance() {
    static UIManager s_instance;
    return &s_instance;
}

UIManager::UIManager() {
    auto env = common::appfuncs::get_env_data();
    settings = std::make_unique<QSettings>(env.appname, "Application");
}

// Управление главным окном

void UIManager::applyMainWinSettings(QWidget* window) {
    if (!window || !settings) return;

    window->setMinimumSize(mainWinConstr.minSize);
    window->setMaximumSize(mainWinConstr.maxSize);

    QPoint pos = settings->value("MainWindow/pos", QPoint(100, 100)).toPoint();
    QSize size = settings->value("MainWindow/size", mainWinConstr.minSize).toSize();
    bool isMaximized = settings->value("MainWindow/maximized", false).toBool();

    // Валидация
    QScreen* screen = QApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();

    if (!screenRect.contains(pos)) {
        pos = screenRect.center();
        pos.setX(pos.x() - size.width() / 2);
        pos.setY(pos.y() - size.height() / 2);
    }

    if (size.width() < mainWinConstr.minSize.width()) {
        size.setWidth(mainWinConstr.minSize.width());
    }
    if (size.height() < mainWinConstr.minSize.height()) {
        size.setHeight(mainWinConstr.minSize.height());
    }

    window->move(pos);
    window->resize(size);

    if (isMaximized) {
        window->showMaximized();
    }
}

void UIManager::saveMainWinState(QWidget* window) {
    if (!window || !settings) return;

    if (!window->isMaximized()) {
        settings->setValue("MainWindow/pos", window->pos());
        settings->setValue("MainWindow/size", window->size());
    }
    settings->setValue("MainWindow/maximized", window->isMaximized());
    settings->sync();
}

// Управление workbar и workwdg и виджетов

void UIManager::applyWorkBarSettings(QWidget* bar) {
    if (!bar) return;

    bar->setFixedWidth(workBarConstr.width);
    bar->setMinimumHeight(0);
}

void UIManager::applyWorkWdgSettings(QWidget* workWdg) {
    if (!workWdg) return;

    workWdg->setMinimumSize(QSize(400, 300));

    QPalette palette = workWdg->palette();
    palette.setColor(QPalette::Window, QColor(30, 30, 30));
    workWdg->setPalette(palette);
    workWdg->setAutoFillBackground(true);
}

void UIManager::applySubsIconSettings(QWidget* icon) {
    if (!icon) return;

    // Устанавливаем строго фиксированный размер
    icon->setFixedSize(subsIconConstr.size, subsIconConstr.size);
    icon->setAutoFillBackground(false);

    // Находим QLabel внутри виджета и применяем стиль текста
    QLabel* label = icon->findChild<QLabel*>();
    if (label) {
        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, subsIconConstr.textColor);
        label->setPalette(palette);

        QFont font = label->font();
        font.setPointSize(subsIconConstr.fontSize);
        font.setBold(true);
        label->setFont(font);
    }
}

void UIManager::applyGenerativeTabSettings(QWidget* tab) {
    if (!tab) return;

    tab->setAutoFillBackground(false);

    QPalette palette = tab->palette();
    palette.setColor(QPalette::Window, genTabConstr.bgColor);
    tab->setPalette(palette);

    // Находим QLabel внутри виджета и применяем стиль текста
    QLabel* label = tab->findChild<QLabel*>();
    if (label) {
        QPalette labelPalette = label->palette();
        labelPalette.setColor(QPalette::WindowText, genTabConstr.textColor);
        label->setPalette(labelPalette);

        QFont font = label->font();
        font.setPointSize(genTabConstr.fontSize);
        font.setBold(true);
        label->setFont(font);
    }
}

} // namespace uniter::control
