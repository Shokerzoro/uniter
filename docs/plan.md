# Data Layer Implementation Plan

This plan complements `docs/documentation.md` and tracks the practical
implementation path for `DataManager`, `IDataBase`, SQLite, subsystem
executors, raw SQL authoring, CodeGen and migrations.

## 1. Goal

Build the data layer in stages so each subsystem executor can be tested
independently from UI and AppManager.

The first milestone is complete: DataManager opens SQLite, initializes the
active executor registry, routes manager CRUD, can reset local database state,
and has a real lifecycle test. The next milestone is preparing raw SQL
instructions for every subsystem so CodeGen can generate `gen_sql_*` headers
and the empty executors can be filled one by one.

## 2. Responsibility Boundaries

### DataManager

`DataManager` connects AppManager, database access, subsystem executors and UI
observers.

It is responsible for:

1. Initializing for the current user.
2. Opening the local database.
3. Setting user context.
4. Initializing and verifying executors.
5. Routing incoming `UniterMessage` CRUD messages.
6. Routing UI READ requests.
7. Clearing resource data before `FULL_SYNC`.
8. Notifying subscribers after data changes.

Current first-milestone fields:

```cpp
std::unique_ptr<database::IDataBase> db_;
std::vector<std::unique_ptr<database::IResExecutor>> executors_;
```

Current active registry:

```cpp
CommonExecutor
ManagerExecutor
```

Other subsystem executor registrations remain commented in `DataManager`
construction until their raw/generated SQL and lifecycle methods are complete.

### IDataBase

`IDataBase` is the low-level database engine interface. It owns connection
state, transactions and SQL execution, but it does not know business resources
or subsystem schemas.

It provides:

- `Open`;
- `Close`;
- `SetUserContext`;
- SQL execution returning unified `SqlResult`;
- transaction begin/commit/rollback.

SQLite is the first implementation. PostgreSQL and test in-memory
implementations should remain possible behind the same interface.

### IResExecutor

`IResExecutor` is the common lifecycle interface for one subsystem executor.
It receives `IDataBase&` for every operation and owns DDL, DML, migrations and
mapping rules for its subsystem.

Minimum lifecycle:

- `Subsystem()`;
- `Initialize(IDataBase&)`;
- `Verify(IDataBase&)`;
- `ApplyMigrations(IDataBase&)`;
- `ClearData(IDataBase&)`;
- `DropStructures(IDataBase&)`.

Concrete executors expose CRUD methods while mappings are still subsystem
specific. A later interface pass may add common `Create/Read/Update/Delete` or
`HandleMessage` virtual methods once all resource contracts stabilize.

## 3. SQL and CodeGen Organization

Current layout:

```text
src/uniter/database/
  common/
    commondomains.h
    commonexecutor.h
    raw_sql_common/
    gen_sql_common/
  manager/
    managerdomains.h
    managerexecutor.h/.cpp
    raw_sql_manager/
    gen_sql_manager/
  {subsystem}/
    {subsystem}domains.h
    {subsystem}executor.h/.cpp
    raw_sql_{subsystem}/
    gen_sql_{subsystem}/
```

Rules:

- `raw_sql_*` is the readable SQL source maintained in the DataGrip side
  project.
- `gen_sql_*` is CodeGen output only and contains generated
  `static constexpr const char*` SQL literals.
- Enum-backed table domains are not stored in raw/gen SQL folders.
- Table domains are generated from contract/project
  `std::array<std::pair<int, std::string>, N>` values in root-level
  `{subsystem}domains.h` files.

Every subsystem should provide these raw SQL files:

- `tables.sql`;
- `create.sql`;
- `read.sql`;
- `update.sql`;
- `delete.sql`;
- `clear.sql`;
- `drop.sql`;
- `verify.sql`;
- `migrations.sql` only if that subsystem owns temporary local migration logic.

Required content for each file:

