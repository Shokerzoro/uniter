# FSM AppManager

Диаграмма: [FSM-AppManager.pdf](FSM-AppManager.pdf)

## Принципы

1. **Имя состояния = действие на входе.** Имена отражают entry-action, а не событие
   (`AUTHENIFICATION` = «запрос auth-данных», `KCONNECTOR` = «инициализация KafkaConnector»,
   `DBCLEAR` = «очистка БД»).
2. **Один entry-action на состояние.** Если действий несколько — вводится
   отдельное состояние (поэтому появились `KCONNECTOR` и `DBCLEAR`).
3. **Сетевые запросы — только через `UniterMessage`.** Вход в состояние, которое
   запрашивает что-то у сервера, формирует `UniterMessage` и эмитит
   `signalSendUniterMessage`. Отдельных сигналов типа «запрос full-sync» нет.
4. **Online/offline ортогональны `AppState`.** Потеря сети меняет только `NetState`
   и блокирует UI; `AppState` остаётся. Но семантика реакции на возврат сети
   зависит от типа состояния:
   - **Сетевые состояния** (имеют offline-зеркало на диаграмме):
     `AUTHENIFICATION`, `IDLE_AUTHENIFICATION`, `KAFKA`, `SYNC`, `READY` —
     при возврате online entry-action **повторяется** (возобновить запрос).
   - **Локальные состояния** (offline-зеркала нет):
     `DBLOADING`, `CONFIGURATING`, `KCONNECTOR`, `DBCLEAR` —
     при возврате online entry-action **не повторяется** (внутренние операции
     не зависят от сети).
5. **Отложенный вход в сетевое состояние.** Если в момент логического перехода
   в сетевое состояние (например `KCONNECTOR → KAFKA` после `OFFSET_RECEIVED`)
   сеть в `OFFLINE`, entry-action откладывается и выполнится при
   `NET_CONNECTED` через `reenterOnReconnect()`.
6. **`SHUTDOWN` достижим из любого состояния** (сохранение настроек и буферов).

## Состояния

| Состояние | Тип | Действие на входе | Что отправляется |
|---|---|---|---|
| `IDLE` | — | — | — |
| `STARTED` | — | запрос соединения у сетевого класса | `signalMakeConnection` |
| `AUTHENIFICATION` | сетевое | запрос auth-данных у виджета **либо** отправка уже полученных | `signalFindAuthData` / `UniterMessage(AUTH REQUEST)` |
| `IDLE_AUTHENIFICATION` | сетевое | ожидание ответа сервера | — |
| `DBLOADING` | локальное | сигнал пользователю об успехе + инициализация БД | `signalAuthed(true)`, `signalLoadResources(userhash)` |
| `CONFIGURATING` | локальное | вызов менеджера конфигурации | `signalConfigProc(User)` |
| `KCONNECTOR` | локальное | инициализация `KafkaConnector` под пользователя | `signalInitKafkaConnector(userhash)` |
| `KAFKA` | сетевое | запрос у сервера актуальности offset | `UniterMessage(GET_KAFKA_CREDENTIALS REQUEST)` + `add_data["offset"]` |
| `DBCLEAR` | локальное | полная очистка таблиц БД через `DataManager` | `signalClearDatabase` |
| `SYNC` | сетевое | запрос full-sync у сервера | `UniterMessage(FULL_SYNC REQUEST)` |
| `READY` | сетевое | подписка на broadcast-топик Kafka | `signalSubscribeKafka` |
| `SHUTDOWN` | — | сохранение настроек и буферов (TODO) | — |

## События

