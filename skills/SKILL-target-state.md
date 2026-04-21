---
name: uniter-target-state
description: Целевое состояние проекта Uniter как PDM/ERP системы без интеграций и ИИ. Архитектура, подсистемы, протокол, FSM, DataManager, FileManager, PDMManager и ERPManager.
---

# SKILL: Uniter — Целевое состояние проекта (PDM/ERP без интеграций и ИИ)

## Общее описание системы

**Uniter** — Qt C++ (C++17) клиент-серверная PDM/ERP система для автоматизации учёта материалов в машиностроении. Цель: охватить процессы выпуска КД, закупок и производства для производственных/проектных фирм. Использует ЕСКД — готовое конструкторское документирование (КД) парсится для автоматического извлечения структуры изделий, материалов и исполнений.

Архитектурный принцип: все функции системы — CRUD операции над ресурсами. Это позволяет масштабировать систему и добавлять новые бизнес-процессы без изменения ядра.

---

## Архитектура: четыре независимых слоя

```
┌───────────────────────────────────────────┐
│             UI Layer                      │  MainWindow, виджеты подсистем
├───────────────────────────────────────────┤
│         Business Logic Layer              │  PDMManager, ERPManager
├───────────────────────────────────────────┤
│        Data Management Layer              │  DataManager, FileManager
├───────────────────────────────────────────┤
│       Application Management Layer       │  AppManager (FSM), ConfigManager
├───────────────────────────────────────────┤
│           Network Layer                   │  ServerConnector, KafkaConnector, MinioConnector
└───────────────────────────────────────────┘
```

Все слои взаимодействуют только через строго определённые границы. Сетевой слой не имеет прямых связей с бизнес-логикой или UI — только через AppManager.

---

## Сетевой слой (Network Layer)

### ServerConnector

TCP/SSL соединение с самописным сервером (движок авторизации и CRUD).

**Ответственность:**
- Установка и поддержание постоянного TCP/SSL соединения
- Сериализация UniterMessage в XML и отправка на сервер
- Десериализация XML-ответов в UniterMessage, передача наверх
- Буферизация исходящих CUD запросов при отсутствии соединения
- Обработка таймаутов запросов

**Операции (PROTOCOL):** AUTH, FULL_SYNC, GET_KAFKA_CREDENTIALS, GET_MINIO_TOKEN
**Операции (CRUD):** CREATE, UPDATE, DELETE для всех подсистем

### KafkaConnector

Односторонний канал broadcast-уведомлений. Клиент — только consumer.

**Ответственность:**
- Подключение к Kafka с credentials от сервера
- Подписка на топик компании, непрерывное чтение, десериализация, передача наверх
- Хранение последнего обработанного offset в OS Secure Storage (привязка к конкретному User)
- При инициализации: проверка доступности offset → сигнал FULL_SYNC_REQUIRED если offset устарел

**Типы входящих сообщений из Kafka:**
- `NOTIFICATION` — изменение данных другим пользователем
- `SUCCESS` — подтверждение успешного выполнения собственной CUD операции

### MinioConnector

HTTP-клиент для MinIO S3 API. Работает только с presigned URLs.

**Ответственность:**
- HTTP GET/PUT/DELETE к MinIO по presigned URLs
- Атомарные задачи: какой файл скачать/загрузить и куда
- Не хранит credentials — только временные presigned URLs от сервера

---

## Слой управления приложением (Application Management Layer)

### AppManager — Главная FSM

**Состояния:**

```
IDLE → STARTED → CONNECTED → AUTHENTIFICATED
  → KAFKA_CONNECTING → KAFKA_CONNECTED
  → DB_LOADING (FULL_SYNC или продолжение с offset)
  → READY
  ↕ (при потере связи ↔ OFFLINE_*)
  SHUTDOWN
```

**Ключевые переходы:**
- `STARTED` → DNS-lookup → SSL connect → `CONNECTED`
- `CONNECTED` → AUTH → получение User → `AUTHENTIFICATED`
- `AUTHENTIFICATED` → запрос Kafka credentials → `KAFKA_CONNECTING` → `KAFKA_CONNECTED`
- `KAFKA_CONNECTED` → проверка offset: если FULL_SYNC → очистка БД + POLL с сервера → `DB_LOADING` → `READY`; если offset актуален → `DB_LOADING` → `READY`
- При потере сети: переход в соответствующее OFFLINE-состояние (UI заблокирован)

