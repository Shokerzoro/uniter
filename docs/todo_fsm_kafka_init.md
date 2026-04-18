# TODO: FSM — Инициализация KafkaConnector после AUTHENTIFICATED

**Контекст:** Пункт разработки №4 (по плану работ). Выходит за рамки текущего шага (протокол).

---

## Текущее поведение (что есть сейчас)

```
AUTHENTIFICATED → in_authentificated():
    emit signalLoadResources(userhash)   // DataManager инициализирует SQLite
```

После `signalResourcesLoaded()` от DataManager:
```
AUTHENTIFICATED → DB_LOADED → in_dbloaded():
    emit signalConfigProc(m_user)        // ConfigManager строит UI-вкладки
```

Kafka, GET_KAFKA_CREDENTIALS, FULL_SYNC — **не задействованы**.

---

## Что нужно реализовать

Последовательность после AUTH_SUCCESS строго упорядочена контрактом.
**DB_LOADING как отдельное FSM-состояние не нужно** — AppManager знает,
что при входе в AUTHENTIFICATED он инициировал загрузку БД,
и ждёт события DB_LOADED (сигнал от DataManager).

### Расширенная цепочка инициализации

```
CONNECTED
  ↓  AUTH_SUCCESS
AUTHENTIFICATED
  → emit signalLoadResources(userhash)         // DataManager: открыть/создать SQLite
  → emit signalRequestKafkaCredentials()       // ServerConnector: GET_KAFKA_CREDENTIALS REQUEST
  (ответ GET_KAFKA_CREDENTIALS RESPONSE придёт в handleInitProtocolMessage — нужно расширить)
  → emit signalInitKafka(credentials)          // KafkaConnector: подключиться к брокеру
  → KafkaConnector проверяет сохранённый offset (OS Secure Storage, ключ = userhash)

  Ветка A: offset отсутствует или устарел (earliest available > saved_offset)
    → emit signalFullSyncRequired()
    → emit signalClearResources()              // DataManager: DROP + CREATE таблиц
    → emit signalSendFullSyncRequest()         // ServerConnector: FULL_SYNC REQUEST
    → DataManager принимает поток CRUD CREATE из Kafka (статус NOTIFICATION)
    → сервер завершает поток: FULL_SYNC + SUCCESS
    → DB_LOADED → in_dbloaded() → emit signalConfigProc(m_user)

  Ветка B: offset актуален
    → KafkaConnector начинает читать с saved_offset
    → пропущенные NOTIFICATION применяются к DataManager в фоне
    → DB_LOADED → in_dbloaded() → emit signalConfigProc(m_user)
```

### Важные детали реализации

**1. handleInitProtocolMessage нужно расширить:**
Сейчас он обрабатывает только AUTH RESPONSE в состоянии CONNECTED.
Нужно добавить ветку: состояние AUTHENTIFICATED + protact == GET_KAFKA_CREDENTIALS + status == RESPONSE.

```cpp
// В handleInitProtocolMessage добавить:
if (m_appState == AppState::AUTHENTIFICATED &&
    message->protact == contract::ProtocolAction::GET_KAFKA_CREDENTIALS &&
    message->status  == contract::MessageStatus::RESPONSE) {
    // Извлечь credentials из add_data и передать KafkaConnector
    emit signalInitKafka(message);
    return;
}
```

**2. Новые сигналы AppManager (добавить в appmanager.h):**
```cpp
void signalRequestKafkaCredentials();          // → ServerConnector
void signalInitKafka(std::shared_ptr<contract::UniterMessage> credentialsMsg);  // → KafkaConnector
void signalFullSyncRequired();                 // → DataManager (очистить БД)
void signalSendFullSyncRequest();              // → ServerConnector (FULL_SYNC REQUEST)
```

**3. Новые события FSM (добавить в Events):**
```cpp
KAFKA_CREDENTIALS_RECEIVED,   // GET_KAFKA_CREDENTIALS RESPONSE получен
KAFKA_CONNECTED,               // KafkaConnector подтвердил соединение
KAFKA_FAILED,                  // KafkaConnector не смог подключиться
FULL_SYNC_REQUIRED,            // KafkaConnector: offset устарел
FULL_SYNC_COMPLETE,            // сервер завершил поток FULL_SYNC
```

**4. Маркер завершения FULL_SYNC:**
Сервер отправляет последним:
```
UniterMessage {
  subsystem: PROTOCOL,
  protact:   FULL_SYNC,
  status:    SUCCESS,
  error:     SUCCESS
}
```
AppManager при получении этого сообщения в `handleInitProtocolMessage` (или новом `handlePostAuthProtocolMessage`)
генерирует событие `FULL_SYNC_COMPLETE` → DataManager → DB_LOADED.

**5. NOTIFICATION во время FULL_SYNC:**
Пока AppManager находится между AUTHENTIFICATED и DB_LOADED, входящие Kafka NOTIFICATION
не должны применяться к DataManager — они не нужны, т.к. FULL_SYNC содержит полное актуальное
состояние. KafkaConnector обновляет offset по каждому прочитанному сообщению, поэтому после
окончания FULL_SYNC offset уже будет актуальным, и пропусков не возникнет.

**6. Именование ключа offset в OS Secure Storage:**
Ключ привязан к пользователю: `"uniter_kafka_offset_" + QString::number(employee.id)`.
Это позволяет нескольким пользователям на одной машине иметь независимые offset-ы.

---

## Связанные файлы для изменения

| Файл | Что изменить |
|------|-------------|
| `control/appmanager.h` | Events, новые сигналы |
| `control/appmanager.cpp` | handleInitProtocolMessage, in_authentificated, ProcessEvent |
| `network/kafkaconnector.h/.cpp` | Новый класс (IKafkaConnector + stub) |
| `network/mainnetworker.h` | Добавить KafkaConnector |
| `data/datamanager.cpp` | Слот onClearResources уже есть; добавить режим FULL_SYNC (подавить Observer-уведомления) |
