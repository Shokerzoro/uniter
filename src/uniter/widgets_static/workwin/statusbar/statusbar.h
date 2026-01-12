#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>

namespace uniter::staticwdg {



class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);

signals:
};



} // uniter::staticwdg


#endif // STATUSBAR_H