**Маршрутизация UniterMessage по MessageType:**
- `PROTOCOL` → FSM AppManager / ConfigManager
- `CRUD` (RESPONSE / NOTIFICATION / SUCCESS) → DataManager
- `MINIO` → FileManager

**Механизм POLL (FULL_SYNC):**
1. `onClearResources()` → DataManager очищает БД
2. Запрос FULL_SYNC серверу через ServerConnector
3. Пассивный приём всех CRUD сообщений (CREATE) → DataManager
4. По завершению → `READY`

### ConfigManager

- Парсинг `resources::Employee` (User) от сервера
- Генерация сигналов для создания виджетов-вкладок подсистем
- Обработка изменений конфигурации (новые генеративные подсистемы)

---

## Слой управления данными (Data Management Layer)

### DataManager

Центральный компонент управления локальной SQLite БД.

**Инициализация:**
- При запуске: открыть существующую БД пользователя (имя файла = хэш пользователя) или создать новую с нуля
- Схема: по одной таблице на каждый тип ресурса

**Таблицы SQLite (по одной на тип ресурса):**
`materials`, `material_templates`, `material_instances`, `projects`, `assemblies`, `parts`, `snapshots`, `deltas`, `employees`, `purchases`, `procurement_requests`, `production_tasks`

**Обработка UniterMessage (CUD):**
- `CREATE` → INSERT в соответствующую таблицу
- `UPDATE` → UPDATE (побеждает сообщение с большим sequence_id)
- `DELETE` → soft delete (пометка `actual = false`)

**Система подписок (Observer pattern) через SubscribeAdaptor:**

| Тип Observer | Назначение | Уведомление |
|---|---|---|
| `ResourceListObserver` | Список ресурсов подсистемы | При любом изменении в таблице |
| `ResourceObserver` | Конкретный ресурс по id | При изменении конкретного ресурса |
| `TreeObserver` | Дерево (конструктор) | При изменении структуры проекта |

`SubscribeAdaptor` — класс-адаптер, предотвращающий висячие указатели, race conditions и прямые вызовы.

**READ:** прямое извлечение данных из SQLite через `onGetResource()` без Observer.

**FULL_SYNC:** `onClearResources()` → DROP + CREATE таблиц → пассивный приём CREATE операций.

### FileManager

Координатор операций с файлами через MinIO.

**Цепочка загрузки (download):**
1. Запрос presigned URL у сервера (через AppManager)
2. `MinioConnector::get(url, localPath)`
3. Проверка SHA256 (хэш из DataManager)
4. Уведомление подписчиков через `IFileObserver::onFileDownloaded(key, localPath)`

**Цепочка выгрузки (upload):**
1. Запрос upload token у сервера
2. `MinioConnector::put(url, localPath)`
3. Уведомление DataManager об обновлении URL ресурса
4. Уведомление через `IFileObserver::onFileUploaded(key, url)`

**Кэширование:** не скачивать файл, если SHA256 совпадает с кэшированным.

---

## Протокол UniterMessage (целевая структура)

### MessageType (тип сообщения)
- `CRUD` — операции Create/Update/Delete ресурсов
- `PROTOCOL` — протокольные операции с сервером
- `MINIO` — операции с файловым хранилищем

### MessageStatus
- `REQUEST` — запрос от клиента
- `RESPONSE` — ответ от сервера
- `ERROR` — ошибка обработки
- `NOTIFICATION` — уведомление об изменении данных другим пользователем (из Kafka)
- `SUCCESS` — подтверждение успешного выполнения собственной CUD операции (из Kafka)

### Subsystem (для CRUD)
`MATERIALS`, `INSTANCES`, `DESIGN`, `PDM`, `PURCHASES`, `MANAGER`, `GENERATIVE`

### ProtocolAction (для PROTOCOL)
`AUTH`, `FULL_SYNC`, `ACCOCATE_ID`, `GET_KAFKA_CREDENTIALS`, `GET_MINIO_TOKEN`

### Поля MinIO (для MINIO)
- `minio_object_key` — ключ объекта в MinIO
- `minio_presigned_url` — временный URL с токеном доступа
- Опциональный `file_payload` для передачи файлов при UPDATE

