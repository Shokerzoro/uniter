# PDM/DESIGN architecture: interaction of subsystems, file management, versioning

> ⚠️ **OBSOLETE (legacy).** This document describes the previous iteration
> DESIGN/PDM concepts (Assembly with embedded child_assemblies/parts,
> `Project.active_snapshot_id`, single `MATERIAL_INSTANCE`). Current
> concept - in `docs/db/design.md`, `docs/db/pdm.md`,
> `docs/db/pdm_design_logic.md`. The document is left for history and for
> FileManager/PDMManager related workflows that are currently
> maintain validity.

> Reference for implementing the `contract/design/`, `contract/pdm/` resource classes, SQLite schema and `PDMManager` logic.

---

## 1. Data model: resource fields

### 1.1. ResourceAbstract (base fields of all resources)

Inherited by all resources below - they are not repeated in tables.

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `id` | `uint64_t` | `id` | `INTEGER PK` | Server Global ID |
| `is_actual` | `bool` | `actual` | `INTEGER` (0/1) | Soft-delete flag |
| `created_at` | `QDateTime` | `created_at` | `TEXT` (ISO 8601) | |
| `updated_at` | `QDateTime` | `updated_at` | `TEXT` (ISO 8601) | |
| `created_by` | `uint64_t` | `created_by` | `INTEGER` | FK → employees.id |
| `updated_by` | `uint64_t` | `updated_by` | `INTEGER` | FK → employees.id |

---

### 1.2. Project

**Role:** The source of truth for the current state of the constructor. `active_snapshot_id` is the “official” version that is allowed for use by other subsystems.

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `name` | `QString` | `name` | `TEXT NOT NULL` | Project name |
| `description` | `QString` | `description` | `TEXT` | Description |
| `projectcode` | `QString` | `projectcode` | `TEXT UNIQUE NOT NULL` | Project code (ESKD-top-level designation) |
| `rootdirectory` | `QString` | `rootdirectory` | `TEXT` | CD root folder (local path or URI) |
| `root_assembly_id` | `uint64_t` | `root_assembly_id` | `INTEGER` | FK → assemblies.id (root assembly) |
| `active_snapshot_id` | `std::optional<uint64_t>` | `active_snapshot_id` | `INTEGER NULL` | FK → snapshots.id; NULL no Snapshots have been approved yet |

> **What to add to the current class:** remove `std::shared_ptr<Assembly> root_assembly` (eager load), replace with `uint64_t root_assembly_id`. Add `active_snapshot_id`.

---

### 1.3. Assembly

**Role:** assembly tree node. Stores the current object_key files - the “now” state.

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `project_id` | `uint64_t` | `project_id` | `INTEGER NOT NULL` | FK → projects.id |
| `parent_assembly_id` | `std::optional<uint64_t>` | `parent_assembly_id` | `INTEGER NULL` | FK → assemblies.id; NULL for root |
| `designation` | `QString` | `designation` | `TEXT NOT NULL` | Designation according to ESKD (eg `SB-001`) |
| `name` | `QString` | `name` | `TEXT NOT NULL` | Name |
| `description` | `QString` | `description` | `TEXT` | |
| `type` | `AssemblyType` | `type` | `INTEGER` | 0=REAL, 1=VIRTUAL |
| `drawing_object_key` | `QString` | `drawing_object_key` | `TEXT` | MinIO key assembly drawing (current) |
| `drawing_sha256` | `QString` | `drawing_sha256` | `TEXT` | SHA-256 assembly drawing |
| `spec_object_key` | `QString` | `spec_object_key` | `TEXT` | MinIO key specifications |
| `spec_sha256` | `QString` | `spec_sha256` | `TEXT` | SHA-256 specifications |
| `mounting_drawing_object_key` | `QString` | `mounting_drawing_object_key` | `TEXT` | MinIO key of installation drawing (if available) |
| `mounting_drawing_sha256` | `QString` | `mounting_drawing_sha256` | `TEXT` | |
| `model_3d_object_key` | `QString` | `model_3d_object_key` | `TEXT` | MinIO key 3D models (if available) |
| `model_3d_sha256` | `QString` | `model_3d_sha256` | `TEXT` | |

