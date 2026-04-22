---
name: uniter-current-state
description: Актуальное состояние проекта Uniter (Qt C++ PDM/ERP). Структура кода, реализованные компоненты, отсутствующие части и пошаговый план работ по развитию ядра.
---

# SKILL: Uniter — Актуальное состояние проекта

## Контекст проекта

**Uniter** — Qt C++ (C++17) клиент-серверная PDM/ERP система для автоматизации учёта материалов в машиностроении. CMake-проект с тремя подпроектами: статическая библиотека `Common`, исполняемый файл `Updater` и основное приложение `Uniter`. Сборка в двух вариантах: динамическая линковка (Debug/Profile) и статическая (RelWithDebInfo/Release). Среда: MSYS/MinGW, Qt, сторонние библиотеки MuPDF и tinyxml2. Тесты — GoogleTest, запускаются при каждой сборке.

Репозиторий: https://github.com/Shokerzoro/uniter

\---

## Структура исходного кода (актуальная)

```
src/
├── common/           # Статическая библиотека (appfuncs, excepts)
├── uniter/           # Основное приложение
│   ├── contract/     # Ресурсы и протокол (UniterMessage, ResourceAbstract, подсистемы)
│   │   ├── design/           # Assembly, Part, Project, FileVersion
│   │   ├── material/         # MaterialTemplateBase, Simple, Composite
│   │   ├── materialinstance/ # MaterialInstanceBase, Simple, Composite
│   │   ├── employee/         # Employee (User)
│   │   ├── supply/           # ProcurementRequest
│   │   ├── plant/            # ProductionTask
│   │   ├── unitermessage.h/.cpp
│   │   ├── uniterprotocol.h
│   │   └── resourceabstract.h/.cpp
│   ├── control/      # AppManager, ConfigManager, UIManager
│   ├── data/         # DataManager, IDataObserver
│   ├── network/      # TcpConnector, MockNetwork, MainNetworker, UpdaterWorker
│   ├── widgets\_static/
│   ├── widgets\_generative/
│   ├── widgets\_dynamic/
│   └── main.cpp
└── updater/
```

\---

## Актуальное состояние компонентов

### UniterMessage и протокол

**Файлы:** `contract/unitermessage.h`, `contract/uniterprotocol.h`

Текущая структура `UniterMessage`:

* Метаданные: `version`, `timestamp`, `message\_sequence\_id`, `request\_sequence\_id`
* Подсистема: `subsystem` (Subsystem enum), `GenSubsystem`, `GenSubsystemId`
* Тип операции: `crudact` (CrudAction), `protact` (ProtocolAction), `status` (MessageStatus), `error` (ErrorCode)
* Данные: `std::shared\_ptr<ResourceAbstract> resource`, `std::map<QString,QString> add\_data`
* Методы: `to\_xml()`, `form\_xml()`

**Что реализовано в протоколе:**

* `Subsystem`: PROTOCOL, MATERIALS, DESIGN, PURCHASES, MANAGER, GENERATIVE
* `CrudAction`: NOTCRUD, CREATE, READ, UPDATE, DELETE
* `ProtocolAction`: NOTPROTOCOL, ACCOCATE\_ID, AUTH, POLL
* `MessageStatus`: REQUEST, RESPONSE, ERROR, NOTIFICATION
* `ErrorCode`: SUCCESS, BAD\_REQUEST, BAD\_TIMING, PERMISSION\_DENIED, INTERNAL\_ERROR, SERVICE\_UNAVAILABLE
* `ResourceType`: DEFAULT, EMPLOYEES, PRODUCTION, INTEGRATION, GROUP, PURCHASE, PROJECT, ASSEMBLY, PART
* `GenSubsystemType`: NOTGEN, PRODUCTION, INTERGATION

**Чего НЕТ (требует доработки):**

* Отсутствует `MessageType` (CRUD / PROTOCOL / MINIO) — сейчас CRUD и PROTOCOL смешаны через `crudact`/`protact`
* Нет полей для Kafka: отсутствуют `SUCCESS` (подтверждение через Kafka) в MessageStatus (только NOTIFICATION есть)
* Нет полей для MinIO-операций (presigned URL, upload token)
* Нет поддержки передачи файлов в сообщении (UPDATE с бинарными данными)
* Подсистема `INSTANCES` (MaterialInstance) отсутствует в `Subsystem` enum
* `PDM` отсутствует в `Subsystem` enum