| File | Content |
|---|---|
| `tables.sql` | Subsystem table/view/trigger/index DDL. Initialization must be idempotent. Enum/domain tables generated from C++ arrays do not belong here. |
| `create.sql` | Statements that create synced resources, including explicit-id inserts for full sync/server data and auto-id variants only when local drafts need them. |
| `read.sql` | Exact-resource and list/context SELECTs that collapse normalized tables into runtime resources. |
| `update.sql` | Statements for mutable fields and owned child-row replacement/update logic. |
| `delete.sql` | Resource delete or soft-delete statements according to the table's sync semantics. |
| `clear.sql` | Full-sync data cleanup in child-to-parent FK-safe order. Preserve domains, migrations and service structures. |
| `drop.sql` | Full reset/test structure drops in dependency-safe order. Use `DROP ... IF EXISTS`. |
| `verify.sql` | Schema metadata checks only: required tables, columns, indexes, triggers and views. Empty tables must pass. |
| `migrations.sql` | Optional temporary local migration helpers until migrations are distributed as resources. |

Raw SQL files are the DataGrip-readable source of truth. Only statements marked
for CodeGen should be emitted to `gen_sql_*`; unmarked helper queries and notes
may remain in raw files.

## 4. User Isolation

SQLite has no native schemas or users. The accepted client model is:

- one physical local database file;
- `IDataBase::SetUserContext(userHash)` sets active logical user;
- executors make resource data user-scoped;
- all DML filters by active user context.

Do not create per-user table names inside one SQLite file. SQLite table names
share one namespace, so this is not a clean schema substitute.

## 5. DataManager Initialization Flow

Implemented first-milestone flow:

1. Create `SqliteDataBase`.
2. Resolve the local database path from `TEMP_DIR` or application environment.
3. Open the database.
4. Call `SetUserContext(userHash)`.
5. Initialize registered executors in dependency order.
6. Verify registered executors in dependency order.
7. Keep only `CommonExecutor` and `ManagerExecutor` active until the remaining
   executor SQL is filled; other executor registrations stay as commented
   placeholders.
8. Enter `LOADED` state and emit `signalResourcesLoaded(true)`.

Verification means schema validation only. `Verify(IDataBase&)` must check that
all tables required by the executor exist and expose the expected structure.
It must not select resource rows or require tables to contain data. Empty tables
created from correct DDL are valid. `verify.sql` files should contain metadata
queries (`pragma_table_info`, `sqlite_master`, or equivalent), and executor
code should fail with `SchemaInvalid` only when schema is missing or malformed.

If verification fails during `DBState::LOADING`, `DataManager` enters `ERROR`
and emits `signalResourcesLoaded(false)`. `AppManager` records that local DB
load failed, lets configuration finish, skips `KCONNECTOR`, and enters
`DBCLEAR` to recreate the DB from zero before `FULL_SYNC`.

Next extension:

1. Uncomment subsystem executor registration as each executor receives complete
   raw/generated SQL.
2. Route all subsystem CRUD through registered executors.

## 6. Data Clearing Flow

Implemented first-milestone flow:

1. Enter `CLEARING` state.
2. Close and remove the current local DB file if present.
3. Recreate the DB, initialize common and manager structures, and verify schema.
4. Return to `LOADED`.
5. Emit `signalResourcesCleared`.

`DBCLEAR` also asks `KafkaConnector` to forget the saved offset for this user.
After `SYNC`, `AppManager` enters `READY` and calls
`signalSubscribeKafka(offset)` with the server-provided actual offset from
`UniterMessage::add_data["offset"]`.

Next extension:

1. Call `ClearData` for every registered executor in reverse dependency order.
2. Preserve domains, service structures and migration tables.
3. Make the full clear operation transactional once all executors participate.

## 7. CRUD Routing and Observers

Incoming network messages:

```text
UniterMessage
 -> DataManager
 -> choose executor by Subsystem / GenSubsystem / ResourceType
 -> executor performs DML
 -> DataManager notifies subscribers
```

UI READ requests use shared contract keys:

```cpp
struct SubsystemKey {
    contract::Subsystem subsystem;
    contract::GenSubsystem genSubsystem;
    std::optional<uint64_t> genId;
    contract::ResourceType resourceType;
};

struct ResourceKey {
    SubsystemKey subsystemKey;
    std::optional<uint64_t> resourceId;
};
```