**Separate table `assembly_children`** (relationship N:M Assembly → child Assembly with quantity):

| Column | Type | Description |
|---|---|---|
| `parent_id` | `INTEGER` | FK → assemblies.id |
| `child_id` | `INTEGER` | FK → assemblies.id |
| `quantity` | `INTEGER` | Number of occurrences |
| `config` | `TEXT` | Execution (if applicable) |

> **What to add to the current class:** add `designation`, file fields with `object_key`/`sha256` instead of `shared_ptr<FileVersion>`. `FileVersion` as a separate entity has been abolished - file version history is stored in `Delta` (see below). Remove `part_id_ctr` (server responsibility), remove `assembly_id` (duplicates `id`).

---

### 1.4. Part (PartDef)

**Role:** leaf node of a tree. Stores the current object_key of the drawing and 3D model.

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `project_id` | `uint64_t` | `project_id` | `INTEGER NOT NULL` | FK → projects.id |
| `assembly_id` | `uint64_t` | `assembly_id` | `INTEGER NOT NULL` | FK → assemblies.id (parent assembly) |
| `designation` | `QString` | `designation` | `TEXT NOT NULL` | ESKD designation (eg `SB-001-01`) |
| `name` | `QString` | `name` | `TEXT NOT NULL` | Name |
| `litera` | `QString` | `litera` | `TEXT` | Letter KD (O, O1, A, etc.) |
| `organization` | `QString` | `organization` | `TEXT` | Development organization |
| `material_instance_id` | `std::optional<uint64_t>` | `material_instance_id` | `INTEGER NULL` | FK → material_instances.id |
| `drawing_object_key` | `QString` | `drawing_object_key` | `TEXT` | MinIO key for part drawing (current) |
| `drawing_sha256` | `QString` | `drawing_sha256` | `TEXT` | SHA-256 drawing |
| `drawing_modified_at` | `QDateTime` | `drawing_modified_at` | `TEXT` | Drawing file modification date |
| `model_3d_object_key` | `QString` | `model_3d_object_key` | `TEXT` | MinIO key 3D models (optional) |
| `model_3d_sha256` | `QString` | `model_3d_sha256` | `TEXT` | |

**Separate table `part_configs`** (configurations of versions of one part):

| Column | Type | Description |
|---|---|---|
| `part_id` | `INTEGER` | FK → parts.id |
| `config_id` | `TEXT` | Execution ID (`01`, `02`, ...) |
| `length_mm` | `REAL` | |
| `width_mm` | `REAL` | |
| `height_mm` | `REAL` | |
| `mass_kg` | `REAL` | |

**Separate table `part_signatures`**:

| Column | Type | Description |
|---|---|---|
| `part_id` | `INTEGER` | FK → parts.id |
| `role` | `TEXT` | Signatory Role (Developed, Reviewed, ...) |
| `name` | `TEXT` | Full name |
| `date` | `TEXT` | Signature date (ISO 8601) |

> **What to add to the current class:** add `designation`, `litera`, `organization`, `material_instance_id`, replace `vector<shared_ptr<FileVersion>>` with flat fields `drawing_object_key`/`drawing_sha256`/`drawing_modified_at` and `model_3d_object_key`/`model_3d_sha256`. Remove `source_project_id` (duplicates `project_id`).

---

### 1.5. Snapshot

