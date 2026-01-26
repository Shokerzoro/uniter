#ifndef SUBSYSTEMICON_H
#define SUBSYSTEMICON_H

#include <QMouseEvent>
#include <QWidget>

namespace uniter::genwdg {

class SubsystemIcon : public QWidget {
    Q_OBJECT

public:
    explicit SubsystemIcon(int index, QWidget* parent = nullptr);

signals:
    void clicked(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    int index;
};

} // genwdg



#endif // SUBSYSTEMICON_H
