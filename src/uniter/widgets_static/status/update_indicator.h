#ifndef UPDATE_INDICATOR_H
#define UPDATE_INDICATOR_H

#include <QWidget>

#include <QPainter>
#include <QColor>
#include <QPaintEvent>

class UpdateIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateIndicator(QWidget *parent = nullptr);
    enum State { ONLINE, OFFLINE, WARNING };

    //Изменить статус
    void set_state(int new_state);

private:
    int m_state;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor color;
        switch (m_state) {
        case State::ONLINE:  color = Qt::green; break;
        case State::OFFLINE: color = Qt::red; break;
        case State::WARNING: color = Qt::yellow; break;
        }

        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect());
    }
};

#endif // UPDATE_INDICATOR_H