\---

### AppManager и FSM

**Файлы:** `control/appmanager.h`, `control/appmanager.cpp`

**Текущие состояния (AppState):** IDLE → STARTED → IDLE\_CONNECTED → CONNECTED → AUTHENTIFICATED → DBLOADED → READY → SHUTDOWN

**Текущие события (Events):** START, NET\_CONNECTED, NET\_DISCONNECTED, AUTH\_SUCCESS, DB\_LOADED, CONFIG\_DONE, LOGOUT, SHUTDOWN

**Реализованные методы:**

* Синглтон через `instance()`
* `start\_run()` — запуск FSM
* `onConnectionUpdated(bool)` — слот от сетевого класса
* `onResourcesLoaded()`, `onConfigured()`, `onLogout()`, `onShutDown()`
* `onRecvUniterMessage(...)` / `onSendUniterMessage(...)` — маршрутизация

**Реализованные сигналы:**

* `signalMakeConnection()`, `signalPollMessages()`
* `signalConnectionUpdated(bool)`, `signalAuthed(bool)`, `signalLoggedOut()`
* `signalFindAuthData()`, `signalLoadResources(QByteArray)`, `signalConfigProc(Employee)`, `signalClearResources()`
* `signalRecvUniterMessage(...)`, `signalSendUniterMessage(...)`

**Чего НЕТ (требует доработки):**

* Нет состояний для Kafka (KAFKA\_CONNECTED, ожидание credentials)
* Нет обработки FULL\_SYNC (POLL) как отдельного состояния/события
* Нет маршрутизации MinIO-сообщений
* Нет события для получения Kafka credentials от сервера

\---

### Сетевой слой

**Файлы:** `network/tcpconnector.h/.cpp`, `network/mocknetwork.h/.cpp`, `network/mainnetworker.h`, `network/updaterworker.h/.cpp`

**Что реализовано:**

* `TcpConnector` — реальный TCP/SSL коннектор к серверу с сериализацией UniterMessage в XML
* `MockNetwork` — заглушка, имитирующая сетевую работу (авторизация, базовые ответы)
* `MainNetworker` — фасад для выбора между TcpConnector и MockNetwork
* `UpdaterWorker` — отдельный класс для работы с обновлениями (скачивание, установка)

**Интерфейс `MainNetworker`:** принимает `std::shared\_ptr<UniterMessage>`, отправляет UniterMessage обратно через сигналы, взаимодействует только с AppManager

**Чего НЕТ:**

* `KafkaConnector` — отсутствует полностью (нет даже заглушки)
* `MinioConnector` — отсутствует полностью (нет даже заглушки)
* `ServerConnector` как отдельный класс (сейчас роль выполняет TcpConnector, без разделения ответственности по документации)

\---

### DataManager и Observer

**Файлы:** `data/datamanager.h/.cpp`, `data/idataobserver.h/.cpp`

**Что реализовано:**

* Синглтон `DataManager`
* Внутренние состояния: IDLE, LOADING, LOADED, ERROR
* Три списка observers: `treeObservers`, `listObservers`, `resourceObservers` (через `std::weak\_ptr`)
* Слоты: `onStartLoadResources(QByteArray)`, `onClearResources()`, `onRecvUniterMessage(...)`, `onSubsystemGenerate(...)`
* Подписки: `onSubscribeToResourceList/Tree/Resource()`, `onUnsubscribeFrom\*()`
* `onGetResource()` — прямой запрос ресурса
* `signalResourcesLoaded()` — сигнал AppManager

**Чего НЕТ (требует доработки):**

* Нет реальной локальной БД (SQLite) — нет инициализации, создания таблиц, персистентности
* Нет обработки CUD-операций с сохранением в БД
* Нет `SubscribeAdaptor` (механизм из документации для безопасных подписок)
* Нет разделения на `ResourceListObserver`, `ResourceObserver`, `TreeObserver` (пока единый `IDataObserver`)

\---

### Ресурсы (contract/)

**Что реализовано:**

