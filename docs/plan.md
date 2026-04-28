# Data Layer Implementation Plan

This plan is the consolidated data-layer implementation plan. It was formed
from the former database planning notes and now complements
`docs/documentation.md`. It describes the next implementation path for
`DataManager`, `IDataBase`, SQLite support, subsystem executors, SQL
organization, migrations and CodeGen.

## 1. Goal

Build the data layer in stages so that each subsystem executor can be tested
independently from UI and AppManager.

The first milestone is a working database boundary with one real subsystem
executor. After that, the remaining subsystems can be added by repeating the
same executor pattern.

## 2. Responsibility Boundaries

### DataManager

`DataManager` is the application component that connects AppManager, database
access, executors and UI observers.

It must:

1. Initialize for the current user.
2. Open the local database.
3. Set user context.
4. Register subsystem executors.
5. Initialize/check executor structures.
6. Route incoming `UniterMessage` CRUD messages.
7. Route UI READ requests.
8. Clear resource data before `FULL_SYNC`.
9. Notify subscribers after data changes.

Target fields:

```cpp
std::unique_ptr<database::IDataBase> db_;
std::unordered_map<contract::Subsystem,
                   std::unique_ptr<database::IResExecutor>> executors_;
```

### IDataBase

`IDataBase` is the low-level database engine interface. It owns connection
state, transactions and SQL execution, but it does not know business resources
or subsystem schemas.

It must provide:

- `Open`;
- `Close`;
- `SetUserContext`;
- SQL execution returning unified `SqlResult`;
- transaction begin/commit/rollback;
- minimal engine-specific helpers needed by executors.

SQLite is the first implementation. PostgreSQL and test in-memory
implementations should remain possible behind the same interface.

### IResExecutor

`IResExecutor` is the common interface for one subsystem executor. It receives
`IDataBase&` for every operation and owns all DDL, DML, migrations and mapping
rules for its subsystem.

Minimum interface:

- `Subsystem()`;
- `Initialize(IDataBase&)`;
- `Verify(IDataBase&)`;
- `ApplyMigrations(IDataBase&)`;
- `ClearData(IDataBase&)`;
- `DropStructures(IDataBase&)`;
- `Create`;
- `Read`;
- `Update`;
- `Delete`;
- `HandleMessage(IDataBase&, const UniterMessage&)`.

Executor methods return `ExecutorResult`: status, error code, message and
optional resource/list payload.

## 3. DDL and DML Separation

DDL and DML must be stored and maintained separately.

| Category | Purpose | Owner |
|---|---|---|
| DDL | `CREATE TABLE`, `CREATE INDEX`, `ALTER TABLE`, migrations | subsystem executor |
| DML | `INSERT`, `SELECT`, `UPDATE`, soft delete, joins | subsystem executor |

This keeps subsystem schema, queries and C++ mapping close to each other.

Recommended layout:

```text
src/uniter/database/
  common/
  sqlite/
  executors/
    manager/
      ddl/
      dml/
    materials/
      ddl/
      dml/
    design/
      ddl/
      dml/
```

## 4. User Isolation

SQLite has no native schemas or users. The accepted client model is:

- one physical local database file;
- `IDataBase::SetUserContext(userHash)` sets active logical user;
- executors make their tables user-scoped;
- all DML filters by active user context.

Practical implementation options:

1. Add `user_hash` or `local_user_id` to resource tables.
2. Use connection-local views/triggers if needed.
3. Keep filtering inside executor DML as the first implementation.

Do not create per-user table names inside one SQLite file. SQLite table names
share one namespace, so this is not a clean schema substitute.

## 5. DataManager Initialization Flow

1. Create `SqliteDataBase`.
2. Resolve the common local database path.
3. Open the database.
4. Call `SetUserContext(userHash)`.
5. Create/register subsystem executors.
6. Call `Initialize` for each executor.
7. Call `Verify` for each executor.
8. Enter `LOADED` state and notify AppManager.

## 6. Data Clearing Flow

Used before `FULL_SYNC` or session reset.

1. Open transaction.
2. Call `ClearData` for each executor.
3. Preserve service structures and migration tables if policy requires it.
4. Commit transaction.
5. Emit `signalDatabaseCleared`.

