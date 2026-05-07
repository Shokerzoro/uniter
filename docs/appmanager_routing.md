# Маршрутизация сообщений внутри AppManager

Этот документ описывает правила диспетчеризации `UniterMessage`
в классе `uniter::control::AppManager` — «выше» (к `DataManager` /
`FileManager`) и «ниже» (в сетевые классы). Он дополняет
[`docs/fsm_appmanager.md`](fsm_appmanager.md) и
[`docs/add_data_convention.md`](add_data_convention.md), описывая поведение
конкретно тех двух слотов, через которые проходит весь трафик AppManager.

---

## Общая схема

```
                 ┌─────────────────────────────────────────────────┐
                 │                 AppManager                      │
                 │                                                 │
   UI (AuthWidget/                                                 │
   динамические  ──► onSendUniterMessage ──► dispatchOutgoing ──┬──│──► signalSendToServer   → ServerConnector
   виджеты)                                                     │  │
   FileManager   ──►                                            ├──│──► signalSendToMinio    → MinIOConnector
                                                                │  │
   FSM (enter-*) ────────────────────────────────────────────── ├──│──► signalSendToServer   → ServerConnector
                                                                │  │    (минуя onSendUniterMessage)
                                                                │  │
   ServerConnector ─┐                                           │  │
   KafkaConnector   ├─► onRecvUniterMessage                     │  │
   MinIOConnector   ┘                 │                         │  │
                                      ▼                         │  │
                            handleInitProtocolMessage  (FSM) ───┘  │
                            handleReadyProtocolMessage ────────────┼──► signalForwardToFileManager → FileManager
                            handleReadyCrudMessage ────────────────┼──► signalRecvUniterMessage    → DataManager
                                                                   │
                                                                   ▼
```

Оба направления проходят через один слот — `onRecvUniterMessage` для
входящих и `onSendUniterMessage` для исходящих; внутри каждого слота
AppManager самостоятельно принимает решение, в какую шину направить
сообщение. Сетевые и подсистемные классы не фильтруют трафик — это
ответственность AppManager.

---

## Состояния AppManager, важные для маршрутизации

Подробное описание FSM — в [`fsm_appmanager.md`](fsm_appmanager.md). Для
маршрутизации важны две оси:

* `m_appState`:
  - `IDLE / NET_CONNECTING / IDLE_AUTHENIFICATION / AUTHENIFICATION /
     KAFKA / SYNC` — **до READY** (инициализация);
  - `READY` — штатная работа с пользователем;
  - `SHUTDOWN` — ничего не маршрутизируется.
* `m_netState`: `ONLINE / OFFLINE` — в `OFFLINE` AppManager глушит
  обе шины.

В дальнейшем под «до READY» понимаются все состояния
`m_appState != READY` (кроме SHUTDOWN, который обрабатывается отдельно).

---

## Сигналы и слоты AppManager

### Входящие сигналы от AppManager (вверх)

| Сигнал | Получатель | Условия эмита |
| --- | --- | --- |
| `signalRecvUniterMessage(msg)` | `DataManager::onRecvUniterMessage` | `READY` + `subsystem != PROTOCOL` (CRUD) |
| `signalForwardToFileManager(msg)` | `FileManager::onForwardUniterMessage` (TODO, пока не подключён в `main.cpp`) | `READY` + `subsystem == PROTOCOL` + `protact ∈ { GET_MINIO_PRESIGNED_URL, GET_MINIO_FILE, PUT_MINIO_FILE }` |

### Исходящие сигналы от AppManager (вниз)

| Сигнал | Получатель | Назначение |
| --- | --- | --- |
| `signalSendToServer(msg)` | `ServerConnector::onSendMessage` | AUTH, `GET_KAFKA_CREDENTIALS`, `FULL_SYNC`, `GET_MINIO_PRESIGNED_URL`, любые CRUD, `UPDATE_*` |
| `signalSendToMinio(msg)` | `MinIOConnector::onSendMessage` | `GET_MINIO_FILE`, `PUT_MINIO_FILE` |

Все постоянные подключения этих сигналов выполняются в `main.cpp`
(раздел «AppManager ↔ Network Layer»).

### Входной слот от сети

| Слот | Подписчики |
| --- | --- |
| `onRecvUniterMessage(msg)` | `ServerConnector::signalRecvMessage`, `KafkaConnector::signalRecvMessage`, `MinIOConnector::signalRecvMessage` |

### Входной слот от UI / подсистем

| Слот | Подписчики |
| --- | --- |
| `onSendUniterMessage(msg)` | `AuthWidget` (до READY), динамические виджеты подсистем (в READY), `FileManager` (в READY) |

---

## Правила маршрутизации «вверх» (`onRecvUniterMessage`)

Источники сообщений: `ServerConnector` (ответы и ошибки сервера),
`KafkaConnector` (CRUD-`NOTIFICATION`, а также подтверждения `SUCCESS`),
`MinIOConnector` (ответы на GET/PUT файлов).

Правила в порядке приоритета:

1. **`m_netState == OFFLINE`** — любое входящее игнорируется с логом.
2. **`subsystem == PROTOCOL`**:
   - **`m_appState == READY`** → `handleReadyProtocolMessage(msg)`:
     - если `protact` попадает в `isMinioProtocolAction(...)` —
       эмитим `signalForwardToFileManager` (→ FileManager);
     - иначе (обновление Kafka credentials, `UPDATE_*`) — пока
       логируем и ничего не пересылаем (TODO).
   - **`m_appState != READY`** → `handleInitProtocolMessage(msg)`:
     часть процесса инициализации, пересылка наружу не нужна — FSM
     сама реагирует на `AUTH RESPONSE`, `GET_KAFKA_CREDENTIALS RESPONSE`
     и `FULL_SYNC SUCCESS`.