| Подсистема | Ресурс | Статус |
|---|---|---|
| DESIGN | `Project` | Реализован (name, description, projectcode, rootdirectory, root\_assembly) |
| DESIGN | `Assembly` | Реализован (project\_id, name, type, parent, child\_assemblies, parts, docs) |
| DESIGN | `Part` | Реализован |
| DESIGN | `FileVersion` | Реализован |
| MATERIALS | `MaterialTemplateBase/Simple/Composite` | Реализованы |
| INSTANCES | `MaterialInstanceBase/Simple/Composite` | Реализованы |
| MANAGER | `Employee` | Реализован |
| PURCHASES | `Supply` (ProcurementRequest) | Реализован частично |
| MANAGER | `Plant` (ProductionTask) | Реализован частично |

**Чего НЕТ в ресурсах:**

* `PDM::Snapshot` — отсутствует как ресурс
* `PDM::Delta` — отсутствует
* `Assembly` не имеет полей для snapshot XML (документация §9)
* Структуры конструктора (Assembly/Part) не соответствуют целевой XML-структуре snapshot (§9 документации): нет `Invariant`, `Config`, `PartsDef`-нотации

\---

### Бизнес-логика

**Чего НЕТ (не реализовано совсем):**

* `PDMManager` — отсутствует
* `ERPManager` — отсутствует
* `FileManager` — отсутствует

\---

## План работ (приоритизированный)

### 1\. Актуализация UniterMessage (Kafka + MinIO + UPDATE с файлами)

**Задача:** расширить протокол без поломки существующей структуры.

* Добавить `MessageType` enum: `CRUD`, `PROTOCOL`, `MINIO`
* Добавить `MessageStatus::SUCCESS` (подтверждение CUD через Kafka)
* Добавить в `Subsystem`: `INSTANCES`, `PDM`
* Добавить в `ProtocolAction`: `GET\_KAFKA\_CREDENTIALS`, `GET\_MINIO\_TOKEN`, `FULL\_SYNC`
* Добавить поля MinIO в UniterMessage: `minio\_object\_key`, `minio\_presigned\_url`, `minio\_token`
* Добавить поддержку передачи файлов: поле `file\_payload` (опциональный бинарный blob или путь)
* Обновить `to\_xml()` / `form\_xml()` под новые поля

**1.1. Доработать AppManager — маршрутизацию с учётом новых типов**

* `onRecvUniterMessage`: ветвление по `MessageType` (CRUD → DataManager, PROTOCOL → FSM, MINIO → FileManager)
* Добавить сигналы: `signalSendToKafka(...)`, `signalSendToMinio(...)`

\---

### 2\. Актуализировать ресурсы конструкторской подсистемы

**Задача:** привести Assembly, Part, Project к целевой XML-структуре snapshot (документация §9).

Целевая XML-структура snapshot:

```xml
<shapshot id="" designation="" name="" version="" previousVersion="">
  <Assembly designation="" name="">
    <Structure>
      <Invariant>
        <Assemblies><AssemblyRef designation="" config=""/></Assemblies>
        <Parts><PartRef designation="" config=""/></Parts>
      </Invariant>
      <Config number="01">...</Config>
    </Structure>
    <PartsDef>
      <Part designation="" name="">
        <Config id="01"/>
      </Part>
    </PartsDef>
  </Assembly>
</shapshot>
```

Целевая структура `Partdef`:

```xml
<Partdef designation="..." name="...">
  <Metadata>
    <Material><Base>...</Base><CoatingTop/><CoatingBottom/></Material>
    <Signatures><Signature role="..." name="..." date="..."/></Signatures>
    <Litera>О</Litera>
    <Organization>...</Organization>
    <DrawingFile><Path/><Hash/><ModifiedAt/></DrawingFile>
  </Metadata>
  <Config id="01"><Dimensions length="" width="" height=""/><Mass/></Config>
</Partdef>
```

* Добавить в `Assembly`: поля `designation`, `configs` (map<QString, ConfigData>), `invariant`
* Добавить в `Part`: поля `designation`, `litera`, `organization`, `signatures`, `configs` с Dimensions/Mass
* Добавить ресурс `PDM::Snapshot` и `PDM::Delta`
* Обновить сериализацию `from\_xml` / `to\_xml`

\---

### 3\. Реализовать DataManager с локальной SQLite БД

**Задача:** полноценный DataManager с персистентностью.

