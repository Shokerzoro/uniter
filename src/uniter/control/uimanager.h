#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "uiconstraints.h"
#include <QSettings>
#include <QPoint>
#include <QWidget>
#include <memory>

namespace uniter::control {

class UIManager {
public:
    static UIManager* instance();
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    UIManager(UIManager&&) = delete;
    UIManager& operator=(UIManager&&) = delete;

    // Managing the Main Window
    void applyMainWinSettings(QWidget* window);
    void saveMainWinState(QWidget* window);

    // Manage workbar, workwdg and widgets
    void applyWorkBarSettings(QWidget* bar);
    void applyWorkWdgSettings(QWidget* workWdg);
    void applySubsIconSettings(QWidget* icon);
    void applyGenerativeTabSettings(QWidget* tab);

    // Getters for constraints (one per widget)
    const WorkBarConstraints& getWorkBarConstraints() const { return workBarConstr; }
    const SubsystemIconConstraints& getSubsIconConstraints() const { return subsIconConstr; }
    const GenerativeTabConstraints& getGenerativeTabConstraints() const { return genTabConstr; }

private:
    UIManager();
    ~UIManager() = default;

    std::unique_ptr<QSettings> settings;

    // Constraints for all widgets
    MainWindowConstraints mainWinConstr;
    WorkBarConstraints workBarConstr;
    SubsystemIconConstraints subsIconConstr;
    GenerativeTabConstraints genTabConstr;
};

} // namespace uniter::control

#endif // UIMANAGER_H
