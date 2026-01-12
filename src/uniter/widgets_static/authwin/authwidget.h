#ifndef AUTHWDG_H
#define AUTHWDG_H

#include <QWidget>

namespace uniter::staticwdg {



class AuthWdg : public QWidget
{
    Q_OBJECT
public:
    explicit AuthWdg(QWidget *parent = nullptr);

public slots:

signals:
};


} //uniter::staticwdg

#endif // AUTHWDG_H