| Событие | Источник |
|---|---|
| `START` | `AppManager::start_run()` |
| `NET_CONNECTED` / `NET_DISCONNECTED` | сетевой класс (`onConnectionUpdated(bool)`) |
| `AUTH_DATA_READY` | внутреннее: после отправки `AUTH REQUEST` серверу |
| `AUTH_SUCCESS` / `AUTH_FAIL` | `UniterMessage(AUTH RESPONSE)` от сервера |
| `DB_LOADED` | `DataManager::signalResourcesLoaded` → `onResourcesLoaded` |
| `CONFIG_DONE` | `ConfigManager::signalConfigured` → `onConfigured` |
| `OFFSET_RECEIVED` | `KafkaConnector::signalOffsetReady(QString)` → `onKafkaOffsetReceived` |
| `OFFSET_ACTUAL` / `OFFSET_STALE` | `UniterMessage(GET_KAFKA_CREDENTIALS RESPONSE)` + `offset_actual` |
| `DB_CLEARED` | `DataManager::signalDatabaseCleared` → `onDatabaseCleared` |
| `SYNC_DONE` | `UniterMessage(FULL_SYNC SUCCESS)` от сервера |
| `LOGOUT` | UI → `onLogout` |
| `SHUTDOWN` | UI/система → `onShutDown` |

## Золотой путь

```
IDLE
 └─START→ STARTED
          └─NET_CONNECTED→ AUTHENIFICATION
                           └─AUTH_DATA_READY→ IDLE_AUTHENIFICATION
                                              └─AUTH_SUCCESS→ DBLOADING
                                                              └─DB_LOADED→ CONFIGURATING
                                                                           └─CONFIG_DONE→ KCONNECTOR
                                                                                          └─OFFSET_RECEIVED→ KAFKA
                                                                                                            ├─OFFSET_ACTUAL→ READY
                                                                                                            └─OFFSET_STALE→ DBCLEAR
                                                                                                                            └─DB_CLEARED→ SYNC
                                                                                                                                         └─SYNC_DONE→ READY
```

Из `READY`:
- `LOGOUT` → `IDLE_AUTHENIFICATION` (очистка ресурсов, отвязка пользователя).
- `SHUTDOWN` → финальное состояние.

## Переходы сигналов в `main.cpp`

См. блоки `=== 2. Управление сетевым состоянием ===`, `=== 2.1 KafkaConnector ===`
и `=== 3. Управление ресурсами и БД ===` в `src/uniter/main.cpp`.

## Работа с `UniterMessage` в FSM

Всё общение с сервером — через `UniterMessage`. Форматы (см. также
`docs/add_data_convention.md` и `src/uniter/contract/uniterprotocol.h`):

| Состояние | Запрос | Ответ сервера |
|---|---|---|
| `AUTHENIFICATION` | `PROTOCOL` + `AUTH` + `REQUEST` + `add_data["login","password_hash"]` | `AUTH RESPONSE` + `resource = Employee` или `error` |
| `KAFKA` | `PROTOCOL` + `GET_KAFKA_CREDENTIALS` + `REQUEST` + `add_data["offset"]` | `GET_KAFKA_CREDENTIALS RESPONSE` + `add_data["offset_actual"]` |
| `SYNC` | `PROTOCOL` + `FULL_SYNC` + `REQUEST` | сначала поток `CRUD CREATE` через Kafka, затем `FULL_SYNC SUCCESS` |

## Статус реализации заглушек

| Класс | Статус |
|---|---|
| `MockNetManager` | stub; отвечает на `AUTH`, `GET_KAFKA_CREDENTIALS` (`offset_actual=true`), `FULL_SYNC REQUEST` (`SUCCESS`), CRUD-эхо |
| `KafkaConnector` | stub; `onInitConnection(hash)` отдаёт произвольный offset через `QTimer::singleShot(0, ...)`; `onSubscribeKafka()` эмитит `signalKafkaSubscribed(true)` |
| `DataManager::onClearDatabase` | stub; сразу эмитит `signalDatabaseCleared` (TODO: реальный DROP/TRUNCATE таблиц) |
| `ServerConnector` | **не реализован**, роль выполняет `MockNetManager` |
| `FileManager` | **не реализован** |

## TODO (протокол / ресурсы)

- Хэш пользователя: сейчас `makeUserHash()` возвращает `id` в ASCII. Нужен реальный
  хэш (например, SHA256 от `id + salt`) для ключа в OS Secure Storage.
- В `DataManager::onClearDatabase` — реальная очистка БД (оставить только
  структуру таблиц).
- `KafkaConnector`: заменить stub на librdkafka-клиент + OS Secure Storage
  для persistence offset.
