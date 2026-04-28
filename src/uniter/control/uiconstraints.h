#ifndef UICONSTRAINTS_H
#define UICONSTRAINTS_H

#include <QSize>
#include <QColor>

namespace uniter::control {

// Constraints for the main window
struct MainWindowConstraints {
    QSize minSize = QSize(800, 500);
    QSize maxSize = QSize(1920, 1440);
};

// Constraints for WorkBar
struct WorkBarConstraints {
    int width = 75;
    int borderWidth = 1;
    QColor borderColor = QColor(119, 124, 124, 77);
};

// Constraints for SubsystemIcon
struct SubsystemIconConstraints {
    int size = 70;
    QColor defaultBgColor = QColor(0, 0, 0, 0);
    QColor hoverBgColor = QColor(255, 255, 255, 26);
    QColor textColor = QColor(200, 200, 200);
    int fontSize = 13;
};

// Constraints for GenerativeTab (ISubsWdg)
struct GenerativeTabConstraints {
    QColor bgColor = QColor(0, 0, 0, 0);
    QColor textColor = QColor(200, 200, 200);
    int fontSize = 13;
};

} // namespace uniter::control

#endif // UICONSTRAINTS_H
