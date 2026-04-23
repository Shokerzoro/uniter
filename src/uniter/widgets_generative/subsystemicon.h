#ifndef SUBSYSTEMICON_H
#define SUBSYSTEMICON_H

#include "../contract/unitermessage.h"
#include <QMouseEvent>
#include <QWidget>
#include <QString>
#include <QLabel>

namespace uniter::genwdg {

class SubsystemIcon : public QWidget {
    Q_OBJECT

public:
    explicit SubsystemIcon(contract::Subsystem subsystem,
                           contract::GenSubsystemType genType,
                           uint64_t genId,
                           int index,
                           QWidget* parent = nullptr);
signals:
    void clicked(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // Для коллбека и переключения
    int index;

    // Визуалка
    QLabel* nameLabel;
    bool isHovered = false;
};

} // namespace uniter::genwdg




#endif // SUBSYSTEMICON_H
