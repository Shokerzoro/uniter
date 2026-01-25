#ifndef WORKWIDGET_H
#define WORKWIDGET_H

#include <QWidget>

namespace uniter::staticwdg {



class WorkWdg : public QWidget
{
    Q_OBJECT
public:
    explicit WorkWdg(QWidget *parent = nullptr);

signals:
};



} // uniter::staticwdg

#endif // WORKWIDGET_H