* Инициализация: открыть существующую БД пользователя или создать новую (имя файла привязать к userhash)
* Схема: по одной таблице на каждый тип ресурса (materials, instances, projects, assemblies, parts, snapshots, deltas, employees, purchases, plant\_tasks)
* Обработка входящих UniterMessage с `crudact` == CREATE/UPDATE/DELETE → запись в SQLite
* Реализовать механизм подписок через `SubscribeAdaptor`:

  * `ResourceListObserver` — уведомление при изменении списка ресурсов подсистемы
  * `ResourceObserver` — уведомление при изменении конкретного ресурса по id
  * `TreeObserver` — уведомление при изменении дерева (для конструкторской подсистемы)
* READ: прямая выдача данных из SQLite через `onGetResource()`

\---

### 4\. Обновить FSM в AppManager (Kafka + POLL)

**Задача:** учесть новые состояния после добавления Kafka.

Новые состояния FSM:

* `AUTHENTIFICATED` → запрос Kafka credentials → `KAFKA\_CONNECTING` → `KAFKA\_CONNECTED`
* При успешном подключении к Kafka → запуск DB\_LOAD (FULL\_SYNC или продолжение с offset)
* Обработка события `FULL\_SYNC\_REQUIRED`: очистка БД, POLL с сервера, пассивный приём CRUD из Kafka

Механизм POLL:

* `signalPollMessages()` инициирует FULL\_SYNC: `onClearResources()` → запрос синхронизации серверу → пассивный приём всех CRUD-сообщений → переход в READY

\---

### 5\. Реализовать заглушки сетевых классов (ServerConnector, KafkaConnector, MinioConnector)

**Задача:** заглушки, моделирующие полноценную сетевую работу без реальной сети.

**ServerConnector (stub):**

* При авторизации — всегда возвращает `Employee` с полными правами
* На CRUD-запросы — возвращает `RESPONSE` с `SUCCESS`, параллельно генерирует `NOTIFICATION` через KafkaConnector
* На `GET\_KAFKA\_CREDENTIALS` — возвращает фиктивные credentials
* На `GET\_MINIO\_TOKEN` — возвращает фиктивный presigned URL

**KafkaConnector (stub):**

* Хранит offset в памяти (имитация OS Secure Storage)
* При инициализации: если offset == 0 → сигнал FULL\_SYNC\_REQUIRED
* Получает от ServerConnector подтверждённые CUD операции и рассылает их как `NOTIFICATION` / `SUCCESS`

**MinioConnector (stub):**

* Работает с локальной папкой (аналог S3 bucket)
* GET по presigned URL → читает файл из локального пути
* PUT → сохраняет файл локально

\---

### 6\. Реализовать FileManager

**Задача:** координатор операций с файлами.

* Цепочка загрузки: запрос presigned URL у сервера (через AppManager/ServerConnector) → запрос файла у MinioConnector → проверка SHA256 (из DataManager)
* Цепочка выгрузки: PUT через MinioConnector → уведомление DataManager об обновлении URL
* `IFileObserver` — интерфейс подписки: `onFileDownloaded(key, localPath)`, `onFileUploaded(key, url)`
* Кэширование: не скачивать повторно файл с совпадающим SHA256

\---

### 7\. Реализовать PDMManager и ERPManager (бизнес-логика)

**PDMManager:**

* Работает поверх DataManager и FileManager
* Парсинг PDF-каталога через MuPDF → XML snapshot (вызов библиотеки компиляции)
* Создание нового Snapshot ресурса или Delta при повторном парсинге
* Управление версиями: применение дельты, откат к предыдущей версии
* Валидация КД по критериям ЕСКД
* Управление XMLDocument на уровне PDMManager, передача root в компилятор

**ERPManager:**

* Работает поверх DataManager
* Расчёт потребности в материалах на основе ProductionTask + конструкторских данных
* Прогнозирование дефицитов: сравнение доступных MaterialInstance с требуемыми
* Построение отчётов по материальному балансу

\---

## Текущее состояние работоспособности

Приложение работает на **минимальном уровне**:

* Запуск, FSM инициализации, подключение через MockNetwork, авторизация
* Структуры ресурсов определены, сериализация базовая реализована
* DataManager создан как skeleton (без реальной БД)
* UI статические виджеты существуют
* Kafka, MinIO, PDMManager, ERPManager, FileManager — **не реализованы**
