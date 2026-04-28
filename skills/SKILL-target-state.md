---
name: uniter-target-state
description: Целевое состояние Uniter как Qt C++ PDM/ERP системы: слои, протокол, FSM, DataManager/IDataBase/executors, FileManager, DESIGN/PDM, PDMManager и ERPManager.
---

# SKILL: Uniter — целевое состояние

## Цель системы

Uniter — клиент-серверная Qt C++17 PDM/ERP система для инженерных и производственных компаний. Она автоматизирует:

- выпуск и версионирование конструкторской документации;
- структуру изделий и материалов;
- закупки;
- склад и производственные задания;
- локальную аналитику потребности в материалах.

Архитектурный принцип: все данные синхронизируются как CRUD над ресурсами. Сложные бизнес-процессы раскладываются на операции над ресурсами и файлами.

## Слои

```text
UI
Business Logic: PDMManager, ERPManager
Data Management: DataManager, FileManager, IDataBase, executors
Application Management: AppManager, ConfigManager
Network: ServerConnector, KafkaConnector, MinioConnector
```

Правило зависимостей:

- Network общается вверх только через AppManager.
- UI отправляет сообщения через AppManager.
- DataManager не знает UI.
- Business logic использует DataManager/FileManager как API.

## Network

`ServerConnector`:

- TCP/SSL;
- AUTH;
- CRUD requests;
- `FULL_SYNC`;
- Kafka credentials;
- MinIO presigned URL;
- update protocol.

`KafkaConnector`:

- только consumer;
- получает `NOTIFICATION` и `SUCCESS`;
- хранит offset в OS Secure Storage по user hash;
- инициирует full sync при stale/missing offset.

`MinioConnector`:

- HTTP GET/PUT/DELETE по presigned URL;
- не хранит permanent credentials;
- решает только атомарные file transfer задачи.

## AppManager/FSM

Целевая FSM:

```text
IDLE
 -> STARTED
 -> AUTHENIFICATION
 -> IDLE_AUTHENIFICATION
 -> DBLOADING
 -> CONFIGURATING
 -> KCONNECTOR
 -> KAFKA
 -> READY
```

Если offset Kafka устарел:

```text
KAFKA -> DBCLEAR -> SYNC -> READY
```

Routing:

- `PROTOCOL` before `READY` -> FSM;
- CRUD before `READY` -> reject/log;
- CRUD in `READY` -> DataManager;
- MinIO protocol in `READY` -> FileManager;
- outgoing CRUD -> ServerConnector;
- `GET_MINIO_PRESIGNED_URL` -> ServerConnector;
- `GET_MINIO_FILE` / `PUT_MINIO_FILE` -> MinioConnector.

FSM enter-actions may send protocol requests directly to `signalSendToServer` so that `onSendUniterMessage` remains the UI/subsystem gate.

## UniterMessage

`UniterMessage` is the universal message container.

Core dimensions:

- `subsystem`;
- `genSubsystem`;
- `genSubsystemId`;
- `resourceType`;
- `crudact`;
- `protact`;
- `status`;
- `error`;
- `resource`;
- `add_data`.

Status semantics:

- `REQUEST` — client request;
- `RESPONSE` — direct server response;
- `ERROR` — processing error;
- `NOTIFICATION` — Kafka change from another user;
- `SUCCESS` — Kafka confirmation of own CUD.

`add_data` is a strict protocol map documented in `docs/documentation.md`. Future target: `contract/add_data_keys.h`.

## Data layer target

### IDataBase

Engine/session abstraction:

- open/close;
- `SetUserContext(userHash)`;
- execute SQL and return `SqlResult`;
- transactions;
- minimal backend helpers.

It must not know resources, subsystem schemas or migrations of concrete subsystems.

### IResExecutor

One executor per subsystem. Owns:

- DDL;
- DML;
- resource mapping;
- migrations;
- clear/drop/verify operations;
- CRUD routing for its resources.

Executor lifecycle:

- `Subsystem()`;
- `Initialize(IDataBase&)`;
- `Verify(IDataBase&)`;
- `ApplyMigrations(IDataBase&)`;
- `ClearData(IDataBase&)`;
- `DropStructures(IDataBase&)`;
- `Create/Read/Update/Delete`;
- `HandleMessage(IDataBase&, const UniterMessage&)`.

### DataManager

Responsibilities:

- create/open DB;
- set user context;
- register executors;
- initialize/verify executors;
- route incoming CRUD;
- serve UI READ through `ResourceKey`;
- clear data before full sync;
- notify subscribers.

Use indexed subscriptions by:

- subsystem;
- gen subsystem/context;
- resource type;
- optional resource id.

Mandatory subscription types:

- resource list;
- specific resource.

Tree observer is deferred until the tree model is stable.

## SQLite user isolation

Target model:

- one physical local DB file;
- logical user context via `SetUserContext(userHash)`;
- user-scoped data in tables through `user_hash`/`local_user_id` or views/triggers;
- all executor DML must filter by current user.

