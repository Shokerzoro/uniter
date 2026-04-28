# Subsystem::PDM

Version control subsystem (Product Data Management). Stores
**immutable** snapshots of design projects (snapshots) and
incremental changes between them (deltas).

Code: `src/uniter/contract/pdm/`.
Scheme: `PDM.pdf`. Logic for interaction with DESIGN: `pdm_design_logic.md`.

## Key principle

The PDM subsystem contains:

1. Own resources: `pdm_project`, `pdm_snapshot`, `pdm_delta`.
2. **Mirror copies** of DESIGN resources - when creating a snapshot, all
related `design_*` records are copied to parallel `pdm_*_snapshot`
tables are “frozen” and tied to a specific `snapshot_id`.

This ensures the immutability of snapshots: after creating a snapshot, any
editing in DESIGN does not change what was frozen.

PDM resources that are not mirrors behave like regular CRUD resources.
Mirrors are written by PDMManager when creating a snapshot and are no longer changed.

## Subsystem resources (own)

| Class | ResourceType | Table |
|---|---|---|
| `PdmProject` | `PDM_PROJECT = 52` | `pdm_project` |
| `Snapshot` | `SNAPSHOT = 50` | `pdm_snapshot` |
| `Delta` | `DELTA = 51` | `pdm_delta` |
| `Diagnostic` | `DIAGNOSTIC = 57` | `pdm_diagnostic` |

## Mirror tables (copies of DESIGN in the context of a snapshot)

| Table | Role | Mapped to ResourceType |
|---|---|---|
| `pdm_assembly` | A copy of `design_assembly` at the time of snapshot | `ASSEMBLY_PDM = 53` |
| `pdm_assembly_config` | Copy of `design_assembly_config` | `ASSEMBLY_CONFIG_PDM = 54` |
| `pdm_part` | Copy of `design_part` | `PART_PDM = 55` |
| `pdm_part_config` | Copy of `design_part_config` | `PART_CONFIG_PDM = 56` |
| `pdm_assembly_config_parts` | join | — |
| `pdm_assembly_config_children` | join | — |
| `pdm_assembly_config_standard_products` | join | — |
| `pdm_assembly_config_materials` | join | — |
| `pdm_snapshot_diagnostics` | join snapshot ↔ diagnostic | — |

**Numbering of PDM mirrors.** Previously codes `ASSEMBLY_PDM` / `ASSEMBLY_CONFIG_PDM` /
`PART_PDM` / `PART_CONFIG_PDM` were 35–38 (DESIGN range). After refactoring
they are moved to the PDM range (50–59): 53 / 54 / 55 / 56 respectively.
This aligns the codes with the actual owning subsystem.

Resource classes are **reused** from the DESIGN subsystem (`contract::design::*`).
The marker “this is a PDM copy” is the ResourceType and the table in which it lies.
At the field level, the pdm resource differs only in the addition of FK `snapshot_id`
to the owning snapshot.

To map to different tables with one class, use an explicit parameter
`table_suffix` or `subsystem_context` in DataManager under CRUD (this is
DataManager agreement; the resources themselves are the same).

TODO (to think about it): if it turns out that the PDM mirror should have significantly
other fields (for example, `approved_at`, `approved_by` at the level
pdm_assembly_config), introduce separate classes `pdm::AssemblyConfig`
etc. Now the differences come down only to FK `snapshot_id`, so
let's reuse the design::* classes.

## Table `pdm_project` - PDM project (versioned branch)

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `base_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` - first snapshot of the branch |
| `head_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` - current (current) snapshot |

At the time of PdmProject creation, both `*_snapshot_id == NULL`. After the first
parsing PDMManager creates `Snapshot #1` and sets both to its id.
Next, `head_snapshot_id` moves when new snapshots are added,
`base_snapshot_id` remains unchanged.

Feedback from `design_project.pdm_project_id`
(one design_project ↔ one pdm_project).

## Table `pdm_snapshot` - immutable slice

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `prev_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` - previous |
| `next_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` - next |
| `prev_delta_id` | INTEGER NULL | FK → `pdm_delta.id` - delta that resulted in this snapshot |
| `next_delta_id` | INTEGER NULL | FK → `pdm_delta.id` - delta that goes from here |
| `root_assembly_id` | INTEGER NOT NULL | FK → `pdm_assembly.id` - root assembly (frozen copy!) |