**Role:** fixed section of the project at the time of parsing/approval. Similar to a git branch in the sense that one Project can have multiple Snapshots (different versions), and each Snapshot independently moves through the DRAFT → APPROVED → ARCHIVED life cycle.

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `project_id` | `uint64_t` | `project_id` | `INTEGER NOT NULL` | FK → projects.id |
| `version` | `uint32_t` | `version` | `INTEGER NOT NULL` | Monotonically increasing version number |
| `previous_snapshot_id` | `std::optional<uint64_t>` | `previous_snapshot_id` | `INTEGER NULL` | FK → snapshots.id; NULL for first |
| `xml_object_key` | `QString` | `xml_object_key` | `TEXT NOT NULL` | MinIO key of the snapshot XML file |
| `xml_sha256` | `QString` | `xml_sha256` | `TEXT NOT NULL` | SHA-256 XML file |
| `status` | `SnapshotStatus` | `status` | `INTEGER` | 0=DRAFT, 1=APPROVED, 2=ARCHIVED |
| `approved_by` | `std::optional<uint64_t>` | `approved_by` | `INTEGER NULL` | FK → employees.id |
| `approved_at` | `std::optional<QDateTime>` | `approved_at` | `TEXT NULL` | |
| `error_count` | `uint32_t` | `error_count` | `INTEGER` | Number of ESKD validation errors |
| `warning_count` | `uint32_t` | `warning_count` | `INTEGER` | Number of warnings |

Enum `SnapshotStatus`: `DRAFT = 0`, `APPROVED = 1`, `ARCHIVED = 2`.

> Snapshot does not store an inline list of parts/assemblies - only `xml_object_key`. The structure is read from MinIO if necessary (view version, compare).

---

### 1.6. Delta

**Role:** Incremental changes between the base Snapshot and the next one. Delta = "what changed" when moving from `snapshot_id` to the next version.

**Table `deltas`:**

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `snapshot_id` | `uint64_t` | `snapshot_id` | `INTEGER NOT NULL` | FK → snapshots.id (base version from which delta is calculated) |
| `next_snapshot_id` | `uint64_t` | `next_snapshot_id` | `INTEGER NOT NULL` | FK → snapshots.id (version before which the delta is applied) |
| `changes_count` | `uint32_t` | `changes_count` | `INTEGER` | Number of changes in delta |

**Table `delta_changes`** (one row = one change in delta):

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `delta_id` | `uint64_t` | `delta_id` | `INTEGER NOT NULL` | FK → deltas.id |
| `designation` | `QString` | `designation` | `TEXT NOT NULL` | Designation of the changed element (Part or Assembly) |
| `element_type` | `DeltaElementType` | `element_type` | `INTEGER` | 0=PART, 1=ASSEMBLY |
| `change_type` | `DeltaChangeType` | `change_type` | `INTEGER` | 0=ADD, 1=MODIFY, 2=DELETE |
| `changed_fields` | `QString` | `changed_fields` | `TEXT` | JSON list of changed fields (eg `["drawing","litera"]`) |

**Table `delta_file_changes`** (file changes within one delta_change):

| C++ Field | Type C++ | SQLite Column | Type SQLite | Description |
|---|---|---|---|---|
| `delta_change_id` | `uint64_t` | `delta_change_id` | `INTEGER NOT NULL` | FK → delta_changes.id |
| `file_type` | `DocumentType` | `file_type` | `INTEGER` | Document type (PART_DRAWING, ASSEMBLY_DRAWING, MODEL_3D, ...) |
| `old_object_key` | `QString` | `old_object_key` | `TEXT` | MinIO key before change (empty with ADD) |
| `old_sha256` | `QString` | `old_sha256` | `TEXT` | |
| `new_object_key` | `QString` | `new_object_key` | `TEXT` | MinIO key after change (empty on DELETE) |
| `new_sha256` | `QString` | `new_sha256` | `TEXT` | |

---

## 2. Files: duplication and its meaning

The developer phrased this as “some duplication, but not critical.” This is not duplication - these are three different semantic levels:

| Where is it stored | What stores | Meaning |
|---|---|---|
| `Part.drawing_object_key` / `Assembly.drawing_object_key` | Current object_key **now** | “Here is the file we are working with today” - live state DESIGN |
| `Snapshot.xml_object_key` | object_key of the snapshot XML file | “Here is the recorded composition of the project at the time of version N” - XML ​​contains designations and links to object_key files that existed at that time |
| `delta_file_changes.new_object_key` | object_key of a file in a specific change | “Here's what exactly changed in the file when moving from version N to N+1” |

**Why not duplication:**

