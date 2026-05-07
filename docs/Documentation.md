# Uniter Documentation

## Purpose

Uniter is a client-server PDM/ERP system for machine-building and project/production management.
The shared domain and persistence layers are reused by both client and server.

This file is the single active source of truth for architecture, protocol conventions, persistence direction, and the current target state.

## Current Direction

- Shared layers:
  - `src/uniter/contract/` is the common domain model.
  - `src/uniter/database/` is the common persistence layer.
  - Both layers use only standard C++ and must not depend on Qt.
- Client layers:
  - UI, widgets, Qt adapters, and presentation logic stay outside the shared model.
- Server layers:
  - network, broker integration, and tenant context stay outside the shared model.

The same resource model and the same persistence executors must work on both client and server.
The only difference is the concrete database descriptor and backend context.

## Shared Contract

The contract layer defines the shared resources and protocol types.

- Use `std::string`, `std::vector`, `std::map`, `std::optional`, and `std::chrono`.
- No Qt types in shared headers.
- Resources are grouped by subsystem:
  - `DESIGN`
  - `MATERIAL`
  - `INSTANCE`
  - `PDM`
  - `DOCUMENTS`
  - `MANAGER`
  - `PURCHASE`
  - `PRODUCTION`
  - `INTEGRATION`
- Each resource keeps identity metadata:
  - `subsystem`
  - `gen_subsystem`
  - `resource_type`
- Resources must support:
  - default construction
  - construction with known primary key
  - construction without primary key

The contract model is the source of truth for ODB annotations and persistence mapping.

## ODB-First Database Layer

`src/uniter/database/` is the shared persistence layer and is implemented on top of ODB.

For each subsystem, the database layer exposes a dedicated executor/repository class that works with that subsystem's resources.
Each class receives a concrete database descriptor and uses the same interface on SQLite or PostgreSQL.

Minimum API for each subsystem executor:

- `create(resource_without_pk)`
- `create_with_pk(resource_with_known_pk)`
- `get(id)`
- `exists(id)`
- `update(resource)`
- `remove(id)`
- `list(...)` / `find(...)` with subsystem-specific filters

This executor is not a raw SQL helper.
It is the persistence facade over ODB-generated code and backend-specific session/transaction handling.

### Database descriptor

Every executor receives a descriptor of the database it works with.
The descriptor represents:

- backend type: SQLite or PostgreSQL
- connection details
- database name or file path
- schema or tenant context

This keeps the architecture identical to the previous executor-based approach while replacing manual mapper logic with ODB-backed executors.

### Server reuse

The same executors must remain usable on the server.

- Local client storage uses SQLite.
- Server storage uses PostgreSQL.
- Multi-tenancy is modeled as a company/user context around the database connection and schema selection.
- Resource classes themselves do not know whether they run on client or server.

## DataManager And AppManager

`AppManager` is the orchestration layer that drives startup and high-level FSM transitions.
`DataManager` owns the local persistence lifecycle and resource access.

During initialization:

- `AppManager` starts the loading phase.
- `DataManager` opens the database through the descriptor.
- `DataManager` checks that required tables exist and match the expected structure.
- Missing tables are created automatically.
- Only after the schema is valid does `DataManager` report that resources are loaded.

`DataManager` also needs an explicit full-clear operation for all resource tables.
That operation is a persistence-layer responsibility, not a UI responsibility.

The implementation may be:

- ordered table cleanup, or
- schema drop/recreate for local storage

The exact backend strategy is documented in `Plan.md`.

## Resource Keys And Subscriptions

The UI subscription system should use typed keys instead of loose ad-hoc identifiers.

- `SubsystemKey` identifies the subsystem.
- `ResourceKey` identifies a concrete resource inside a subsystem.

`DataManager` uses these keys to:

- cache resources
- notify UI subscribers
- route resource updates
- minimize full reloads

This layer comes after the ODB-backed repository layer is stable.

## Protocol And `add_data`

`UniterMessage::add_data` is the canonical parameter map for protocol operations.
It is intentionally flexible and not strongly typed to avoid an explosion of specialized message fields.
Any code that creates or consumes protocol messages must use the published keys below.

### Protocol actions

- `AUTH`
- `GET_KAFKA_CREDENTIALS`
- `GET_MINIO_PRESIGNED_URL`
- `GET_MINIO_FILE`
- `PUT_MINIO_FILE`
- `FULL_SYNC`
- `UPDATE_CHECK`
- `UPDATE_CONSENT`
- `UPDATE_DOWNLOAD`

### Canonical `add_data` keys

- `AUTH REQUEST`
  - `login`
  - `password_hash`
- `GET_KAFKA_CREDENTIALS RESPONSE`
  - `bootstrap_servers`
  - `topic`
  - `username`
  - `password`
  - `group_id`
- `GET_MINIO_PRESIGNED_URL`
  - `object_key`
  - `minio_operation`
  - response: `presigned_url`, `url_expires_at`
- `GET_MINIO_FILE`
  - `presigned_url`
  - `object_key`
  - `expected_sha256` optional