Implemented observer decisions:

- Subscribers own `DataAdapter` members.
- `SingleResourceAdapter` stores one resource.
- `VectorResourceAdapter` stores a vector of resources.
- Initial adapter fill does not emit update signals.
- DataManager keeps guarded non-owning adapter references indexed by
  `ResourceKey` and `SubsystemKey`.
- After successful CUD, DataManager notifies matching single adapters, then
  matching vector adapters.
- For delete, adapters receive `updateData(nullptr, CrudAction::DELETE, ...)`.
- For create/update, vector adapters update one element by id.

Hierarchical/tree subscription remains postponed until the design tree model is
stable.

DataManager provides two types of subscription for single resource and for resource vector.
Resource vector subscription should returns vector filled only with AbstractResources (basic info) 
without internal classes linkage otherwise it can cause all DB data injects through vector subscription.
So execitors must have method which returns vector with abstract resources.

## 8. ManagerExecutor

`ManagerExecutor` is the first real worker executor because manager data is
required for authentication, permissions, plants and integrations.

Current implemented scope:

- `Employee`;
- `Plant`;
- employee assignments;
- permissions;
- employee-assignment links.

Pending manager scope:

- `Integration` runtime class and CRUD routing;
- user-context filtering in DML;
- replacement of embedded SQL constants with generated SQL headers after
  CodeGen is ready.

Tables:

- `manager_employees`;
- `manager_assignments`;
- `manager_assigments_permissions`;
- `manager_employee_assignments`;
- `manager_plants`;
- `manager_integrations`.

Implementation status:

1. [x] Wire DDL/domain initialization into `Initialize`.
2. [x] Implement `Verify`.
3. [x] Implement `ClearData`.
4. [x] Route `Employee` CRUD.
5. [x] Route `Plant` CRUD.
6. [x] Add assignments/permissions/link CRUD for employees.
7. [x] Add DataManager lifecycle test for manager CRUD/init/clear.
8. [ ] Add focused direct `ManagerExecutor` tests.
9. [ ] Add `Integration` resource class and CRUD.
10. [ ] Switch embedded SQL constants to generated SQL headers.

## 9. Subsystem Executor Order

Recommended order after `ManagerExecutor`:

1. `DocumentsExecutor`
2. `MaterialExecutor`
3. `InstanceExecutor`
4. `DesignExecutor`
5. `PdmExecutor`
6. `SupplyExecutor`
7. `ProductionExecutor`
8. `IntegrationExecutor`

Reasoning:

- DOCUMENTS is reused by materials/design/supply.
- MATERIALS and INSTANCES are dependencies for DESIGN, SUPPLY and PRODUCTION.
- DESIGN must exist before PDM.
- PDM must exist before production tasks can reliably reference snapshots.

## 10. Raw SQL Preparation

This is the current main work item.

For each subsystem:

1. Fill `tables.sql` with normalized table/view/trigger/index DDL.
2. Fill `create.sql`, `read.sql`, `update.sql`, `delete.sql` with CRUD
   instructions for each resource table or collapsed runtime resource.
3. Fill `clear.sql` with data cleanup in FK-safe order.
4. Fill `drop.sql` with structure drop in FK-safe order.
5. Fill `verify.sql` with metadata-only schema checks.
6. Add CodeGen comments only to SQL instructions that should be emitted.
7. Keep table-domain DDL and inserts in `{subsystem}domains.h`, not raw SQL.
8. Keep DataGrip-friendly formatting and avoid C++ string escaping in raw SQL.

## 11. CodeGen

CodeGen has two responsibilities: enum table-domain preparation and SQL literal
generation.

Enum/table-domain generation:

- An enum class marked with `// CodeGen need` is transformed into
  `std::array<std::pair<int, std::string>, N>`.
- After first generation the marker becomes
  `// CodeGen version: 1. Need update (yes/no): no`.