---

## Подсистемы и ресурсы (полный список)

### Подсистема базы материалов (MATERIALS)

| Ресурс | Описание |
|---|---|
| `MaterialTemplateSimple` | Шаблон материала (ГОСТ/ОСТ/ТУ), сегменты префикса/суффикса |
| `MaterialTemplateComposite` | Составной шаблон (два простых, через дробь) |

`SegmentDefinition`: id, code, name, description, value_type (STRING/ENUM/NUMBER/CODE), allowed_values, is_active

### Подсистема ссылок на материалы (INSTANCES)

| Ресурс | Описание |
|---|---|
| `MaterialInstanceSimple` | Специализация простого шаблона + Quantity |
| `MaterialInstanceComposite` | Специализация составного шаблона + Quantity |

`Quantity`: `items` (штуки), `length` (м), `fig.area` (м²) — зависит от DimensionType.

### Подсистема конструктора (DESIGN)

| Ресурс | Описание |
|---|---|
| `Project` | name, description, projectcode, rootdirectory, root_assembly |
| `Assembly` | designation, name, type (REAL/VIRTUAL), parent_id, child_assemblies, parts, configs, invariant, docs |
| `Part` (PartDef) | designation, name, litera, organization, signatures, material (MaterialInstance), configs с Dimensions/Mass, DrawingFile (path + SHA256) |

**Целевая XML-структура Assembly (snapshot):**
```xml
<Assembly designation="" name="">
  <Structure>
    <Invariant>
      <Assemblies><AssemblyRef designation="" config=""/></Assemblies>
      <Parts><PartRef designation="" config=""/></Parts>
      <StandardProducts/>
      <BuyingProducts/>
      <OtherMaterials/>
    </Invariant>
    <Config number="01">...</Config>
  </Structure>
  <PartsDef>
    <Part designation="" name="">
      <Config id="01"><Dimensions length="" width="" height=""/><Mass/></Config>
    </Part>
  </PartsDef>
</Assembly>
```

**Целевая структура PartDef:**
```xml
<Partdef designation="..." name="...">
  <Metadata>
    <Material>
      <Base>Швеллер 20П ГОСТ 8239-89</Base>
      <CoatingTop/><CoatingBottom/>
    </Material>
    <Signatures>
      <Signature role="Разработал" name="..." date="..."/>
    </Signatures>
    <Litera>О</Litera>
    <Organization>ОАО "Завод"</Organization>
    <DrawingFile><Path/><Hash/><ModifiedAt/></DrawingFile>
  </Metadata>
  <Config id="01">
    <Dimensions length="2400" width="200" height="80"/>
    <Mass>85.2</Mass>
  </Config>
</Partdef>
```

### Подсистема PDM (PDM)

| Ресурс | Описание |
|---|---|
| `Snapshot` | Слепок конкретного проекта на момент парсинга. Поля: id, designation, name, version, previousVersion, XML-содержимое сборок |
| `Delta` | Ресурс внутри Snapshot. Инкрементальные изменения между версиями |

Snapshot-XML:
```xml
<shapshot id="" designation="" name="" version="2" previousVersion="1">
  <Assembly ...>...</Assembly>
  <Errors>
    <Error severity="Error" category="FileSystem" type="NO_FILE" path="..."/>
    <Error severity="Warning" category="VersionControl" type="INFORMAL_CHANGE" .../>
  </Errors>
</shapshot>
```

### Подсистема закупок (PURCHASES)

| Ресурс | Описание |
|---|---|
| `ProcurementRequest` | Закупочная заявка с MaterialInstance |
| `ProcurementRequestComplex` | Комплексная заявка (ссылки на простые ProcurementRequest) |

### Подсистема менеджера (MANAGER)

| Ресурс | Описание |
|---|---|
| `Employee` | Сотрудник/пользователь, права доступа |
| `Production` | Производство (генеративный ресурс — создаёт подсистему) |

### Генеративные подсистемы (GENERATIVE)

**Подсистема производства X:**

| Ресурс | Описание |
|---|---|
| `ProductionTask` | Производственное задание со своим id |
| `MaterialInstance` list | Список материалов задания |

---

## Слой бизнес-логики

### PDMManager

Управляет конструкторской документацией и версионированием.

