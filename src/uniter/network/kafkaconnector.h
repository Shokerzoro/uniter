#ifndef KAFKACONNECTOR_H
#define KAFKACONNECTOR_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include "../contract/unitermessage.h"

namespace uniter::net {

/*
 * KafkaConnector — заглушка.
 *
 * Роль в архитектуре (см. docs/fsm_appmanager.md и uniter-target-state):
 *   — в рабочем режиме отвечает за consumer-канал Kafka:
 *       * хранение offset последнего обработанного сообщения очереди
 *         broadcast (в OS Secure Storage, привязанно к хэшу пользователя);
 *       * подписку на топик broadcast после подтверждения актуальности
 *         offset сервером;
 *       * получение сообщений из Kafka и их проброс в приложение.
 *
 *   — в FSM AppManager задействован в двух состояниях:
 *       * KCONNECTOR — AppManager при входе шлёт signalInitKafkaConnector(hash);
 *         KafkaConnector инициализируется (локально) и сразу после этого
 *         присылает сохранённый offset через signalOffsetReady(QString).
 *       * READY     — AppManager шлёт signalSubscribeKafka; KafkaConnector
 *         подписывается на broadcast-топик.
 *
 * Stub-поведение:
 *   — onInitConnection(hash): запоминает hash, через event-loop шлёт
 *     произвольный offset. Этого достаточно для продвижения FSM.
 *   — onSubscribeKafka(): no-op + signalKafkaSubscribed(true) через event-loop.
 *   — onShutdown(): сбрасывает внутреннее состояние.
 *
 * Реальная реализация (librdkafka + OS Secure Storage) — отдельный этап,
 * вне задачи по переработке FSM.
 */
class KafkaConnector : public QObject
{
    Q_OBJECT

private:
    // Синглтон
    KafkaConnector();

    KafkaConnector(const KafkaConnector&) = delete;
    KafkaConnector& operator=(const KafkaConnector&) = delete;
    KafkaConnector(KafkaConnector&&) = delete;
    KafkaConnector& operator=(KafkaConnector&&) = delete;

    bool m_initialized = false;
    QByteArray m_userhash;

public:
    static KafkaConnector* instance();
    ~KafkaConnector() override = default;

public slots:
    // От AppManager (вход в KCONNECTOR): инициализироваться под пользователя
    // и прислать сохранённый offset.
    void onInitConnection(QByteArray userhash);

    // От AppManager (вход в READY): подписаться на broadcast-топик Kafka.
    void onSubscribeKafka();

    // От AppManager: общий shutdown/logout — сбросить состояние.
    void onShutdown();

signals:
    // offset последнего обработанного сообщения Kafka (после инициализации).
    // Именно этот сигнал триггерит переход KCONNECTOR → KAFKA в FSM.
    void signalOffsetReady(QString offset);

    // Статус подписки на broadcast-топик (после READY).
    void signalKafkaSubscribed(bool ok);
};

} // namespace uniter::net

#endif // KAFKACONNECTOR_H
