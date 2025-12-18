#ifndef INTERNALMESSAGE_H
#define INTERNALMESSAGE_H

#include <QObject>

namespace uniter {


class InternalMessage : public QObject
{
    Q_OBJECT
public:
    explicit InternalMessage(QObject *parent = nullptr);

signals:
};

} // namespace uniter

#endif // INTERNALMESSAGE_H