- `PUT_MINIO_FILE`
  - `presigned_url`
  - `object_key`
  - `local_path`
  - `sha256` optional
- `FULL_SYNC REQUEST`
  - no extra parameters
- `UPDATE_CHECK REQUEST`
  - `current_version`
- `UPDATE_CONSENT`
  - `accepted`

`add_data` remains the single source of truth for protocol key naming.
A typed helper/builder can be added later, but it must not change the canonical keys.

## AppManager Routing

`AppManager` is the traffic director for protocol messages.
It decides where each `UniterMessage` goes and does not leave this decision to the network or storage layers.

### Outgoing routing

- `signalSendToServer`
  - auth
  - `GET_KAFKA_CREDENTIALS`
  - `FULL_SYNC`
  - CRUD
  - update-related protocol messages
- `signalSendToMinio`
  - `GET_MINIO_FILE`
  - `PUT_MINIO_FILE`

### Incoming routing

- `signalRecvUniterMessage`
  - forwarded to `DataManager` for CRUD traffic
- `signalForwardToFileManager`
  - forwarded to file-related handling when the protocol action is MinIO-related

### Routing principles

- All incoming traffic enters through `onRecvUniterMessage`.
- All outgoing traffic leaves through `onSendUniterMessage`.
- Network classes do not filter business traffic themselves.
- `AppManager` applies the routing decision based on current app state and message type.

## AppManager FSM

`AppManager` uses an entry-action oriented FSM.
State names reflect what happens on entry, not just abstract labels.

Important states:

- `IDLE`
- `STARTED`
- `AUTHENIFICATION`
- `IDLE_AUTHENIFICATION`
- `DBLOADING`
- `CONFIGURATING`
- `KCONNECTOR`
- `KAFKA`
- `DBCLEAR`
- `SYNC`
- `READY`
- `SHUTDOWN`

State responsibilities:

- `DBLOADING`
  - ask `DataManager` to load resources and bootstrap schema
- `CONFIGURATING`
  - load app/user configuration
- `KCONNECTOR`
  - initialize Kafka connector for the current user
- `KAFKA`
  - verify Kafka offset freshness
- `DBCLEAR`
  - clear database tables before resync when needed
- `SYNC`
  - request full sync from the server
- `READY`
  - normal operational mode

The FSM and routing documents historically split this logic into separate notes.
That separation is now folded into this single document.

## Domain And Persistence Notes

The old database documentation established several important principles that remain valid:

- class layout is not 1:1 with database tables
- some resources are represented by multiple tables
- read-side materialization may collapse several rows into one resource
- write-side persistence may split one resource into multiple rows
- soft delete is part of the persistence model
- join tables are required for M:N relations
- tables and resources should be designed with backend portability in mind

For the new ODB-first direction, those rules still apply.
The difference is that ODB-generated persistence code becomes the implementation mechanism instead of hand-written mapper SQL.

## PDM And DESIGN

The PDM/DESIGN model is still the current business foundation for the system.
The exact legacy geometry/versioning notes are retained here so they are not scattered across multiple files.

### DESIGN overview

- `Project` is the top-level design entity.
- `Assembly` is a node in the assembly tree.
- `Part` is a leaf-level design item.
- `AssemblyConfig` and `PartConfig` represent execution/configuration variants.
- Documents are linked through `Doc` and `DocLink`.
- File metadata lives as object-key and hash fields rather than as eager in-memory file objects.

### PDM overview

- `Snapshot` represents a versioned frozen state.
- `Delta` represents the difference between snapshots.
- `PdmProject` owns the versioning flow.
- `Diagnostic` is the technical/validation side of the PDM layer.

### Legacy notes kept from earlier documentation

- `Project` used to reference `active_snapshot_id`; the current architecture is being reworked around explicit PDM ownership.
- The data model historically used `root_assembly_id`, snapshot chains, and delta chains to preserve version history.
- The current target remains compatible with full sync and versioned history, but the implementation is being moved to the ODB-first shared model.

## Target Subsystems

The shared resource model covers the main business areas:

- `DESIGN` for projects, assemblies, parts, and configs
- `MATERIAL` for templates and material definitions
- `INSTANCE` for concrete material instances
- `PDM` for snapshots, deltas, diagnostics, and project versioning
- `DOCUMENTS` for document links and attachments
- `MANAGER` for employees, plants, permissions, and integration settings
- `PURCHASE` for purchase workflows
- `PRODUCTION` for tasks, stock, and supply
- `INTEGRATION` for external integration tasks

Each subsystem should eventually have its own ODB-backed executor in `src/uniter/database/<subsystem>/`.

## Documentation Consolidation

The following older documentation is consolidated conceptually into this file:

- `docs/Documentation.txt`
- `docs/add_data_convention.md`
- `docs/appmanager_routing.md`
- `docs/fsm_appmanager.md`
- `docs/pdm_design_architecture.md`
- `docs/db/*.md`
- subsystem PDFs in `docs/`

This file is the one to update first when architecture changes.

