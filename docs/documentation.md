# Uniter Documentation

This document is the consolidated project reference. It summarizes the former
architecture documents, former database notes, and the manual conflict
decisions that were merged before the old source Markdown files were removed.
`docs/plan.md` remains the separate implementation plan for the data layer.
Visual references are stored in `docs/visual/`.

## 1. Project Overview

Uniter is a Qt C++17 client/server PDM/ERP application for engineering and
manufacturing companies. Its purpose is to automate accounting of materials,
design documentation, procurement and production tasks by turning design data
into structured resources that can be synchronized between clients.

The project is built with CMake, self-made SDK "DevKit" and CLion. The repository contains three
main build products:

| Product | Role |
|---|---|
| `Common` | Static helper library |
| `Updater` | Separate executable for application updates |
| `Uniter` | Main client application |

The project uses GoogleTest for tests. Third-party libraries include MuPDF for
PDF extraction and tinyxml2 for XML processing. The build environment is
MinGW/MSYS with Qt.

## 2. Submodules

The repository uses git submodules for shared (between client app (current project) and server services) contract and database code:

| Submodule | Path | Repository |
|---|---|---|
| `uniter-contract` | `src/uniter/contract/` | `https://github.com/Shokerzoro/uniter-contract` |
| `uniter-database` | `src/uniter/database/` | `https://github.com/Shokerzoro/uniter-database` |

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/Shokerzoro/uniter.git
```

Initialize submodules after a regular clone:

```bash
git submodule update --init --recursive
```

Update submodules:

```bash
git submodule update --remote --merge
```

The submodules are included from `src/uniter/CMakeLists.txt` through
`sources.cmake` files:

```cmake
include(contract/sources.cmake)
include(database/sources.cmake)
```

## 3. Architectural Layers

Uniter is split into independent layers. Each layer communicates through clear
boundaries and avoids direct coupling to unrelated parts of the application.

| Layer | Main components | Responsibility |
|---|---|---|
| Network | `ServerConnector`, `KafkaConnector`, `MinioConnector` | External communication |
| Application management | `AppManager`, `ConfigManager` | FSM, routing, lifecycle, configuration |
| Data management | `DataManager`, `FileManager`, database executors | Local structured data, files, observers |
| Business logic | `PDMManager`, `ERPManager` | Domain workflows over data/files |
| UI | `MainWindow`, static/generative/dynamic widgets | User workflows and presentation |

All network traffic is routed through `AppManager`. Network classes do not talk
directly to UI, DataManager or business logic.

## 4. Network Layer

### ServerConnector

`ServerConnector` is the TCP/SSL channel to the Uniter server. It is responsible
for authentication, protocol requests and client-originated CUD requests.

Responsibilities:

- maintain TCP/SSL connection;
- serialize and deserialize `UniterMessage` XML;
- send protocol and CRUD requests to the server;
- buffer outgoing CUD requests when disconnected;
- handle request timeouts.

Protocol operations include `AUTH`, `FULL_SYNC`, `GET_KAFKA_CREDENTIALS`,
`GET_MINIO_PRESIGNED_URL` and update-related actions.

### KafkaConnector

`KafkaConnector` is a one-way broadcast channel. The client is only a consumer.
It receives CRUD notifications and success confirmations from Kafka.

Responsibilities:

- connect using credentials received from the server;
- subscribe to the company topic;
- deserialize Kafka messages into `UniterMessage`;
- store the last processed offset in OS Secure Storage per user;
- request full synchronization if the saved offset is stale or missing.

Kafka messages use:

| Status | Meaning |
|---|---|
| `NOTIFICATION` | Change made by another user |
| `SUCCESS` | Confirmation of this user's own CUD operation |

### MinioConnector

`MinioConnector` is the HTTP client for MinIO object storage. It works only with
temporary presigned URLs issued by the server.

Responsibilities:

- download files through HTTP GET;
- upload files through HTTP PUT;
- delete files if the protocol later requires it;
- avoid direct storage of permanent MinIO credentials.

## 5. Application Management Layer

### AppManager

`AppManager` owns the global application FSM and all message routing. It routes
incoming messages from network classes upward and outgoing messages from UI or
subsystems downward.

Important states:

| State | Role                                       |
|---|--------------------------------------------|
| `IDLE` | Initial state                              |
| `STARTED` | Connection requested                       |
| `AUTHENIFICATION` | Auto search auth data and request          |
| `IDLE_AUTHENIFICATION` | Waiting for user auth data input           |
| `DBLOADING` | DataManager initialization                 |
| `CONFIGURATING` | ConfigManager processes user permissions   |
| `KCONNECTOR` | Kafka connector initialization             |
| `KAFKA` | Server checks Kafka credentials/offset     |
| `DBCLEAR` | Local database is cleared before full sync |
| `SYNC` | Full synchronization request               |
| `READY` | Normal application work                    |
| `SHUTDOWN` | Final cleanup                              |

Golden path:

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

If the Kafka offset is stale:

```text
KAFKA -> DBCLEAR -> SYNC -> READY
```

If `DataManager` reports `signalResourcesLoaded(false)` from `DBLOADING`,
`AppManager` remembers that the local database is not usable, still runs
`CONFIGURATING`, then skips `KCONNECTOR` and enters `DBCLEAR` directly:

```text
DBLOADING(false) -> CONFIGURATING -> DBCLEAR -> SYNC -> READY
```

`DBCLEAR` is a full local reset point. Its entry action must ask
`KafkaConnector` to forget the saved offset for the current user, then ask
`DataManager` to recreate local database state before `FULL_SYNC`.

`READY` subscription to Kafka is started with an explicit offset:
`signalSubscribeKafka(offset)`. The offset comes from the server protocol
response (`add_data["offset"]`) so the consumer knows where to start.

Online/offline state is orthogonal to `AppState`. Network states repeat their
entry action after reconnect (if they exist); local states do not.

### Routing Rules

Incoming routing through `onRecvUniterMessage`:

| Condition                         | Destination                   |
|-----------------------------------|-------------------------------|
| Offline                           | Drop/log                      |
| `PROTOCOL` before `READY`         | FSM internal handlers         |
| CUD after `READY`                 | To DataManager                |
| `PROTOCOL` after `READY`          | MinIO-related to FileMaanger  |
| `PROTOCOL` after `READY`          | Update-related to FileMaanger |
| `PROTOCOL` after `READY`  | User-related to ConfigManager |

Outgoing routing through `onSendUniterMessage`:

| Source/state | Destination |
|---|---|
| Auth request before `READY` | `ServerConnector` |
| CRUD in `READY` | `ServerConnector` |
| `GET_MINIO_PRESIGNED_URL` | `ServerConnector` |
| `GET_MINIO_FILE` / `PUT_MINIO_FILE` | `MinioConnector` |

### ConfigManager

`ConfigManager` parses the authenticated `Employee` resource and generates UI
configuration: available subsystem tabs, permissions and generative subsystem
visibility.

## 6. Data Management Layer

### DataManager

`DataManager` is the client-side owner of local structured data (thick client). It connects
AppManager, database access, subsystem executors and UI observers.

Responsibilities:

- initialize database access for the current user;
- keep local data isolated by user context;
- route CRUD messages to subsystem executors (databaase submodule);
- clear local resource data before `FULL_SYNC` on AppManager demand;
- apply CUD events based on UniterMessager recieved from AppManager;
- provide direct READ access for UI (througn observers mechanism);
- notify subscribers after changes.

The target internal structure is:

```cpp
std::unique_ptr<database::IDataBase> db_;
std::vector<std::unique_ptr<database::IResExecutor>> executors_;
```

`executors_` is ordered by schema dependency. `DataManager` must use it for
template lifecycle actions:

- initialize and verify in forward order;
- clear/drop in reverse order where foreign keys require child tables first;
- keep concrete CRUD entry points on concrete executors until
  `IResExecutor::HandleMessage` is implemented for all subsystems.

Subscriptions should be indexed by shared contract keys: `SubsystemKey` for
subsystem/generative/resource-type lists and `ResourceKey` for an exact
resource id inside that context. The mandatory API should include:

- subscription to a resource list;
- subscription to a specific resource.

### IDataBase

`IDataBase` is the low-level database engine abstraction (in database submodule). It knows how to open
connections, execute SQL and manage transactions, but it does not know business
resources or subsystem schemas.

Responsibilities:

- open and close database connections;
- select physical storage for a user;
- store current user context;
- execute SQL and return `SqlResult`;
- manage transactions;
- expose minimal engine-specific helpers.

SQLite, PostgreSQL and test in-memory implementations should fit this boundary.

### Executors

Executors own database logic for one subsystem. They receive `IDataBase&` as descriptor from
DataManager and own DDL, DML, and resource mapping for their subsystem.

Minimum lifecycle:

- `Subsystem()`;
- `Initialize(IDataBase&)`;
- `Verify(IDataBase&)`;
- `ApplyMigrations(IDataBase&)`;
- `ClearData(IDataBase&)`;
- `DropStructures(IDataBase&)`;
- `Create/Read/Update/Delete`;
- `HandleMessage(IDataBase&, const UniterMessage&)`.

`ManagerExecutor` is the first target worker executor.

Verification contract:

- `Verify(IDataBase&)` checks schema only, not business data availability.
- Empty valid tables must pass verification.
- A verification failure means required tables are missing or their required
  columns/structure do not match the executor contract.
- `raw_sql_{subsystem}/verify.sql` must contain schema-inspection queries
  (`pragma_table_info`, `sqlite_master`, or equivalent engine metadata), not
  `SELECT ... FROM business_table LIMIT 1` row-presence probes.
- Executor `Verify` methods interpret those checks and return
  `SchemaInvalid` when the schema cannot safely accept sync/CRUD data.

### SQL Authoring and CodeGen

Database SQL is authored as raw `.sql` files under each subsystem directory:

```text
src/uniter/database/{subsystem}/raw_sql_{subsystem}/
```

These files are maintained in the DataGrip side project and are the readable
source for table DDL, CRUD instructions, verification queries, cleanup scripts
and drop scripts. CodeGen translates selected instructions from
`raw_sql_{subsystem}` into generated C++ headers under:

```text
src/uniter/database/{subsystem}/gen_sql_{subsystem}/
```

The generated headers expose SQL as `static constexpr const char*` literals.
CodeGen is controlled by comments in the raw SQL files, so a raw SQL file may
contain helper queries or notes that are not emitted into C++.

Each `raw_sql_{subsystem}` directory must contain these files with the following
content:

| File | Required content |
|---|---|
| `tables.sql` | DDL for subsystem-owned tables, indexes, triggers and views. Use `CREATE ... IF NOT EXISTS` where initialization is idempotent. Do not include enum/domain tables generated from C++ arrays. |
| `create.sql` | Insert/upsert statements needed to create every synced resource. Include explicit-id variants for server/full-sync input and auto-id variants only where local drafts require them. |
| `read.sql` | SELECT statements that reconstruct runtime resources from normalized tables. Include exact-resource reads and list/context reads used by observers/UI. |
| `update.sql` | Update statements for mutable resource fields and replacement logic for owned child rows when a collapsed runtime resource changes. |
| `delete.sql` | Delete or soft-delete statements for resources. Preserve the project decision for each table: hard delete for local reset helpers, soft delete for synced resources when required. |
| `clear.sql` | Data cleanup for full-sync reload. Delete only resource/user data, in FK-safe child-to-parent order. Preserve domains, migrations and service schema unless the subsystem explicitly owns disposable service data. |
| `drop.sql` | Structure drop for full local reset fallback and tests. Drop views/triggers/indexes/tables in dependency-safe order. `DROP ... IF EXISTS` is required. |
| `verify.sql` | Schema metadata checks only: table existence, required columns, indexes/triggers/views where they are part of the contract. Empty valid tables must pass. Do not check business row presence. |
| `migrations.sql` | Optional. Only for subsystem-local migration helpers that are not distributed as normal migration resources yet. |

Raw SQL may include DataGrip comments and helper queries, but every query marked
for CodeGen must be executable by the target `IDataBase` implementation without
manual editing.

Enum-backed table domains are not part of raw or generated SQL directories.
They are generated directly from `std::array<std::pair<int, std::string>, N>`
definitions in the main contract/project code. Each subsystem owns these domain
builders in a root-level header:

```text
src/uniter/database/{subsystem}/{subsystem}domains.h
```

Examples are `common/commondomains.h` for protocol/resource domains and
`manager/managerdomains.h` for manager permissions. The `raw_sql_*` directories
remain SQL authoring inputs, and `gen_sql_*` remains CodeGen output only.

### FileManager

`FileManager` coordinates file operations over MinIO.

Download flow:

1. request presigned URL from the server;
2. call `MinioConnector::get`;
3. verify SHA-256 using metadata from DataManager;
4. notify `IFileObserver` subscribers.

Upload flow:

1. request upload URL/token from the server;
2. call `MinioConnector::put`;
3. notify DataManager about URL/object metadata updates;
4. notify `IFileObserver`.

## 7. UniterMessage

`UniterMessage` is the universal exchange container for protocol, CRUD and file
operations. Resource-specific data is carried as XML/resource payload; protocol
parameters are carried in `add_data`.

### Message Type

| Type | Meaning                                                                                            |
|---|----------------------------------------------------------------------------------------------------|
| `CRUD` | Resource create/read/update/delete                                                                 |
| `PROTOCOL` | Control communication with server: authentification, MINIO credentials, Kafka credentials, updates |

### Message Status

| Status | Meaning |
|---|---|
| `REQUEST` | Client request |
| `RESPONSE` | Server response |
| `ERROR` | Processing error |
| `NOTIFICATION` | Broadcast data change |
| `SUCCESS` | Successful CUD confirmation |

### Protocol `add_data`

`add_data` is the strict key/value contract for protocol parameters. Values are
always `QString`.

Important keys:

| Action | Keys |
|---|---|
| `AUTH` | `login`, `password_hash` |
| `GET_KAFKA_CREDENTIALS` | `bootstrap_servers`, `topic`, `username`, `password`, `group_id`, `offset`, `offset_actual` |
| `GET_MINIO_PRESIGNED_URL` | `object_key`, `minio_operation`, `presigned_url`, `url_expires_at` |
| `GET_MINIO_FILE` | `presigned_url`, `object_key`, `expected_sha256`, `local_path`, `reason` |
| `PUT_MINIO_FILE` | `presigned_url`, `object_key`, `local_path`, `sha256`, `reason` |
| `FULL_SYNC` | response may include `offset`, the actual Kafka offset for post-sync subscription |
| `UPDATE_CHECK` | `current_version`, `update_available`, `new_version`, `release_notes` |
| `UPDATE_DOWNLOAD` | `version`, `file_path`, `presigned_url`, `file_size`, `sha256`, `file_index`, `files_total` |

String literals should be moved to a future `contract/add_data_keys.h`.

## 8. Database Design Principles

The database documentation follows one key rule:

```text
Runtime class != database table
```

Runtime classes are convenient collapsed objects. Database tables are normalized.
DataManager and subsystem executors collapse multiple tables into one resource
when reading and expand resources back to tables when writing.

Examples:

- `TemplateSimple` contains segment vectors at runtime clas, but segments and allowed
  values live in separate database tables.
- `Employee` contains assignments and permissions at runtime, but the database
  stores employee, assignment, permission and assignment-link tables separately.
- `Assembly` contains configurations at runtime, but assembly configuration and
  join data are stored in separate tables.

Every linked table that must be independently synchronized should have its own
`ResourceType` or at least a clear logical CRUD unit.

## 9. Database User Isolation

SQLite does not provide PostgreSQL-like users, schemas or `search_path`.
The selected client model is:

- one physical local database file;
- `IDataBase::SetUserContext(userHash)` stores the active logical user context;
- executors create user-scoped tables, views or filters;
- DML always respects the current user context.

The practical SQLite option is a technical `user_hash` or `local_user_id`
column in resource tables, optionally hidden behind views/triggers.

## 10. Subsystems and Resources

### MIGRATIONS

Independen subsystem and resource distributed through common massage flow and
applied upon receipt in DataManager, perform Databases migrations
(may be received form server after update), up to design and implement

### DOCUMENTS

Documents are represented by `DocLink` folders and `Doc` file records.

| Runtime class | ResourceType | Table |
|---|---|---|
| `DocLink` | `DOC_LINK = 91` | `documents_doc_link` |
| `Doc` | `DOC = 90` | `documents_doc` |

`DocLink` stores target type and collapsed `docs`. `Doc` stores MinIO metadata:
`object_key`, `sha256`, document type, name, size, MIME type, description and
optional local cache path. A `Doc` belongs to one `DocLink` through database FK.

### MATERIALS and INSTANCES

Materials define templates; instances specialize templates and add quantities.

| Runtime class | ResourceType | Main table |
|---|---|---|
| `TemplateSimple` | `MATERIAL_TEMPLATE_SIMPLE = 20` | `material_template_simple` |
| `TemplateComposite` | `MATERIAL_TEMPLATE_COMPOSITE = 21` | `material_template_composite` |
| `SegmentDefinition` | `SEGMENT` | `material_segment` |
| Segment value | `SEGMENT_VALUE` | `material_segment_value` |
| Compatibility pair | `TEMPLATE_COMPATIBILITY` | `material_template_compatibility` |
| `InstanceSimple` | `MATERIAL_INSTANCE_SIMPLE = 61` | `material_instances_simple` |
| `InstanceComposite` | `MATERIAL_INSTANCE_COMPOSITE = 62` | `material_instances_composite` |

Simple templates own prefix/suffix segment definitions and allowed values.
Composite templates reference two simple templates. Instances reference the
template they specialize and carry quantity data. Simple instance may be:
standalone, assortment or material and contain information about which
can be combined to form composite templates

### MANAGER

Manager owns employees, permission assignments, plants and integrations.

| Runtime class/data | ResourceType | Table |
|---|---|---|
| `Employee` | `EMPLOYEES = 10` | `manager_employee` |
| `EmployeeAssignment` | `EMPLOYEE_ASSIGNMENT` | `manager_employee_assignment` |
| Permission row | `PERMISSION` | `manager_permissions` |
| Employee-assignment link | `EMPLOYEE_ASSIGNMENT_LINK` | `manager_employee_assignment_link` |
| `Plant` | `PRODUCTION = 11` | `manager_plant` |
| `Integration` | `INTEGRATION = 12` | `manager_integration` |

`Employee` collapses assignments and permissions at runtime. Permissions use
the shared `contract::Subsystem` enum, not a duplicate manager-local enum.

### DESIGN

DESIGN is the source of truth for the current editable product structure.

| Runtime class | ResourceType | Table(s) |
|---|---|---|
| `Project` | `PROJECT = 30` | `design_project` |
| `Assembly` | `ASSEMBLY = 31` | `design_assembly` |
| `AssemblyConfig` | `ASSEMBLY_CONFIG = 33` | `design_assembly_config` + joins |
| `Part` | `PART = 32` | `design_part` |
| `PartConfig` | `PART_CONFIG = 34` | `design_part_config` |

Important model decisions:

- `design_project` has `root_assembly_id` and nullable `pdm_project_id`.
- `active_snapshot_id` was removed from DESIGN.
- `design_assembly` stores general assembly data only.
- assembly contents live in `design_assembly_config` and join tables.
- `design_part` has no `assembly_id`; parts can appear in many assemblies.
- `design_part` must have `project_id`, the same way `design_assembly` belongs
  to a project.
- `design_assembly_config_standard_products` references
  `material_instances_simple`.
- `design_assembly_config_materials` references `material_instances_simple`.
- files are linked through `documents_doc_link`.

### PDM

PDM stores versioned immutable snapshots of DESIGN data and deltas between them.

| Runtime class | ResourceType | Table |
|---|---|---|
| `PdmProject` | `PDM_PROJECT = 52` | `pdm_project` |
| `Snapshot` | `SNAPSHOT = 50` | `pdm_snapshot` |
| `Delta` | `DELTA = 51` | `pdm_delta` |
| `Diagnostic` | `DIAGNOSTIC = 57` | `pdm_diagnostic` |

PDM also owns mirror copies of DESIGN resources:

| Mirror table | ResourceType |
|---|---|
| `pdm_assembly` | `ASSEMBLY_PDM = 53` |
| `pdm_assembly_config` | `ASSEMBLY_CONFIG_PDM = 54` |
| `pdm_part` | `PART_PDM = 55` |
| `pdm_part_config` | `PART_CONFIG_PDM = 56` |

When a snapshot is created, the current `design_*` records are copied into
`pdm_*` mirror tables with `snapshot_id`. The snapshot points to the frozen
`pdm_assembly` root, not to the live `design_assembly`.

The snapshot/delta chain is a doubly linked list:

```text
snapshot_1 <-> delta_1 <-> snapshot_2 <-> delta_2 <-> snapshot_3
```

`pdm_project.base_snapshot_id` points to the first snapshot, and
`pdm_project.head_snapshot_id` points to the active snapshot. There is no
separate `approved_snapshot_id` in the accepted model.

Accepted implementation rule for SQL/C++ conflicts:

- SQL schema wins over current C++ class shape.
- Resource classes should be finally approved after `.sql` files are developed.
- The `snapshot_id` transfer convention for PDM mirror CRUD is finalized during
  SQL/executor implementation, not from old Markdown notes.

### PURCHASES

Procurement contains complex purchase groups and simple purchase requests.

| Runtime class | ResourceType | Table |
|---|---|---|
| `PurchaseComplex` | `PURCHASE_GROUP = 40` | `supply_purchase_complex` |
| `Purchase` | `PURCHASE = 41` | `supply_purchase_simple` |

`Purchase` optionally belongs to one `PurchaseComplex` through
`purchase_complex_id`. It references either a simple or composite material
instance, but not both.

### PRODUCTION

Production is a generative subsystem created per `Plant`.

| Runtime class | ResourceType | Table |
|---|---|---|
| `ProductionTask` | `PRODUCTION_TASK = 70` | `production_task` |
| `ProductionStock` | `PRODUCTION_STOCK = 71` | `production_stock` |
| `ProductionSupply` | `PRODUCTION_SUPPLY = 72` | `production_supply` |

`ProductionTask` stores both:

- `snapshot_original_id` - snapshot used when the task was created;
- `snapshot_current_id` - snapshot currently used for production.

This preserves history while allowing a task to move to a newer approved
composition.

### INTEGRATION

Integration is a generative subsystem created per `manager::Integration`.

| Runtime class | ResourceType | Table |
|---|---|---|
| `IntegrationTask` | `INTEGRATION_TASK = 80` | `integration_task` |

`IntegrationTask` references an arbitrary resource through a polymorphic key:
`target_subsystem`, `target_resource_type`, `any_resource_id`. DataManager must
validate this reference because the SQL database cannot express a normal FK to a
runtime-selected table.

In future integrations will have server maintainance at Uniter server. Maintainace
will be based on "id gate" between companies, and creation an IntegrationTask resource
will cause adding linked resource id to be traked in this gateway. If resource is tracked
in gatway it has other company id pair (in other company id space) and all CUD operations
which touches such resource (stored in gateway) will be translated in other company id
space and shared with other company UniterMessage.

## 11. DESIGN and PDM Interaction

DESIGN and PDM are intentionally separated:

- DESIGN stores the current editable state.
- PDM stores immutable frozen copies and deltas.

First snapshot:

1. create `pdm_project`;
2. create `pdm_snapshot` version 1;
3. copy current DESIGN rows into PDM mirror tables;
4. set `pdm_snapshot.root_assembly_id` to the copied root assembly;
5. set `pdm_project.base_snapshot_id` and `head_snapshot_id`;
6. set `design_project.pdm_project_id`.

Subsequent snapshot:

1. create a new `pdm_snapshot` or delta (based on user permissions);
2. copy current DESIGN rows into mirror tables;
3. set the copied root assembly;
4. create `pdm_delta` between previous head and new snapshot;
5. update cross-links on previous snapshot, new snapshot and delta;
6. move `pdm_project.head_snapshot_id`.

## 12. PDMManager and Drawing Compiler

`PDMManager` owns design-documentation workflows:

- scan project PDF directory;
- invoke the drawing compiler;
- build XML snapshot data;
- upload documents/XML to MinIO;
- create/update DESIGN records;
- create PDM snapshots and deltas;
- validate ESKD-related diagnostics;
- support version iteration and rollback.

The drawing compiler works with translation primitives:

1. collect PDF files;
2. split each PDF into page-level primitives;
3. detect page type by designation/title block;
4. group pages into logical translation units;
5. route units to type-specific compilers;
6. link compiled data into the project tree.

`PDMManager` creates the `XMLDocument` and root element. The compiler library
receives only the root element to fill. The XML document structure looks like

The Snapshot XML structure separates **links** (product structure) and **definitions** (item parameters). Inside each assembly there are two elements **structure \<structure>** and **parts \<parts>** used directly in this assembly. Inside the structure there is a **constant data \<invariant>** element, as well as variable data for each **configuration \<config>**, which together provide complete data about the structure for each configuration. In this case, only references to parts/assemblies are used in the structure, for example \<partref> \<assemblyref>. Complete parts data is contained within \<parts>.  This eliminates data duplication and allows parts to be reused in different assemblies.


	<shapshot id="" designation="" name="" version="2" previousVersion="1">
	
	  <Assembly designation="" name="">
	    <Metadata>...</Metadata>
	    <Documentation>
	      <File path="..." hash="sha256:..." modifiedAt="..."/>
	    </Documentation>
	    <Structure>
	      <Invariant>
	        <Assemblies><AssemblyRef designation="" config=""/></Assemblies>
	        <Parts><PartRef designation="" config=""/></Parts>
	        <StandardProducts/>
	        <BuyingProducts/>
	        <OtherMaterials/>
	      </Invariant>
	      <Config number="01">...</Config>
	      <Config number="02">...</Config>
	    </Structure>
	    <PartsDef>
	      <Part designation="" name="">
	        <Config id="01"/>
	        <Config id="02"/>
	      </Part>
	    </PartsDef>
	  </Assembly>
	
	  <Errors>
	    <Error severity="Error" category="FileSystem" type="NO_FILE"
	           path="drawings/4021.01.00.02.pdf"/>
	    <Error severity="Warning" category="VersionControl" type="INFORMAL_CHANGE"
	           designation="4021.01.00.03" hashBefore="sha256:a1b2..." hashAfter="sha256:c3d4..."/>
	  </Errors>
	
	</Shapshot>


    
    <Partdef designation="4021.01.00.01" name="Longitudinal beam">
          <Metadata>
            <Material>
    <Base>Channel 20P GOST 8239-89</Base>
              <CoatingTop/>
              <CoatingBottom/>
            </Material>
            <Signatures>
    <Signature role="Developed" name="Ivanov I.P." date="2026-01-15"/>
    <Signature role="Checked" name="Petrov S.A."  date="2026-01-18"/>
            </Signatures>
    <Litera>O</Litera>
    <Organization>OJSC "Plant"</Organization>
            <DrawingFile>
    <Path>drawings/4021.01.00.01_rev.A.pdf</Path>
              <Hash>sha256:e7f8g9...</Hash>
              <ModifiedAt>2026-01-15T11:30:00+03:00</ModifiedAt>
            </DrawingFile>
          </Metadata>
          <Config id="01">
            <Dimensions length="2400" width="200" height="80"/>
            <Mass>85.2</Mass>
          </Config>
          <Config id="02">
            <Dimensions length="2800" width="200" height="80"/>
            <Mass>99.4</Mass>
          </Config>
        </Partdef>

## 13. ERPManager

`ERPManager` performs local analytics over DataManager data:

- calculate material requirements from production plans;
- compare required material instances with stock and purchases;
- forecast shortages;
- generate material balance reports.

ERP should reference PDM snapshots rather than live editable DESIGN state when
reproducibility matters.

## 14. UI Layer

Static widgets live for the whole application lifecycle:

- `MainWindow`;
- `AuthWidget`;
- `WorkspaceWidget`.

Subsystem widgets are generated according to user permissions and subscribe to
DataManager resource lists. Dynamic widgets subscribe to specific resources and
form CRUD `UniterMessage` instances for user edits.

Widgets should not bypass AppManager for network traffic.

## 15. Migrations

Database migrations are not local ad-hoc `migrations.sql` files. Target design:

- migrations are resources of a future service/system subsystem;
- the server distributes migrations via `UniterMessage`;
- each migration has version, target subsystem, SQL steps or artifact reference,
  checksum and application metadata;
- DataManager orchestrates migration order;
- subsystem executors apply their own migration logic;
- regular CRUD is processed only after required schema version is applied.

Migrations must be idempotent, transactional and verifiable.

## 16. Current Verification Notes

The former conflict file was reviewed manually. Accepted decisions:

1. For C++/SQL conflicts, SQL wins. Resource classes are approved after `.sql`
   files are developed.
2. Old Markdown files are no longer authoritative. If Markdown conflicts with
   C++, C++ wins unless the SQL schema later overrides it.
3. PDM mirror ResourceTypes follow the current C++ protocol values:
   `ASSEMBLY_PDM = 53`, `ASSEMBLY_CONFIG_PDM = 54`, `PART_PDM = 55`,
   `PART_CONFIG_PDM = 56`.
4. `design_assembly_config_materials` references
   `material_instances_simple`.
5. `design_assembly_config_standard_products` also references
   `material_instances_simple`, matching `designtypes.h`.
6. `pdm_project.head_snapshot_id` is the active snapshot. Forget the
   `approved_snapshot_id` idea.
7. `design_part` must have `project_id`.
8. Splitting the old generic `MATERIAL_INSTANCE = 60` into
   `MATERIAL_INSTANCE_SIMPLE = 61` and `MATERIAL_INSTANCE_COMPOSITE = 62` is the
   correct model.

Remaining cleanup notes:

1. Remove legacy comments that mention `Project.active_snapshot_id`.
2. Remove legacy comments that mention generic `MATERIAL_INSTANCE = 60`.
3. If visual schemas are regenerated, ensure DESIGN shows `design_part.project_id`.

## 17. Visual References

Non-Markdown reference artifacts are kept in `docs/visual/`:

- PDF diagrams and exports;
- DWG drawings;
- backup/lock artifacts from visual tooling.
