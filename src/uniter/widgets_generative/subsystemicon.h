#ifndef SUBSYSTEMICON_H
#define SUBSYSTEMICON_H

#include <QMouseEvent>
#include <QWidget>
#include <QString>
#include <QLabel>

namespace uniter::genwdg {

class SubsystemIcon : public QWidget {
    Q_OBJECT

public:
    explicit SubsystemIcon(int index, QWidget* parent = nullptr);
    void setName(const QString& name_);

signals:
    void clicked(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    int index;
    QLabel* nameLabel;
};

} // genwdg




#endif // SUBSYSTEMICON_H