- The DESIGN record stores **one** current key. When a drawing is updated, the `drawing_object_key` is overwritten. The old key is not lost in this case - it is fixed in `delta_file_changes.old_object_key`.
- Snapshot XML stores references to object_key files **at the time of snapshot**, not copies of files. Objects in MinIO are immutable by key - the same key always returns the same file. Therefore, the link in Snapshot remains valid even after updating DESIGN.
- Delta stores `{old_key, new_key}` pairs for each changed file - this is the mechanism for iterating through history.

**Final formula:**

```
DESIGN.Part.drawing_object_key == Snapshot_N.xml contains the same key
                                == delta_file_changes.new_object_key (latest Delta)
```

They are equal, but for different reasons. DESIGN - “what is now”, Snapshot - “what was recorded”, Delta - “how we got there”. During the next parsing, DESIGN will receive a new key, Snapshot will capture the old one, and Delta will record the transition.

---

## 3. Approved Snapshot and inter-system links

### What does `active_snapshot_id` mean?

`Project.active_snapshot_id` - FK on Snapshot with status `APPROVED`. This is the “official” version of the project: the one that was accepted for production, procurement, etc.

- While the Snapshot is not approved (`status = DRAFT`), `active_snapshot_id` is NULL or points to a previous APPROVED Snapshot.
- Snapshot approval (approve) = transaction: `snapshot.status → APPROVED` + `project.active_snapshot_id = snapshot.id` + previous APPROVED → `ARCHIVED`.

### How ProductionTask refers to composition

`Task` (in `task.h`) already contains `project_id`. When creating a job, ERP uses `project.active_snapshot_id` to retrieve the specific composition. Correct storage scheme:

```
Task {
    project_id,
snapshot_id ← add: ID of the snapshot captured when creating the job
}
```

This is critical: if Project receives a new APPROVED Snapshot, already created jobs must reference the Snapshot that was in effect **when the job was created**, not the current one. This is already provided in `add_data_convention.md` (`"snapshot_id"` in add_data when CREATE PRODUCTION_TASK) - you need to fix `snapshot_id` as a required field in the `Task` resource itself.

### Snapshot Lifecycle

```
DRAFT → APPROVED → ARCHIVED

DRAFT:    created by PDMManager after parsing; visible only to PDM managers
APPROVED: approved by the responsible person; project.active_snapshot_id -> this Snapshot
other subsystems (ERP, production) receive it as an actual composition
ARCHIVED: when the next version is approved; data is preserved, Tasks reference it
```

Snapshot is never deleted - only archived. This ensures reproducibility: for any historical Task, you can always restore the composition at the time of its creation.

### What happens when a new Snapshot is approved

1. PDMManager creates a new Snapshot (`status = DRAFT`).
2. The person in charge looks at the diff (Delta between the previous and the new) and makes a decision.
3. PDMManager calls `approve(snapshot_id)`:
   - `UPDATE snapshots SET status=ARCHIVED WHERE id = project.active_snapshot_id`
   - `UPDATE snapshots SET status=APPROVED, approved_by=..., approved_at=... WHERE id = snapshot_id`
   - `UPDATE projects SET active_snapshot_id = snapshot_id WHERE id = project_id`
4. Kafka sends NOTIFICATION: `CRUD UPDATE, ResourceType::PROJECT` - all clients update `project.active_snapshot_id`.
5. The designer UI and ERP reread the composition through the new `active_snapshot_id`.

---

## 4. How to iterate through changes

**Question:** If `object_key` is stored in DESIGN records (Part, Assembly), how to go through the change history of a specific file?

**Answer:** via the Delta chain. Delta contains its own links to changed files (`old_object_key` / `new_object_key`) - this is not redundancy, but a versioning mechanism.

### Algorithm: history of a part drawing file by designation

Task: obtain the complete history of the drawing of the part `SB-001-01`.

1. By `designation = 'SB-001-01'` find `Part` in the `parts` table → get `id` and current `drawing_object_key` (this is the “now” state).
2. Define a Snapshot chain for `project_id`:
   ```sql
   SELECT id, version, previous_snapshot_id
   FROM snapshots
   WHERE project_id = ?
   ORDER BY version DESC
   ```
