# Data layer and database architecture plan

The document fixes the target direction of work on `DataManager`, `IDataBase`, SQLite implementation, executors, SQL files and CodeGen. The goal of the next stage is to first build interfaces and boundaries of responsibility so that after the executor has implemented at least one subsystem, it can be tested independently of the UI and AppManager.

## Top-level responsibility

### DataManager

`DataManager` is an application component that belongs to the client side and connects the AppManager, the network layer, the database and the UI.

He must:

1. Initialize for a specific user.
- Accept `userHash` from AppManager.
- Open a local database file associated with this user.
- Do not mix data from different users.
- Initialize tables through executors of subsystems.
- Raise executors of subsystems.

2. Delete local data upon request from AppManager.
- Clear resource tables before `FULL_SYNC` or when resetting the session.
- Maintain the database structure, service tables and enum domains, if this is the selected policy.
- Report the result to AppManager via `signalDatabaseCleared`.

3. Work with data via CRUD for all resources.
- Parse incoming `UniterMessage`.
- Route it via `Subsystem`/`ResourceType` to the desired executor.
- Perform CUD from Kafka/server connector.
- Perform READ on UI requests.
- Notify subscribers after data changes.

4. Provide API for UI.
- Subscription to a specific resource.
- Subscription to a list of resources.
- Hierarchical subscription should be removed from the mandatory API for now, there is no stable tree model yet.

### IDataBase

`IDataBase` is an abstraction of a specific database engine and a descriptor for an open connection. She does not know the business resources, does not own the subsystem diagram, and does not accept responsibility for the DDL of specific subsystems.

She should:

- open/close connection;
- select physical storage for a specific user;
- execute an SQL statement and return a unified `SqlResult`;
- manage transactions;
- provide minimal engine-specific services that all executors need.

`IDataBase` must be the owner of low-level access to the engine: SQLite, PostgreSQL, test in-memory implementation.

### Executors

Executors - a data management layer for a specific subsystem. They do not open the database, but own the SQL of their subsystem: both DDL and DML.

They:

- get a link to `IDataBase`;
- provide creation/checking of tables of their subsystem;
- ensure data cleaning of their subsystem;
- use DDL instructions of their subsystem;
- use DML instructions;
- map `contract::*` resources in SQL;
- map `SqlResult` back to `contract::*` resources;
- encapsulate the rules of a specific subsystem.

Example: `ManagerExecutor` is responsible for `Employee`, `Plant`, `Integration`, assignments and rights of the manager subsystem.

## Separate DDL and DML

You need to strictly separate:

- DDL - database structure management: `CREATE TABLE`, `CREATE INDEX`, `DROP TABLE`, `ALTER TABLE`, domains, migrations.
- DML - operations with data: `INSERT`, `SELECT`, `UPDATE`, soft delete, join queries.

DDL is used by the executor of the subsystem that owns the tables. This preserves locality: the table structure, CRUD instructions and C++ mapping of one subsystem live nearby and change together.

DML is used by executors.

## User data isolation

The idea of ​​"one db file, but different users see their data" is correct as client local isolation, but SQLite doesn't have built-in users, schemas and `search_path` like PostgreSQL. Therefore, this cannot be implemented only using `IDataBase`, without knowing the tables.

Accepted Model:

- one physical database file;
- `IDataBase::SetUserContext(userHash)` sets the current logical user context;
- executors create DDL so that the data is user-scoped;
- DML executors access their subsystem taking into account the current user context.

For SQLite a practical option is:

1. Physical tables contain a technical column `user_hash` or `local_user_id`.
2. Executor creates real tables and, if necessary, connection-local views/triggers for the current user.
3. `SqlDataBase::SetUserContext()` updates the current connection context.
4. Executor ensures that CRUD only works with the current user context.

The option of having the same table names “as if each user has their own schema” in SQLite can only be emulated through views/triggers or through table prefixes. Simply creating `manager_employees` separately for each user in one SQLite file is not possible: table names in one database namespace are global.

Individual files per user is technically simpler, but this is not the chosen model yet.

## DataManager after preparing interfaces

After `IDataBase` appears and at least one executor, the DataManager should receive the internal fields:

```cpp
std::unique_ptr<database::IDataBase> db_;
std::unordered_map<contract::Subsystem, std::unique_ptr<database::IResExecutor>> executors_;
```

DataManager initialization:

1. Create `SqliteDataBase`.
2. Generate the path to the common local database file.
3. Call `Open(databasePath)`.
4. Call `SetUserContext(userHash)`.
5. Create/register executors.
6. Call DDL initialization of each executor.
7. Go to the `LOADED` state.

