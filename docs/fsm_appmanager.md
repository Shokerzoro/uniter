# FSM AppManager

Диаграмма: [FSM-AppManager.pdf](FSM-AppManager.pdf)

## Принципы

1. **Имя состояния = действие на входе.** Раньше имена отражали событие, приведшее
   к переходу (`CONNECTED`, `AUTHENTIFICATED`), теперь — действие (`AUTHENIFICATION`,
   `DBLOADING`, `CONFIGURATING`, `KAFKA`, `SYNC`, `READY`).
2. **Online/offline ортогональны логическому состоянию.** Потеря сети не сбрасывает
   FSM, только блокирует UI. Восстановление сети не продвигает FSM (кроме
   единственного перехода `STARTED → AUTHENIFICATION`).
3. **`SHUTDOWN` достижим из любого состояния** (сохранение настроек и буферов).

## Состояния

| Состояние | Действие на входе | Отправляемый сигнал |
|---|---|---|
| `IDLE` | — | — |
| `STARTED` | запрос соединения у сетевого класса | `signalMakeConnection` |
| `AUTHENIFICATION` | запрос логина/пароля у виджета **либо** отправка уже полученных | `signalFindAuthData` / `signalSendUniterMessage(AUTH)` |
| `IDLE_AUTHENIFICATION` | ожидание ответа сервера | — |
| `DBLOADING` | сигнал пользователю об успехе + запрос инициализации БД | `signalAuthed(true)`, `signalLoadResources(userhash)` |
| `CONFIGURATING` | вызов менеджера конфигурации | `signalConfigProc(User)` |
| `KAFKA` | запрос инициализации MinIOConnector (для получения offset) | `signalInitMinIO` |
| `SYNC` | запрос full-sync у сервера | `signalPollMessages` |
| `READY` | подписка на broadcast-топик Kafka | `signalSubscribeKafka` |
| `SHUTDOWN` | сохранение настроек и буферов (TODO) | — |

## События

| Событие | Откуда приходит |
|---|---|
| `START` | `AppManager::start_run()` |
| `NET_CONNECTED` / `NET_DISCONNECTED` | сетевой класс (`onConnectionUpdated(bool)`) |
| `AUTH_DATA_READY` | внутреннее: после отправки `AUTH REQUEST` серверу |
| `AUTH_SUCCESS` / `AUTH_FAIL` | ответ сервера на `AUTH` |
| `DB_LOADED` | `DataManager::signalResourcesLoaded` → `onResourcesLoaded` |
| `CONFIG_DONE` | `ConfigManager::signalConfigured` → `onConfigured` |
| `OFFSET_RECEIVED` | `MinIOConnector::signalOffsetReady(QString)` → `onKafkaOffsetReceived` |
| `OFFSET_ACTUAL` / `OFFSET_STALE` | ответ сервера (`GET_KAFKA_CREDENTIALS RESPONSE` + `offset_actual`) |
| `SYNC_DONE` | ответ сервера (`FULL_SYNC SUCCESS`) |
| `LOGOUT` | UI → `onLogout` |
| `SHUTDOWN` | UI/система → `onShutDown` |

## Золотой путь

```
IDLE
 └─START→ STARTED
          └─NET_CONNECTED→ AUTHENIFICATION
                           └─widget отдал auth→ IDLE_AUTHENIFICATION
                                                └─AUTH_SUCCESS→ DBLOADING
                                                                └─DB_LOADED→ CONFIGURATING
                                                                              └─CONFIG_DONE→ KAFKA
                                                                                             ├─OFFSET_RECEIVED (шлём серверу)
                                                                                             ├─OFFSET_ACTUAL→ READY
                                                                                             └─OFFSET_STALE→ SYNC
                                                                                                             └─SYNC_DONE→ READY
```

## Переходы сигналов в `main.cpp`

См. блоки `=== 2. Управление сетевым состоянием ===` и `=== 2.1 MinIOConnector ===`
в `src/uniter/main.cpp`. Все постоянные коннекты созданы там.

## Статус реализации заглушек

| Класс | Статус |
|---|---|
| `MockNetManager` | stub; отвечает на `AUTH`, `GET_KAFKA_CREDENTIALS` (`offset_actual=true`), CRUD-эхо |
| `MinIOConnector` | stub; при `onInitConnection()` отдаёт произвольный offset через `QTimer::singleShot(0, ...)` |
| `KafkaConnector` | **не реализован**, его функциональность временно лежит на `MinIOConnector` (offset + подписка) |
| `ServerConnector` | **не реализован**, роль выполняет `MockNetManager` |
| `FileManager` | **не реализован** |
