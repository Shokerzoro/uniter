#ifndef OFFLINEWDG_H
#define OFFLINEWDG_H

#include <QWidget>

namespace uniter::staticwdg {



class OfflineWdg : public QWidget
{
    Q_OBJECT
public:
    explicit OfflineWdg(QWidget *parent = nullptr);

signals:
};


} //uniter::staticwdg

#endif // OFFLINEWDG_H
