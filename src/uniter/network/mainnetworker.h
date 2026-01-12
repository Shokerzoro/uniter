#ifndef MAINNETWORKER_H
#define MAINNETWORKER_H



#include <QObject>
#include <QThread>

namespace uniter {

// Выполняет поиск IP, установление соединения
// После чего выплевывает UniterMessage менеджеру приложения
// И отправляет сообщения серверу
// Владеет буфером на каждое сообщение (пока не пришло подтверждение)
// Сохраняет их в файл на случай потери соединения
// Поддерживает очередь сообщений, выдает только по порядку
// Полностью отвечает за управление sequence_id (хранит, запрашивает и т.д.)
// Классы выше не владеют данными по id сообщений, только получают последовательно
// Отвечает за периодический POLL и синхронизацию
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