**IMPORTANT:** `root_assembly_id` points to `pdm_assembly.id`, not to
`design_assembly.id`. This ensures that after any edits to
The `design_assembly` snapshot will remain consistent.

`prev_delta_id` / `next_delta_id` duplicate two-way communication with
sides `pdm_delta` - this speeds up iteration through history (in both directions)
without complex JOINs.

Diagnostics in Snapshot are no longer denormalized (fields removed
`error_count` / `warning_count`). Instead, a separate resource has been introduced
`Diagnostic` (see below) and N:M communication via `pdm_snapshot_diagnostics`.
The `Snapshot` runtime class collapses this connection into
`std::vector<std::shared_ptr<Diagnostic>> diagnostics`.

TODO (to think about): application fields `version`, `status` (DRAFT/APPROVED/
ARCHIVED), `approved_by`, `approved_at` are not required by the structure diagram,
but are needed for the snapshot work cycle (life cycle DRAFT → APPROVED
→ ARCHIVED). Left in the class as application attributes.

## Table `pdm_delta` - incremental change

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `prev_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` - from which snapshot |
| `next_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` - to which snapshot |
| `prev_delta_id` | INTEGER NULL | FK → `pdm_delta.id` - adjacent delta on the left |
| `next_delta_id` | INTEGER NULL | FK → `pdm_delta.id` - adjacent delta on the right |
| `changed_elements` | TEXT/JSON | List of changed elements (see below) |

`changed_elements` - “what exactly has changed.” In the database it is expanded to
two separate tables:

| Table | Destination |
|---|---|
| `pdm_delta_changes` | Pairs “was → became”: `(delta_id, element_type, old_ref, new_ref)` |
| `pdm_delta_removed` | Deleted: `(delta_id, element_type, element_ref)` |

`element_type` — enum value `DeltaElementType` (ASSEMBLY=0,
ASSEMBLY_CONFIG=1, PART=2, PART_CONFIG=3). `*_ref` — FK to the corresponding
PDM mirror table (`pdm_assembly.id`, `pdm_assembly_config.id`,
`pdm_part.id`, `pdm_part_config.id`).

In the `Delta` runtime class, these two tables are collapsed into:

```cpp
struct DeltaKey { DeltaElementType type; uint64_t id; };
using ChangesMap = std::map<
    DeltaKey,
    std::pair<std::shared_ptr<ResourceAbstract>,  // was
              std::shared_ptr<ResourceAbstract>>  // became
>;
struct RemovedItem { DeltaElementType type; std::shared_ptr<ResourceAbstract> item; };

ChangesMap changes;                 // pdm_delta_changes
std::vector<RemovedItem> removed;   // pdm_delta_removed
```

This structure allows PDMManager to iterate over deltas and
apply changes sequentially: for each pair (old → new)
replace the link, for each element in removed - delete. Old
`DeltaChange` / `DeltaFileChange` structures removed - file changes
are expressed through the replacement PART/PART_CONFIG, rather than as a separate entry.

## Table `pdm_diagnostic` - error/observation of CD parsing

A separate PDM resource (`ResourceType::DIAGNOSTIC = 57`). Replaces previously
`error_count` / `warning_count` fields denormalized in Snapshot.

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `severity` | INTEGER | ERROR / WARNING / INFO |
| `category` | INTEGER | FILE_SYSTEM / VERSION_CONTROL / PARSING / VALIDATION / … |
| `type` | TEXT | String code ("NO_FILE", "INFORMAL_CHANGE", ...) |
| `path` | TEXT | Object path/address (PDF file, designation, XPath) |
| `message` | TEXT | Human-readable message (optional) |

`severity` and `category` are enums in C++ (`DiagnosticSeverity`,
`DiagnosticCategory` in `pdm/pdmtypes.h`). `type` is left as a string to
do not grow enum when adding new types of diagnostics.

### Join `pdm_snapshot_diagnostics` (N:M)

