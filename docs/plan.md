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

Used before `FULL_SYNC`. Session teardown is a separate DataManager reset flow.

1. Open transaction.
2. Call `ClearData` for each executor.
3. Preserve service structures and migration tables if policy requires it.
4. Commit transaction.
5. Emit `signalResourcesCleared`.

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

Observer implementation decisions:

- `IDataObserver` is removed. Subscribers/widgets do not inherit a DataManager
  observer class.
- Subscribers own `DataAdapter` members and connect each adapter's
  `signalDataUpdated` to the subscriber slot that handles that resource class.
- `SingleResourceAdapter` stores one resource and subscribes with
  subsystem/generative context/resource type/resource id.
- `VectorResourceAdapter` stores a vector of resources and subscribes with
  subsystem/generative context/resource type.
- Adapter construction/subscription may synchronously fill stored data, but
  initial fill must not emit update signals.
- DataManager keeps guarded non-owning adapter references indexed as
  `std::multimap`s: exact resource keys for single adapters, and `AdapterKey`
  for vector adapters.
- ConfigManager `signalSubsystemAdded` tells DataManager which subsystem and
  generative contexts exist for the current user; DataManager creates/removes
  those subscription contexts.
- After successful executor CUD, DataManager notifies matching single-resource
  adapters first, then matching vector adapters.
- For delete, adapters receive `updateData(nullptr, CrudAction::DELETE, ...)`;
  single-resource adapters clear their resource, while vector adapters remove
  only the matching element.
- For create/update, vector adapters update one element by id rather than
  replacing the whole vector.
- This step does not change executor read/list APIs. After successful CUD,
  DataManager updates adapters from the already processed `UniterMessage`
  resource payload; vector list initialization remains adapter/user-flow
  responsibility until executor list reads are stabilized.

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

	Обработка enum-классов
		- Под enum class, который предваряется комментарием "// CodeGen need" создавать std::array<std::pair<int, std::string>>, (начинается через строку после последней строки enum class)
		- Если генерация производится в первый раз, то комментарий // CodeGen need"  заменяется на "*// CodeGen version: 1. Need update (yes/no): no"
		- Для повторной генерации нужно "no" заменить на "yes"
		- А сгенерированный std::array должен предварять комментарий "// CodeGen from %enum_name% version 1."
		- Если CodeGen находит enum с "Need update (yes/no): yes", то перегенерирует enum, инкрементируя версию.
		- Запускается с указанием директории, и обрабатывает все файлы с расширением .h .hpp с рекурсивным поиском
		- Имя std::array формируется как название enum class только с добавлением Pair как суффикс, а формирование структуры
			*constexpr std::array<std::pair<int, std::string>, {кол-во вариантов}> {EnumName}Pair = {{*
				*{{static_cast\<int>(EnumName::first_name), "{first_name}"}},*
				*{{static_cast\<int>(EnumName::second_name), "{second_name}"}},*
				*// ...*
			*}};*
	
      Обработка sql инструкций
		- Каждая обрабатываемая инструкция внутри .sql файла должна быть предварена комментарием "--CodeGen need for {name}."
		- В инструкции, если в какой то строке нужно убрать какое-то конкретное значение (например value или условие выборки), то в этой строке нужно написать в конце комментарий --replacable: ... и список того, что нужно заменять через запятую
		- Если CodeGen видит такую инструкцию, отмеченную с помощью "--CodeGen need for {name}.", то он создает переменную static constexpr const char* {name}, при этом заменяя конкретные replacable на строку "%VAL%
		- После первой генерации CodeGen заменяет м"--CodeGen need for {name}." на "--CodeGen: {name} version 1. Need update (yes/no): no
		- CodeGen получает в аргументах командной строки две директории, первая из которой он берет файлы, а вторая где создает файлы .h с переменными. При этом он зеркалит название файлов и структуру каталогов при создании файлов.

## 12. Work Order

Completed or already established:

1. [x] Validate and decompose directory structure.
2. [x] Define minimal `IDataBase`.
3. [x] Add `SqliteDataBase` with open/transaction/user-context boundary.
4. [x] Clarify executor lifecycle interface.
5. [x] Move SQL DDL/DML to separated `.sql` files within subsystems.
6. [x] Implement DataManager initialization, cleanup and executor routing level.

Next:

7. [x] Realize DataManager API observers with notifications, stable DataManager.
8. [ ] Add DataManager observer mechanism tests before stabilizing `ManagerExecutor`.
9. [ ] Stabilize `ManagerExecutor` as the first real executor.
10. [ ] Add schema migration resource model in a service/system subsystem.
11. [ ] Add basic tests for manager subsystem CRUD and DataManager routing.
12. [ ] Implement actual user filtering in executor DDL/DML.
13. [ ] Implement CodeGen for enums and SQL catalogs.
14. [ ] Realize other Executors

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