**Ответственность:**
- Взаимодействие с FileManager, DataManager, AppManager, UI (предоставление API)
- Парсинг PDF-каталога через MuPDF → XML snapshot (через библиотеку компилятора чертежей)
- При первом парсинге → создание нового ресурса Snapshot
- При повторном парсинге → создание Delta (инкрементальные изменения)
- Управление версиями: применение дельты, откат к предыдущей версии
- Предложение изменений, принятие или отклонение
- Валидация КД по критериям ЕСКД

**Компилятор чертежей (вызывается из PDMManager):**
1. Формирование инициирующего списка PDF файлов проекта
2. Разбивка на примитивы трансляции (каждая страница PDF)
3. Определение типа каждой страницы по designation и основной надписи
4. Объединение страниц в единицы трансляции (логические документы)
5. Распределение по bucket-ам (спецификации, сборочные чертежи, детали)
6. Тип-специфичные компиляторы для каждого bucket-а
7. Линковка в дерево проекта

**Создание XMLDocument и root-элемента:** на уровне PDMManager. В библиотеку компилятора передаётся только root.

**Критерии изменений версии** (не просто изменение хэша):
- Изменение имени файла (например, добавление Рев.Б)
- Добавление конфигураций
- Изменение габаритов
- Добавление/изменение информации в основной надписи

### ERPManager

Аналитика материалов и производства. Работает локально над данными из DataManager.

**Ответственность:**
- Расчёт потребности в материалах по производственным планам (ProductionTask × конструкторские данные)
- Прогнозирование дефицита материалов (требуемые MaterialInstance vs доступные запасы)
- Анализ доступных запасов vs требуемых количеств
- Построение отчётов по материальному балансу

---

## Слой визуализации (UI Layer)

### Статические виджеты (существуют на протяжении всего жизненного цикла)
- `MainWindow` — оркестрация всех виджетов, синглтон
- `AuthWidget` — виджет аутентификации
- `WorkspaceWidget` — рабочая зона с вкладками подсистем

### Виджеты подсистем (генеративные, создаются ConfigManager)
- Подписка на ResourceList/TreeObserver конкретной подсистемы
- Отображение данных по бизнес-процессу
- Обработка пользовательского ввода
- Создание динамических виджетов и управление ими
- Отправка UniterMessage через AppManager

### Динамические виджеты (создаются виджетами подсистем)
- Подписка на ResourceObserver для конкретного ресурса
- Формирование UniterMessage при CRUD-действиях пользователя
- Использование IFileObserver для доступа к файлам из MinIO

---

## Порядок рантайма (startup sequence)

1. Создание `MainWindow` (статических виджетов), базовое состояние FSM
2. Создание `DataManager`, связывание сигналов/слотов с `MainWindow`
3. Создание `AppManager` → создаёт `ConfigManager`. Связывание с `MainWindow` и `DataManager`
4. Создание сетевых классов (`ServerConnector`, `KafkaConnector`, `MinioConnector`), связывание с `AppManager`
5. Создание `FileManager`, связывание с `AppManager`, `DataManager`, `MinioConnector`
6. `AppManager::start_run()` → FSM: сигнал на подключение → DNS → SSL
7. Авторизация: AuthWidget → UniterMessage AUTH → сервер → Employee
8. Запрос Kafka credentials → KafkaConnector init → проверка offset
9. Если FULL_SYNC: очистка БД → POLL → приём всех CRUD → READY
10. ConfigManager генерирует вкладки подсистем на основе Employee.permissions
11. Работа: UI генерирует UniterMessage → ServerConnector → Kafka broadcast → DataManager → UI update

---

## Принципы коннекта сигналов и слотов

- Большинство долгоживущих объектов — синглтоны
- Все постоянные коннекты выполняются в `main()` — это подчёркивает запрет произвольных коннектов в рантайме
- Исключения (коннекты в рантайме): отправка сообщений через AppManager, авто-коннекты при создании Observer

---

## Граница области видимости данного SKILL

Данный SKILL описывает **целевое состояние PDM/ERP ядра** без:
- Интеграции с внешними компаниями (генеративная подсистема интеграции — не реализуется)
- ИИ-оформления чертежей
- Полноценного UI (виджеты подсистем реализуются отдельно)
- Полноценных сетевых классов (ServerConnector/KafkaConnector/MinioConnector — только заглушки на этапе разработки)