3. For each transition `(snapshot_id → next_snapshot_id)` find Delta:
   ```sql
   SELECT dc.id, dc.change_type, dfc.old_object_key, dfc.new_object_key, dfc.new_sha256
   FROM delta_changes dc
   JOIN delta_file_changes dfc ON dfc.delta_change_id = dc.id
   JOIN deltas d ON d.id = dc.delta_id
   WHERE dc.designation = 'ASM-001-01'
     AND dfc.file_type = 2  -- PART_DRAWING
   ORDER BY d.next_snapshot_id DESC
   ```
4. The result is an ordered list of changes:

| Version (next) | Change type | old_object_key | new_object_key |
   |---|---|---|---|
   | 3 | MODIFY | `drawings/SB-001-01/v2.pdf` | `drawings/SB-001-01/v3.pdf` |
   | 2 | MODIFY | `drawings/SB-001-01/v1.pdf` | `drawings/SB-001-01/v2.pdf` |
   | 1 | ADD | `` | `drawings/SB-001-01/v1.pdf` |

5. To download any version - request the presigned URL for the `object_key` version via `GET_MINIO_PRESIGNED_URL`.

**Important:** the `designation` field in `delta_changes` is not an FK, but a text identifier. This is intentional: Delta describes the state at the time of Snapshot creation, when Part might not yet exist as a DESIGN record (for example, during the first parsing). Communication via `designation` is resistant to id renaming on the server.

---

## 5. PDMManager processes (algorithms)

### 5.1. First parsing / project creation

1. The user specifies the root folder with CD PDF files.
2. PDMManager generates a PDF list → passes the root XMLElement to the drawing compiler library.
3. The compiler returns the completed XML document (an in-memory Snapshot XML structure).
4. PDMManager validates against ESKD → records `error_count`, `warning_count`.
5. Uploading files to MinIO:
- For each PDF: `GET_MINIO_PRESIGNED_URL (PUT)` → `MinioConnector::put(url, localPath)`.
- Save `object_key` and `sha256` for each file.
6. Creating DESIGN records (via ServerConnector, CRUD CREATE):
- `Project` → get `project.id`.
- `Assembly[]` (breadth-first traversal of the tree, parents to children) → get `assembly.id` for each.
- `Part[]` → get `part.id` for each.
- Enter `drawing_object_key`, `drawing_sha256` into each entry.
7. Save the Snapshot XML file in MinIO → get `xml_object_key`, `xml_sha256`.
8. Create a `Snapshot` record (CRUD CREATE): `project_id`, `version=1`, `previous_snapshot_id=NULL`, `xml_object_key`, `status=DRAFT`.
9. No Delta for the first version (no base for comparison).

### 5.2. Re-parse / create Delta + new Snapshot

1. The user starts re-parsing for an existing project.
2. The compiler builds a new XML document in memory (similar to steps 2-3 above).
3. PDMManager loads the current Snapshot XML from MinIO (by `active_snapshot_id.xml_object_key`).
4. **Comparison of two XML documents** (diff by designation):
- New designation → `change_type = ADD`.
- Deleted designation → `change_type = DELETE`.
- Same designation with changed fields → `change_type = MODIFY`. For files: compare SHA-256.
5. Load only changed/new files into MinIO.
6. Update DESIGN records via CRUD UPDATE:
- Modified Part/Assembly: update `drawing_object_key`, `drawing_sha256`, `drawing_modified_at`.
- New Part/Assembly: CRUD CREATE.
- Deleted: CRUD DELETE (soft delete via `actual = false`).
7. Save the new Snapshot XML in MinIO.
8. Create a `Snapshot` record (CRUD CREATE): `version = prev.version + 1`, `previous_snapshot_id = prev.id`, `status = DRAFT`.
9. Create a record `Delta` + `delta_changes[]` + `delta_file_changes[]` (CRUD CREATE).

### 5.3. Snapshot approval

