#ifndef WORKAREA_H
#define WORKAREA_H

#include <QWidget>

namespace uniter::staticwdg {



class WorkArea : public QWidget
{
    Q_OBJECT
public:
    explicit WorkArea(QWidget *parent = nullptr);

signals:
};



} // uniter::staticwdg

#endif // WORKAREA_H
