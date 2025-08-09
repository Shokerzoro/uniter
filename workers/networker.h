#ifndef NETWORKER_H
#define NETWORKER_H

#include <QObject>

class NetWorker : public QObject
{
    Q_OBJECT
public:
    explicit NetWorker(QObject *parent = nullptr);

signals:
};

#endif // NETWORKER_H