| Column | Type | Description |
|---|---|---|
| `snapshot_id` | INTEGER | FK → `pdm_snapshot.id` |
| `diagnostic_id` | INTEGER | FK → `pdm_diagnostic.id` |

One snapshot can have N diagnostics; one Diagnostic can
occur in several snapshots (if the problem repeats between
versions). At runtime `Snapshot` stores
`std::vector<std::shared_ptr<Diagnostic>> diagnostics`; Diagnostic of their
`snapshot_id` does not hold - this side of the connection is restored
DataManager with a query on a join table.

## Runtime view: intra-PDM communication via `shared_ptr`

In the database, all Snapshot↔Delta↔PdmProject↔root_assembly connections are stored as
FK `uint64_t`. But in the runtime classes `pdm::Snapshot`, `pdm::Delta`,
`pdm::PdmProject` these connections are collapsed into `std::shared_ptr<...>`
for the classes themselves:

* `Snapshot::prev_snapshot`, `Snapshot::next_snapshot` → `shared_ptr<Snapshot>`
* `Snapshot::prev_delta`, `Snapshot::next_delta` → `shared_ptr<Delta>`
* `Snapshot::root_assembly` → `shared_ptr<design::Assembly>`
* `Delta::prev_snapshot`, `Delta::next_snapshot` → `shared_ptr<Snapshot>`
* `Delta::prev_delta`, `Delta::next_delta` → `shared_ptr<Delta>`
* `PdmProject::base_snapshot`, `PdmProject::head_snapshot` → `shared_ptr<Snapshot>`

This is necessary so that PDMManager can iterate along the version chain
(`pdmProject.base_snapshot → next_snapshot → ...`) without additional
requests to the DataManager at each iteration and, if necessary, apply
deltas to snapshots directly.

When loading from the DataManager database:
1. Collects all `Snapshot` / `Delta` / `PdmProject`.
2. Using FK, restores pointers between objects.
3. When saving, reads `.get()` / `.id` from pointers and writes FK.

`operator==` for Snapshot / Delta / PdmProject compares pointers by
address (`.get()`) - this avoids recursion in bidirectional
list snapshot↔delta.

## Connections (review)

```
pdm_project                  1 ─ 1 pdm_snapshot (base_snapshot_id)
pdm_project                  1 ─ 1 pdm_snapshot (head_snapshot_id)
pdm_snapshot                 1 ─ 0..1 pdm_snapshot (prev_snapshot_id)
pdm_snapshot                 1 ─ 0..1 pdm_snapshot (next_snapshot_id)
pdm_snapshot                 1 ─ 0..1 pdm_delta    (prev_delta_id)
pdm_snapshot                 1 ─ 0..1 pdm_delta    (next_delta_id)
pdm_delta                    N ─ 0..1 pdm_snapshot (prev_snapshot_id)
pdm_delta                    N ─ 0..1 pdm_snapshot (next_snapshot_id)
pdm_snapshot                 1 ─ 1 pdm_assembly    (root_assembly_id)
pdm_assembly                 N ─ 1 pdm_snapshot    (snapshot_id)
pdm_part                     N ─ 1 pdm_snapshot    (snapshot_id)
pdm_assembly_config          N ─ 1 pdm_snapshot    (snapshot_id)
pdm_part_config              N ─ 1 pdm_snapshot    (snapshot_id)
pdm_snapshot                 N ─ M pdm_diagnostic  (join: pdm_snapshot_diagnostics)
pdm_delta                    1 ─ N pdm_delta_changes  (delta_id)
pdm_delta                    1 ─ N pdm_delta_removed  (delta_id)
design_project               1 ─ 0..1 pdm_project  (pdm_project_id)
```

### PDM resource metadata

| Class | Subsystem | GenSubsystem | ResourceType |
|---|---|---|---|
| `PdmProject` | PDM | NOTGEN | PDM_PROJECT (52) |
| `Snapshot` | PDM | NOTGEN | SNAPSHOT (50) |
| `Delta` | PDM | NOTGEN | DELTA (51) |
| `Diagnostic` | PDM | NOTGEN | DIAGNOSTIC (57) |

The fields are filled in automatically in the constructor of each
successor of `ResourceAbstract` and are not stored in the database (they are determined by the table).
