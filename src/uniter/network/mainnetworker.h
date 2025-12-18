#ifndef MAINNETWORKER_H
#define MAINNETWORKER_H



#include <QObject>
#include <QThread>

namespace uniter {

class MainNetWorker : public QObject
{
    Q_OBJECT
private:

public:
    MainNetWorker(QObject *parent = nullptr);

    // Настройки
    void MoveChildenToThread(QThread * NetThread);
};

} //namespace uniter

#endif // MAINNETWORKER_H