## Migrations

Target model:

- migrations are resources of a service/system subsystem;
- distributed by server via `UniterMessage`;
- DataManager orchestrates;
- executors apply subsystem-specific schema changes;
- applied migrations recorded in `system_migrations`;
- CRUD is processed only after required schema version.

Migrations must be idempotent, transactional and checksum-verified.

## Database modeling rule

Runtime classes are collapsed objects. Database tables are normalized.

```text
runtime class != database table
```

DataManager/executors collapse tables on read and expand runtime objects on write.

## Subsystems

### DOCUMENTS

- `DOC_LINK = 91` -> `documents_doc_link`;
- `DOC = 90` -> `documents_doc`.

`DocLink` is a folder. `Doc` stores MinIO metadata. One `Doc` belongs to one `DocLink`.

### MATERIALS / INSTANCES

- `MATERIAL_TEMPLATE_SIMPLE = 20`;
- `MATERIAL_TEMPLATE_COMPOSITE = 21`;
- `SEGMENT`;
- `SEGMENT_VALUE`;
- `TEMPLATE_COMPATIBILITY`;
- `MATERIAL_INSTANCE_SIMPLE = 61`;
- `MATERIAL_INSTANCE_COMPOSITE = 62`.

Templates define standards and segments. Instances specialize templates and carry quantities.

### MANAGER

- `EMPLOYEES = 10`;
- `PRODUCTION = 11` (`Plant`);
- `INTEGRATION = 12`;
- `EMPLOYEE_ASSIGNMENT`;
- `PERMISSION`;
- `EMPLOYEE_ASSIGNMENT_LINK`.

Employee collapses assignments and permissions at runtime.

### DESIGN

Current editable product structure.

- `PROJECT = 30` -> `design_project`;
- `ASSEMBLY = 31` -> `design_assembly`;
- `PART = 32` -> `design_part`;
- `ASSEMBLY_CONFIG = 33` -> `design_assembly_config`;
- `PART_CONFIG = 34` -> `design_part_config`.

Key decisions:

- `Project` points to `root_assembly_id` and nullable `pdm_project_id`;
- no `active_snapshot_id` in DESIGN;
- assembly content is in configs/join tables;
- part can appear in many assembly configs;
- documents are linked through DOCUMENTS.

### PDM

Versioning and immutable frozen copies.

- `SNAPSHOT = 50`;
- `DELTA = 51`;
- `PDM_PROJECT = 52`;
- `ASSEMBLY_PDM = 53`;
- `ASSEMBLY_CONFIG_PDM = 54`;
- `PART_PDM = 55`;
- `PART_CONFIG_PDM = 56`;
- `DIAGNOSTIC = 57`.

Snapshot creation copies current DESIGN rows to PDM mirror tables with `snapshot_id`. `pdm_snapshot.root_assembly_id` points to the frozen `pdm_assembly`.

History is a linked snapshot/delta chain:

```text
base snapshot -> delta -> snapshot -> delta -> head snapshot
```

Open target decision: whether approval uses only `pdm_snapshot.status` or also `pdm_project.approved_snapshot_id`.

### PURCHASES

- `PURCHASE_GROUP = 40` -> complex request;
- `PURCHASE = 41` -> simple request.

Simple purchase references either simple or composite material instance.

### PRODUCTION

Generative subsystem per `Plant`.

- `PRODUCTION_TASK = 70`;
- `PRODUCTION_STOCK = 71`;
- `PRODUCTION_SUPPLY = 72`.

Production task keeps:

- `snapshot_original_id`;
- `snapshot_current_id`.

### INTEGRATION

Generative subsystem per manager integration.

- `INTEGRATION_TASK = 80`.

Uses polymorphic resource reference:

- `target_subsystem`;
- `target_resource_type`;
- `any_resource_id`.

DataManager validates this relation.

## FileManager

Download:

1. request presigned URL from server;
2. `MinioConnector::get`;
3. verify SHA-256;
4. notify `IFileObserver`.

Upload:

1. request upload URL;
2. `MinioConnector::put`;
3. update resource metadata through DataManager;
4. notify `IFileObserver`.

Cache files by object key and SHA-256.

## PDMManager

Responsibilities:

- scan PDF directories;
- call drawing compiler;
- build XML snapshot;
- upload documents/XML to MinIO;
- create/update DESIGN resources;
- create PDM snapshots and deltas;
- validate diagnostics;
- approve/rollback versions.

PDMManager owns `XMLDocument` creation and passes only root element to compiler library.

## ERPManager

Responsibilities:

- material requirement calculation;
- compare required material instances with stock/purchases;
- shortage forecasting;
- material balance reports.

ERP must use PDM snapshots when reproducibility matters.

## Implementation warning

Before implementing PDM/DataManager details, check the accepted decisions in `docs/documentation.md`. Do not silently choose between SQL, C++ and documentation conflicts; update the consolidated documentation when a new decision is made.