On failure, rollback and report an error to AppManager.

## 7. CRUD Routing

Incoming network messages:

```text
UniterMessage
 -> DataManager
 -> choose executor by Subsystem / GenSubsystem / ResourceType
 -> executor.HandleMessage(...)
 -> executor performs DML
 -> DataManager notifies subscribers
```

UI READ requests should not require a full `UniterMessage`. Use a `ResourceKey`:

```cpp
struct ResourceKey {
    contract::Subsystem subsystem;
    contract::GenSubsystem genSubsystem;
    std::optional<uint64_t> genId;
    contract::ResourceType resourceType;
    std::optional<uint64_t> resourceId;
};
```

Supported subscriptions:

- list by subsystem/resource type;
- specific resource by id.

Hierarchical/tree subscription remains postponed until the design tree model is
stable.

## 8. First Executor: ManagerExecutor

`ManagerExecutor` is the first worker executor because the manager subsystem is
required for authentication, permissions, plants and integrations.

Initial scope:

- `Employee`;
- `Plant`;
- `Integration`;
- employee assignments;
- permissions;
- employee-assignment links.

Tables:

- `manager_employee`;
- `manager_employee_assignment`;
- `manager_permissions`;
- `manager_employee_assignment_link`;
- `manager_plant`;
- `manager_integration`.

Implementation order:

1. Wire existing DDL initialization into `Initialize`.
2. Implement `Verify`.
3. Implement `ClearData`.
4. Route `Employee` CRUD.
5. Route `Plant` CRUD.
6. Add assignments/permissions/link CRUD.
7. Add focused tests.

## 9. Subsystem Executor Order

Recommended order after `ManagerExecutor`:

1. `DocumentsExecutor`
2. `MaterialsExecutor`
3. `InstancesExecutor`
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

## 10. Migrations

Migrations are not a local subsystem-owned `migrations.sql` file.

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

## 11. CodeGen

CodeGen should start only after `IDataBase`, executor interfaces and SQL markup
rules are stable.

First stage:

1. Enum mapping generation.
   - Recursively scan `.h` / `.hpp`.
   - Process only enums marked with a CodeGen token.
   - Generate `inline constexpr std::array<std::pair<int, std::string_view>, N>`.
   - Use generated-block markers for safe regeneration.

2. SQL constants generation.
   - Input: subsystem raw SQL directories.
   - Output: generated SQL catalog.
   - SQL statements must have explicit names.
   - Use named placeholders like `:param_name`.
   - Generator can emit parameter lists and replace placeholders as needed.

## 12. Work Order

Completed or already established:

1. [x] Validate and decompose directory structure.
2. [x] Define minimal `IDataBase`.
3. [x] Add `SqliteDataBase` with open/transaction/user-context boundary.
4. [x] Clarify executor lifecycle interface.
5. [x] Move SQL DDL/DML to separated `.sql` files within subsystems.
6. [x] Implement DataManager initialization, cleanup and executor routing level.

Next:

7. [ ] Stabilize `ManagerExecutor` as the first real executor.
8. [ ] Add schema migration resource model in a service/system subsystem.
9. [ ] Add basic tests for manager subsystem CRUD and DataManager routing.
10. [ ] Implement actual user filtering in executor DDL/DML.
11. [ ] Implement CodeGen for enums and SQL catalogs.
12. [ ] Add `DocumentsExecutor`.
13. [ ] Add `MaterialsExecutor` and `InstancesExecutor`.
14. [ ] Add `DesignExecutor`.
15. [ ] Resolve PDM mirror `snapshot_id` transfer convention.
16. [ ] Add `PdmExecutor`.
17. [ ] Add `SupplyExecutor`.
18. [ ] Add `ProductionExecutor`.
19. [ ] Add `IntegrationExecutor`.

## 13. Test Plan

Minimum test coverage for the first data milestone:

- SQLite database opens and closes;
- transactions commit and rollback;
- user context is set and visible to executor queries;
- manager DDL initializes all expected tables;
- `Employee` create/read/update/delete;
- `Plant` create/read/update/delete;
- `DataManager` routes manager CRUD to `ManagerExecutor`;
- `ClearData` removes user resource data and preserves required service tables;
- subscriber notification fires after successful mutation.
