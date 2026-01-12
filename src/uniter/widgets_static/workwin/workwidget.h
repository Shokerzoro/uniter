#ifndef WORKWIDGET_H
#define WORKWIDGET_H

#include <QWidget>

namespace uniter::staticwdg {



class WorkWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WorkWidget(QWidget *parent = nullptr);

signals:
};



} // uniter::staticwdg

#endif // WORKWIDGET_H