Data clearing:

1. Open a transaction.
2. Call data clearing for each executor in a consistent manner.
3. Commit the transaction.
4. Notify AppManager via `signalDatabaseCleared`.

It is better to store subscriptions not in three flat lists, but in indexes:

```cpp
ResourceKey {
    Subsystem subsystem;
    GenSubsystem genSubsystem;
    std::optional<uint64_t> genId;
    ResourceType resourceType;
    std::optional<uint64_t> resourceId;
}
```

And separately:

- subscription to a specific resource;
- subscriptions to a list of resources of a specific type.

Hierarchical subscription should not yet be implemented as a mandatory part of the API. It is better to return it after the appearance of a stable tree model.

## IResExecutor

`IResExecutor` is a common interface for the executor of one subsystem. It does not own the database connection, but receives an `IDataBase&` from the `DataManager` for each call. This is important: `IDataBase` remains the engine/session descriptor, and all table structure, DDL, DML and resource mapping remain within the specific subsystem.

Minimum life cycle of an executor:

- `Subsystem()` - returns the subsystem for which the executor is responsible.
- `Initialize(IDataBase&)` - creates/fills DDL structures of the subsystem.
- `Verify(IDataBase&)` - verifies that the expected structures exist and are accessible.
- `ApplyMigrations(IDataBase&)` - applies migrations of its subsystem.
- `ClearData(IDataBase&)` - deletes user data of the subsystem without necessarily deleting the structure.
- `DropStructures(IDataBase&)` - deletes DDL structures of the subsystem.
- `Create/Read/Update/Delete` - performs CRUD on subsystem resources.
- `HandleMessage(IDataBase&, const UniterMessage&)` - basic routing of a CRUD message to a specific method.

The result of the executor's work is returned via `ExecutorResult`: status, error code, message and optionally one resource or a list of resources. For READ requests without a full `UniterMessage`, a `ResourceKey` is used to allow the UI/DataManager to query a resource by subsystem, type and id.

Migrations should not be the responsibility of `IDataBase`: the base knows neither subsystems, nor tables, nor schema versions. The correct model is orchestration in `DataManager`, execution in a specific executor. Migrations should be a separate service resource of the new subsystem, not `PROTOCOL`: the protocol is responsible for control actions and FSM, not for schema domain data. The migration accounting table can be a general structure of the service subsystem, for example `system_migrations` with the fields `target_subsystem`, `version`, `name`, `checksum`, `applied_at`. The SQL steps themselves remain the property of the executor of the corresponding subsystem.

Now `ManagerExecutor` is adapted to this interface as the first working executor: the lifecycle calls the existing DDL initialization, and CRUD methods route `Employee` and `Plant` to already existing `add/read/update/delete` methods. The SQL logic inside these methods still requires separate stabilization after the DataManager is committed.

## CodeGen

CodeGen is needed after stabilizing the `IDataBase` interfaces, executors and SQL file markup rules.

Minimum first stage:

1. Enum pairs.
- Process `.h` / `.hpp` recursively.
- Search for `enum class` with CodeGen token.
- Generate `inline constexpr std::array<std::pair<int, std::string_view>, N>`.
- Use begin/end markers of the generated block to reliably regenerate.

2. SQL constants.
- Input: raw SQL directory of the subsystem.
- Output: generated SQL catalog of the same subsystem.
- Each SQL statement has a name.
- It is better to write placeholders explicitly as `:param_name`, rather than replacing arbitrary values ​​from the comment.
- The generator replaces `:param_name` with `%VAL%` and can generate an array of parameter names.

## Work order

1. [x] Validate and locally decompose the directory structure.
2. [x] Assert minimal interface `IDataBase`.
3. [x] Add `SqliteDataBase` with opening a common database file, transactions and user context.
4. [x] Clarify the basic interface of the executor: DDL initialization, checking structures, migrations, data cleaning, deleting structures, CRUD routing.
5. [x] Move SQL DDL/DML to separated `.sql` files within subsystems.
6. [x] Implement `DataManager` at the initialization, cleanup and routing level in the executor.
7. Implement `ManagerExecutor` as the first worker executor.
8. Add schema migrations as a separate resource of the new service subsystem, distributed through `UniterMessage`, without local `migrations.sql` in the subsystems.
9. Perform basic testing (one manager subsystem)
10. After this, implement the CodeGen and DataGrip process.
11. Implement actual isolation of user data in DDL/DML executors: `SetUserContext()` now only sets the context, but does not itself filter or separate the data.