1. A user with PDM manager rights selects Snapshot with DRAFT status.
2. PDMManager requests Delta (if any) to display the diff to the user.
3. User confirms → PDMManager sends:
- `CRUD UPDATE, ResourceType::SNAPSHOT` with `status = APPROVED`, `approved_by`, `approved_at`.
- `CRUD UPDATE, ResourceType::PROJECT` with `active_snapshot_id = snapshot.id`.
- `CRUD UPDATE, ResourceType::SNAPSHOT` for previous APPROVED with `status = ARCHIVED`.
4. Kafka sends NOTIFICATION to all three UPDATEs.
5. DataManager applies the changes on all clients; The designer UI and ERP reread the active composition.

### 5.4. Rollback to a previous version

1. PDMManager finds a Snapshot with the required version (`previous_snapshot_id` chain).
2. Loads its XML from MinIO → builds the Part/Assembly list.
3. For each part/assembly: `CRUD UPDATE` in DESIGN with `object_key` and `sha256` from that version (taken from `delta_file_changes.old_object_key` Delta chain in reverse order).
4. Creates a new Snapshot (version N+1) based on the rolled back composition - the rollback is recorded as a new version, and not deleting history.
5. `status = DRAFT` → goes through the standard approve process.

---

## 6. Link scheme

```
                     ┌─────────────────────────────────────────────────────┐
                     │                   DESIGN                            │
                     │                                                      │
                     │  Project ──────────────────────────────────────────►│ active_snapshot_id ─────►  Snapshot (PDM)
                     │    │                                                 │
                     │    └─► root_assembly_id ──────────────────────────► Assembly
                     │                                                      │    │
                     │                                    child_assemblies ◄┘    │
                     │                                                           └─► Part
                     │                                                                │
                     │                                             drawing_object_key │
                     │                                             model_3d_object_key│
                     └───────────────────────────────────────────────────────────────┘
                                                                           │
                                                                           ▼
MinIO (objects)
                                                                           ▲
                     ┌─────────────────────────────────────────────────────┤
                     │                     PDM                             │
                     │                                                      │
                     │  Snapshot ──────────────────────────────────────────┘
│ │ xml_object_key (XML snapshot file)
                     │    │  project_id ──────────────────────────────────► Project (DESIGN)
│ │ previous_snapshot_id ────────────────────────► Snapshot (yourself)
                     │    │
                     │    └─► Delta
│ │ snapshot_id (basic version)
                     │          │  next_snapshot_id
                     │          │
                     │          └─► delta_changes[]
│ │ designation ──── (text link) ──► Part / Assembly
                     │                └─► delta_file_changes[]
                     │                      old_object_key ──────────────────────► MinIO
                     │                      new_object_key ──────────────────────► MinIO
                     └──────────────────────────────────────────────────────────────────
                     
                     ┌──────────────────────────────────────────────────────────────────┐
│ Intersystem links │
                     │                                                                   │
                     │  Task (GENERATIVE::PRODUCTION)                                   │
                     │    project_id ─────────────────────────────────────► Project     │
                     │    snapshot_id ────────────────────────────────────► Snapshot    │
│ (takes the composition from the Snapshot recorded during creation) │
                     │                                                                   │
                     │  ProcurementRequest (PURCHASES)                                  │
                     │    part_id ────────────────────────────────────────► Part        │
                     │    material_instance_id ───────────────────────────► MaterialInstance │
                     └──────────────────────────────────────────────────────────────────┘
```

---

## 7. What needs to be implemented in the contract/

### 7.1. Existing classes - what to change

| Class | File | Changes |
|---|---|---|
| `Project` | `contract/design/project.h` | Remove `shared_ptr<Assembly> root_assembly`. Add: `uint64_t root_assembly_id`, `std::optional<uint64_t> active_snapshot_id` |
| `Assembly` | `contract/design/assembly.h` | Add: `QString designation`, `QString drawing_object_key`, `QString drawing_sha256`, `QString spec_object_key`, `QString spec_sha256`, `QString mounting_drawing_object_key`, `QString mounting_drawing_sha256`, `QString model_3d_object_key`, `QString model_3d_sha256`. Remove: `shared_ptr<FileVersion>` fields, `assembly_id` (duplicates `id`), `part_id_ctr` |
| `Part` | `contract/design/part.h` | Add: `QString designation`, `QString litera`, `QString organization`, `uint64_t project_id`, `std::optional<uint64_t> material_instance_id`, `QString drawing_object_key`, `QString drawing_sha256`, `QDateTime drawing_modified_at`, `QString model_3d_object_key`, `QString model_3d_sha256`. Remove: `vector<shared_ptr<FileVersion>>` fields, `source_project_id` (rename to `project_id`), `part_id` (duplicates `id`) |
| `FileVersion` | `contract/design/fileversion.h` | **Abolish** as an entity. File version history is now in `Delta`. Move approval workflow fields (proposed_by, approved_by) to `Part`/`Assembly` if necessary |
| `Task` | `contract/plant/task.h` | Add: `uint64_t snapshot_id` (ID of the Snapshot captured when creating the job) |

