---
name: uniter-current-state
description: Актуальное состояние проекта Uniter после консолидации документации в docs/documentation.md и docs/plan.md. Использовать для работ по Qt C++ PDM/ERP, AppManager, DataManager, БД, DESIGN/PDM и протоколу.
---

# SKILL: Uniter — актуальное состояние

## Основной источник правды

Перед работой по Uniter сначала проверяй:

- `docs/documentation.md` — сводная архитектура проекта;
- `docs/plan.md` — план реализации data layer;
- `docs/visual/` — PDF/DWG/BAK visual references.

Старые Markdown-документы удалены; их решения сведены в `documentation.md`.

## Проект

Uniter — Qt C++17 клиент-серверная PDM/ERP система для автоматизации материалов, конструкторской документации, закупок и производства. CMake-проект с `Common`, `Updater`, `Uniter`. Используются Qt, MinGW/MSYS, MuPDF, tinyxml2, GoogleTest.

Подмодули:

- `src/uniter/contract/` — `uniter-contract`;
- `src/uniter/database/` — `uniter-database`.

## Текущая архитектурная модель

Слои:

1. Network: `ServerConnector`, `KafkaConnector`, `MinioConnector`.
2. Application management: `AppManager`, `ConfigManager`.
3. Data management: `DataManager`, `FileManager`, `IDataBase`, executors.
4. Business logic: `PDMManager`, `ERPManager`.
5. UI: `MainWindow`, static/generative/dynamic widgets.

Весь сетевой трафик проходит через `AppManager`. UI и бизнес-логика не должны напрямую ходить в network layer.

## AppManager/FSM

Актуальная FSM из `docs/documentation.md`:

```text
IDLE -> STARTED -> AUTHENIFICATION -> IDLE_AUTHENIFICATION
-> DBLOADING -> CONFIGURATING -> KCONNECTOR -> KAFKA
-> READY
```

Если offset Kafka устарел:

```text
KAFKA -> DBCLEAR -> SYNC -> READY
```

Online/offline — отдельное состояние от `AppState`. Network-состояния повторяют entry-action при reconnect, local-состояния нет.

Routing:

- `PROTOCOL` до `READY` — внутренняя FSM;
- CRUD до `READY` — reject/log;
- CRUD в `READY` — `DataManager`;
- MinIO protocol в `READY` — `FileManager`;
- outgoing CRUD — `ServerConnector`;
- `GET_MINIO_FILE` / `PUT_MINIO_FILE` — `MinioConnector`;
- `GET_MINIO_PRESIGNED_URL` — `ServerConnector`.

## UniterMessage/add_data

`UniterMessage::add_data` — строгий строковый контракт для protocol parameters. Ключи описаны в `docs/documentation.md`.

Важные действия:

- `AUTH`: `login`, `password_hash`;
- `GET_KAFKA_CREDENTIALS`: server/topic/user/group/offset data;
- `GET_MINIO_PRESIGNED_URL`: `object_key`, `minio_operation`, `presigned_url`, `url_expires_at`;
- `GET_MINIO_FILE`: `presigned_url`, `object_key`, `expected_sha256`, `local_path`;
- `PUT_MINIO_FILE`: `presigned_url`, `object_key`, `local_path`, `sha256`;
- `FULL_SYNC`: без обязательных ключей;
- update actions: см. `docs/documentation.md`.

Будущее улучшение: вынести ключи в `contract/add_data_keys.h`.

## Data layer

Актуальный план в `docs/plan.md`.

Целевые границы:

- `IDataBase` — низкоуровневое соединение, SQL, транзакции, user context. Не знает ресурсов.
- `IResExecutor` — DDL/DML/mapping/migrations для одной подсистемы.
- `DataManager` — оркестратор: открывает БД, выставляет user context, регистрирует executors, маршрутизирует CRUD/READ, чистит данные перед `FULL_SYNC`, уведомляет observers.

Целевые поля `DataManager`:

```cpp
std::unique_ptr<database::IDataBase> db_;
std::unordered_map<contract::Subsystem,
                   std::unique_ptr<database::IResExecutor>> executors_;
```

SQLite user isolation:

- один физический локальный DB file;
- `SetUserContext(userHash)`;
- фильтрация через `user_hash`/`local_user_id` или views/triggers;
- executor DML обязан учитывать user context.

Первый executor: `ManagerExecutor`.

Рекомендуемый порядок executors:

1. Manager
2. Documents
3. Materials
4. Instances
5. Design
6. PDM
7. Supply
8. Production
9. Integration

## Database model

Главный принцип:

```text
runtime class != database table
```

DataManager/executors collapse normalized tables into runtime resources on READ and expand resources back into tables on CREATE/UPDATE.

## DESIGN

DESIGN — текущая редактируемая структура изделий.

Ресурсы:

- `Project = 30` -> `design_project`;
- `Assembly = 31` -> `design_assembly`;
- `AssemblyConfig = 33` -> `design_assembly_config` + join tables;
- `Part = 32` -> `design_part`;
- `PartConfig = 34` -> `design_part_config`.

Ключевые решения:

- `design_project` содержит `root_assembly_id` и nullable `pdm_project_id`;
- `active_snapshot_id` удалён из DESIGN;
- `design_assembly` хранит общие данные сборки;
- структура сборки хранится в `design_assembly_config`;
- `design_part` не содержит `assembly_id`, деталь может входить в разные исполнения;
- документы подключаются через `documents_doc_link`.

## PDM

PDM хранит неизменяемые snapshot-ы DESIGN и delta между ними.

Ресурсы:

- `Snapshot = 50` -> `pdm_snapshot`;
- `Delta = 51` -> `pdm_delta`;
- `PdmProject = 52` -> `pdm_project`;
- `Diagnostic = 57` -> `pdm_diagnostic`.

PDM mirror resources:

- `ASSEMBLY_PDM = 53`;
- `ASSEMBLY_CONFIG_PDM = 54`;
- `PART_PDM = 55`;
- `PART_CONFIG_PDM = 56`.

При создании snapshot текущие `design_*` строки копируются в `pdm_*` mirror tables с `snapshot_id`. `pdm_snapshot.root_assembly_id` указывает на `pdm_assembly.id`, не на live `design_assembly.id`.

История — двусвязный список snapshot/delta:

```text
snapshot_1 <-> delta_1 <-> snapshot_2 <-> delta_2 <-> snapshot_3
```

`design_project.pdm_project_id -> pdm_project.head_snapshot_id -> pdm_snapshot` заменяет старый `design_project.active_snapshot_id`.

## Открытые решения

Перед реализацией проверить раздел решений в `docs/documentation.md`.

Главные пункты:

- как передавать `snapshot_id` для PDM mirror CRUD;
- исправить расхождение ResourceType PDM mirrors 35-38 vs 53-56;
- проверить simple/composite instance для standard products в DESIGN;
- убрать legacy `active_snapshot_id` wording;
- синхронизировать PDF-схему DESIGN с `design_part.project_id`;
- обновить устаревшие комментарии про `MATERIAL_INSTANCE = 60`;
- решить, нужен ли `pdm_project.approved_snapshot_id`.

## Рабочее правило

При изменениях в коде сверяйся с `docs/documentation.md` и `docs/plan.md`. Если обнаружен новый конфликт между SQL, C++ и документацией, обнови `docs/documentation.md` вместо создания отдельного Markdown-файла.