3. **`subsystem != PROTOCOL`** (CRUD):
   - **`m_appState == READY`** → `handleReadyCrudMessage(msg)` эмитит
     `signalRecvUniterMessage` (→ DataManager). Как правило, источник —
     `KafkaConnector` с `MessageStatus = NOTIFICATION / SUCCESS`.
   - **`m_appState != READY`** — CRUD в инициализации быть не должен,
     игнорируется с логом.

Вспомогательная функция:

```cpp
static bool AppManager::isMinioProtocolAction(contract::ProtocolAction a);
// true для GET_MINIO_PRESIGNED_URL, GET_MINIO_FILE, PUT_MINIO_FILE.
```

---

## Правила маршрутизации «вниз» (`onSendUniterMessage`)

Источники сообщений:

* сама FSM в enter-actions `enterAuthenification` /
  `enterKafka` / `enterSync` — использует `signalSendToServer` **напрямую**,
  минуя этот слот;
* `AuthWidget` — посылает AUTH REQUEST (до READY);
* динамические виджеты подсистем и `FileManager` — CRUD и
  `MINIO`-протокольные запросы в READY.

### В состоянии `READY`

1. **`m_netState == OFFLINE`** — сообщение игнорируется (TODO: буферизация).
2. Иначе вызывается `dispatchOutgoing(msg)`:
   - `subsystem != PROTOCOL` (CRUD) → `signalSendToServer`;
   - `subsystem == PROTOCOL`:
     - `protact == GET_MINIO_FILE` или `PUT_MINIO_FILE` →
       `signalSendToMinio`;
     - `protact == GET_MINIO_PRESIGNED_URL` → `signalSendToServer`
       (presigned URL выдаёт сервер, не MinIO);
     - любое другое (`AUTH`, `GET_KAFKA_CREDENTIALS`, `FULL_SYNC`,
       `UPDATE_*`) → `signalSendToServer`.

### До `READY`

Пропускается **только** `AUTH REQUEST` от UI — он сохраняется как
`m_authMessage`, и если FSM уже в `AUTHENIFICATION` и `ONLINE`, тут же
эмитится `signalSendToServer` и `ProcessEvent(AUTH_DATA_READY)`.

Любое другое сообщение до READY из этого слота **отклоняется с логом**.
В частности, `GET_KAFKA_CREDENTIALS` и `FULL_SYNC` сюда не приходят — FSM
формирует их внутри своих enter-actions и эмитит `signalSendToServer`
напрямую.

---

## Почему FSM обходит `onSendUniterMessage`

До READY единственный внешний канал — AUTH REQUEST от `AuthWidget`;
все остальные PROTOCOL-запросы инициируются самой FSM. Чтобы
`onSendUniterMessage` остался «воротами UI» с простым и строгим
инвариантом («до READY — только AUTH»), enter-actions FSM эмитят
`signalSendToServer` напрямую. Это сохраняет однозначность слота:

* **входная точка UI** — `onSendUniterMessage`;
* **выходная шина к серверу** — `signalSendToServer`, к ней могут
  обращаться и слот, и enter-actions FSM.

---

## Сводная таблица

| Направление | Источник | Состояние | `subsystem` | `protact` | Куда уходит |
| --- | --- | --- | --- | --- | --- |
| Вверх | Server/Kafka/MinIO | OFFLINE | любой | любой | *drop* |
| Вверх | Server | !READY | PROTOCOL | AUTH / GET_KAFKA_CREDENTIALS / FULL_SYNC | FSM (internal) |
| Вверх | Kafka | READY | CRUD | — | DataManager |
| Вверх | Server | READY | PROTOCOL | GET_MINIO_PRESIGNED_URL | FileManager |
| Вверх | MinIO | READY | PROTOCOL | GET_MINIO_FILE / PUT_MINIO_FILE | FileManager |
| Вверх | Server | READY | PROTOCOL | UPDATE_* / GET_KAFKA_CREDENTIALS | *log, TODO* |
| Вниз | UI (AuthWidget) | AUTHENIFICATION+ONLINE | PROTOCOL | AUTH REQUEST | ServerConnector |
| Вниз | UI / FileManager | READY+ONLINE | CRUD | — | ServerConnector |
| Вниз | FileManager | READY+ONLINE | PROTOCOL | GET_MINIO_PRESIGNED_URL | ServerConnector |
| Вниз | FileManager | READY+ONLINE | PROTOCOL | GET_MINIO_FILE / PUT_MINIO_FILE | MinIOConnector |
| Вниз | UI | READY+ONLINE | PROTOCOL | UPDATE_* | ServerConnector |
| Вниз | FSM (enter-actions) | !READY+ONLINE | PROTOCOL | GET_KAFKA_CREDENTIALS / FULL_SYNC / AUTH | ServerConnector (напрямую, минуя слот) |

---

## Связанные файлы

* `src/uniter/control/appmanager.h` — объявление сигналов и
  `dispatchOutgoing` / `handle*` методов;
* `src/uniter/control/appmanager.cpp` — реализация правил, этот
  документ дословно воспроизводит комментарии над методами;
* `src/uniter/main.cpp` — постоянные `QObject::connect` между
  AppManager и сетевым слоем;
* `src/uniter/network/miniconnector.cpp` — обработчик исходящих
  `GET_MINIO_FILE` / `PUT_MINIO_FILE`;
* `docs/add_data_convention.md` — контракты `add_data` для каждого
  `ProtocolAction`, включая `PUT_MINIO_FILE`;
* `docs/fsm_appmanager.md` — собственно FSM и enter-actions.