### 7.2. New classes - create

| Class | File | ResourceType (already exists) |
|---|---|---|
| `Snapshot` | `contract/pdm/snapshot.h` | `ResourceType::SNAPSHOT = 50` ✓ |
| `Delta` | `contract/pdm/delta.h` | `ResourceType::DELTA = 51` ✓ |

**`Snapshot`** - inherits from `ResourceAbstract`. Fields: `project_id`, `version`, `previous_snapshot_id`, `xml_object_key`, `xml_sha256`, `status` (enum `SnapshotStatus`), `approved_by`, `approved_at`, `error_count`, `warning_count`.

**`Delta`** - inherits from `ResourceAbstract`. Fields: `snapshot_id`, `next_snapshot_id`, `changes_count`. Nested structures:

```cpp
struct DeltaFileChange {
    uint64_t       id;
    DocumentType   file_type;
    QString        old_object_key;
    QString        old_sha256;
    QString        new_object_key;
    QString        new_sha256;
};

struct DeltaChange {
    uint64_t                     id;
    QString                      designation;
    DeltaElementType             element_type;   // PART / ASSEMBLY
    DeltaChangeType              change_type;    // ADD / MODIFY / DELETE
    QStringList                  changed_fields;
    std::vector<DeltaFileChange> file_changes;
};

class Delta : public ResourceAbstract {
    uint64_t                    snapshot_id;
    uint64_t                    next_snapshot_id;
    std::vector<DeltaChange>    changes;
    // ...
};
```

**New enums** (add to `contract/pdm/pdmtypes.h` or similar file):

```cpp
enum class SnapshotStatus : uint8_t { DRAFT = 0, APPROVED = 1, ARCHIVED = 2 };
enum class DeltaElementType : uint8_t { PART = 0, ASSEMBLY = 1 };
enum class DeltaChangeType  : uint8_t { ADD = 0, MODIFY = 1, DELETE = 2 };
```

### 7.3. Auxiliary Part/Assembly structures - add

```cpp
// In contract/design/part.h
struct PartConfig {
    QString config_id;   // "01", "02", ...
    double  length_mm = 0;
    double  width_mm  = 0;
    double  height_mm = 0;
    double  mass_kg   = 0;
};

struct PartSignature {
    QString role;
    QString name;
    QDateTime date;
};
```

### 7.4. ResourceType - status

All necessary `ResourceType` are already defined in `uniterprotocol.h`:
- `PROJECT = 30`, `ASSEMBLY = 31`, `PART = 32` - yes ✓
- `SNAPSHOT = 50`, `DELTA = 51` - yes ✓

Add `Subsystem::PDM = 7` - already available ✓

### 7.5. Contract/ directory structure after changes

```
contract/
├── design/
│   ├── project.h / .cpp
│   ├── assembly.h / .cpp
│   ├── part.h / .cpp
│ └── designtypes.h ← AssemblyType, DocumentType (from fileversion.h), PartConfig, PartSignature
├── pdm/
│ ├── snapshot.h / .cpp ← new
│ ├── delta.h / .cpp ← new
│ └── pdmtypes.h ← new: SnapshotStatus, DeltaElementType, DeltaChangeType
├── resourceabstract.h / .cpp
├── unitermessage.h / .cpp
└── uniterprotocol.h
```

`fileversion.h` - delete after moving `DocumentType` to `designtypes.h`.