- Regeneration is requested by changing `Need update` to `yes`; CodeGen then
  regenerates the array and increments the version.
- The generated array is preceded by
  `// CodeGen from {EnumName} version {version}.`
- Array naming follows `{EnumName}Pairs`.

Generated array shape:

```cpp
inline std::array<std::pair<int, std::string>, N> EnumNamePairs = {{
    {static_cast<int>(EnumName::FIRST), "FIRST"},
    {static_cast<int>(EnumName::SECOND), "SECOND"},
}};
```

SQL generation:

- DataGrip-managed raw SQL lives under `raw_sql_{subsystem}`.
- Each emitted instruction is preceded by
  `-- CodeGen need for {name}.`
- Lines that need concrete SQL fragments replaced for C++ templates use a
  trailing comment: `-- replaceable: value1, value2`.
- Replaceable fragments become `%VAL%` in generated literals.
- CodeGen emits:

```cpp
static constexpr const char* name = R"SQL(
...
)SQL";
```

- After first generation the raw marker becomes
  `-- CodeGen: {name} version 1. Need update (yes/no): no`.
- CodeGen receives an input raw SQL directory and an output generated SQL
  directory. It mirrors file names and directory structure.

CodeGen implementation should start after subsystem raw SQL names and comments
are stable.

## 12. Migrations

Migrations are not local ad-hoc subsystem `migrations.sql` files in the final
model.

Target model:

- migrations are resources of a future service/system subsystem;
- server distributes them through `UniterMessage`;
- DataManager orchestrates migration order;
- subsystem executors apply their own schema changes;
- a common accounting table records applied migrations.

Suggested accounting table:

```text
system_migrations(
  target_subsystem,
  version,
  name,
  checksum,
  applied_at
)
```

Migration requirements:

- idempotent;
- transactional;
- checksum-verified;
- applied before regular CRUD for the required schema version.

## 13. Work Order

Completed:

1. [x] Validate and decompose directory structure.
2. [x] Define minimal `IDataBase`.
3. [x] Add `SqliteDataBase` with open/transaction/user-context boundary.
4. [x] Clarify executor lifecycle interface.
5. [x] Move raw SQL files into `raw_sql_*` directories.
6. [x] Move table-domain builders into root-level `*domains.h` files.
7. [x] Add root-level `*domains.h` placeholders for every subsystem.
8. [x] Add empty executor classes for every subsystem.
9. [x] Implement DataManager initialization, cleanup and manager routing.
10. [x] Realize DataManager API observers with notifications.
11. [x] Add DataManager observer tests.
12. [x] Stabilize manager Employee/Plant CRUD enough for lifecycle testing.
13. [x] Add DataManager lifecycle test for DB init, manager CRUD and clear.

Next:

14. [x] Implement CodeGen for SQL catalogs.
15. [x] Implement CodeGen for enum arrays where still missing.
16. [x] Realize "light weighted" vector subsrcription

17. [ ] Prepare raw SQL instructions for every subsystem.
18. [ ] Add CodeGen comments to all emitted SQL instructions.
19. [ ] Replace embedded manager SQL constants with generated SQL headers.
20. [ ] Fill subsystem executors from generated SQL and resource mapping.
21. [ ] Add direct executor tests per subsystem.
22. [ ] Add schema migration resource model in a service/system subsystem.
23. [ ] Implement actual user filtering in executor DDL/DML.

## 14. Test Plan

Covered by current tests:

- [x] DataManager opens SQLite in a moved/mocked temp location.
- [x] Common domains initialize before manager tables.
- [x] Manager DDL initializes required tables.
- [x] DataManager routes manager `Employee` create/read/update/delete.
- [x] `ClearData` removes manager resource data.
- [x] Subscriber notification fires after successful mutation.

Remaining tests:

- [ ] Direct SQLite open/close tests.
- [ ] Direct transaction commit/rollback tests.
- [ ] Direct user-context visibility tests.
- [ ] Direct `ManagerExecutor` tests for `Employee` and `Plant`.
- [ ] Subsystem executor tests as each raw SQL set is completed.
