# Interaction logic DESIGN ↔ PDM

The document describes how the DESIGN subsystems interact (the source of truth
for the current state of the designer) and PDM (versioning via
immutable snapshots and deltas). Accompanying documents:
`design.md`, `pdm.md`.

## 1. Key idea

* **DESIGN** stores the **current** (current, editable) state
projects, assemblies, parts and their executions. Tables:
  `design_project`, `design_assembly`, `design_assembly_config` (+ join),
  `design_part`, `design_part_config`.
* **PDM** stores **frozen copies** of DESIGN data at the time of parsing
(snapshot) and **incremental deltas** between snapshots. Tables:
`pdm_project`, `pdm_snapshot`, `pdm_delta`, and **mirror tables**
  `pdm_assembly`, `pdm_assembly_config`, `pdm_part`, `pdm_part_config`
(+ pdm options for join tables).
* The transition DESIGN → PDM is implemented by **copying**: when creating a snapshot
all current DESIGN records related to the project are copied to
corresponding pdm tables with FK `snapshot_id` affixed.

## 2. Why two sets of tables (design_* and pdm_*)

If the snapshot referenced `design_assembly.id` directly, any
editing the current build would corrupt the snapshot contents
(violation of invariance). To avoid this:

1. PDMManager creates new lines in `pdm_assembly` / `pdm_part` /
`pdm_assembly_config` / `pdm_part_config`, copying all data from
corresponding `design_*` entries.
2. `pdm_snapshot.root_assembly_id` points to the id from `pdm_assembly`,
not from `design_assembly`.
3. After this, `design_*` can be freely edited - the snapshot is not
will lose integrity.

Classes in C++ are reused (`unit::contract::design::Assembly`,
`AssemblyConfig`, `Part`, `PartConfig`). Difference between "DESIGN record"
and “PDM copy” is marked:

* `ResourceType` in UniterMessage: `ASSEMBLY` vs `ASSEMBLY_PDM`,
`ASSEMBLY_CONFIG` vs `ASSEMBLY_CONFIG_PDM`, etc.;
* destination table in DataManager: `design_assembly` vs `pdm_assembly`.

The class fields themselves are the same; The PDM copy additionally stores FK
`snapshot_id` - this field lives in the database table and is passed in the C++ class
via a separate DataManager context or, if necessary, via
`std::optional<uint64_t> snapshot_id` (TODO: decide upon implementation
DataManager).

## 3. Mechanism for creating snapshot

Called by PDMManager when parsing the project's PDF directory.

Input: `design_project.id` (identifier of the project for which
a snapshot is created). Two cases are considered.

### 3.1. First snapshot of the project

```
design_project.pdm_project_id == NULL  (PDM branch has not been started yet)
```

Steps:

1. PDMManager creates a new `pdm_project` with `base_snapshot_id = NULL`,
   `head_snapshot_id = NULL`.
2. Creates a new `pdm_snapshot`:
   - `prev_snapshot_id = NULL`, `next_snapshot_id = NULL`,
     `prev_delta_id = NULL`, `next_delta_id = NULL`;
   - `version = 1`, `status = DRAFT`.
3. **Copies** into pdm tables all current DESIGN entities related to
to `design_project.id`:
- `design_assembly` → `pdm_assembly` (with snapshot_id = new snapshot);
   - `design_assembly_config` → `pdm_assembly_config`;
   - `design_part` → `pdm_part`;
   - `design_part_config` → `pdm_part_config`;
- `design_assembly_config_*` (four join tables) → `pdm_assembly_config_*`.
When copying, FKs inside pdm lines are rewritten to the id of new ones
pdm lines (id design_* is no longer valid).
4. Finds the copied root assembly in `pdm_assembly` (according to
`design_project.root_assembly_id`) and writes its id in
   `pdm_snapshot.root_assembly_id`.
5. Updates `pdm_project`:
`base_snapshot_id = head_snapshot_id = new snapshot.id`.
6. Updates `design_project.pdm_project_id = new pdm_project.id`.

The delta is **not created** at this step (first snapshot).

### 3.2. Subsequent Snapshots

```
design_project.pdm_project_id != NULL
pdm_project.head_snapshot_id  != NULL  (there is a previous head)
```

Steps:

1. A new `pdm_snapshot` is created (as in 3.1.2, but `version = prev.version + 1`,
   `prev_snapshot_id = pdm_project.head_snapshot_id`).
2. The current DESIGN entities of the project are copied into pdm tables (as in 3.1.3).
3. `pdm_snapshot.root_assembly_id` is set to the id of the copied
root assembly in `pdm_assembly`.
4. PDMManager forms `pdm_delta` between the previous head and the new snapshot:
   - `prev_snapshot_id = prev_head.id`,
     `next_snapshot_id = new_snapshot.id`;
- `prev_delta_id` — id of the previous delta (prev_head had `next_delta_id = NULL`
before this step; after this step, a new delta is written in it,
and in the new delta - its prev_delta_id points to the previous delta
in the chain, if it exists);
- `next_delta_id = NULL` (until the next delta is created).
5. Cross FKs are updated:
   - `prev_head.next_snapshot_id = new_snapshot.id`,
     `prev_head.next_delta_id = new_delta.id`;
   - `new_snapshot.prev_delta_id = new_delta.id`.
6. `pdm_project.head_snapshot_id = new_snapshot.id`
(`base_snapshot_id` remains the same).

## 4. Iterate through version history

A chain of snapshots and deltas forms a **doubly linked list** inside one
`pdm_project`:

```
 (base) snapshot_1  ←── delta_1 ──→  snapshot_2  ←── delta_2 ──→  snapshot_3  (head)
```

Each element of the chain knows its neighbors:

| Resource | Navigation fields |
|--------------|---------------------------------------------------------------|
| `pdm_snapshot` | `prev_snapshot_id`, `next_snapshot_id`, `prev_delta_id`, `next_delta_id` |
| `pdm_delta`    | `prev_snapshot_id`, `next_snapshot_id`, `prev_delta_id`, `next_delta_id` |

The `prev_delta_id / next_delta_id` fields of the snapshot **duplicate** two-way
communication from `pdm_delta` side (normalization sacrificed for speed
iterations in both directions without JOINs).

Iterate forward: `snap → snap.next_delta_id → delta.next_snapshot_id → ...`
Iterate backwards: `snap → snap.prev_delta_id → delta.prev_snapshot_id → ...`

## 5. Communication design_project ↔ pdm_project

```
design_project.pdm_project_id  (nullable FK → pdm_project.id)
```

* `NULL` = the project has **not running** versioned PDM branch
(there are no snapshots; the project is freely edited).
* non-NULL = PDM branch is running, there is at least one snapshot.

Field `design_project.active_snapshot_id` from the previous version of the schema
**deleted**. The question “which Snapshot is relevant” is resolved internally
`pdm_project.head_snapshot_id` and is not duplicated in DESIGN.

Other subsystems (PRODUCTION, ERPManager) requiring “approved
composition", you should ask:
```
design_project.pdm_project_id → pdm_project.head_snapshot_id → pdm_snapshot
```

The snapshot's `DRAFT/APPROVED/ARCHIVED` status is TODO: left in class
as an application attribute, but not required by the structure diagram. When implementing
approval workflow it will be possible to:

* or store status inside pdm_snapshot (as it is now in the class);
* or enter a separate `pdm_project.approved_snapshot_id` next to
`head_snapshot_id` (head = last created, approved = last
approved).

The decision is postponed until work on PDMManager.

## 6. CRUD on UniterMessage - what the DataManager writes where

| ResourceType | Table | Recorded at |
|-------------------------|-------------------------------|---------------------------|
| `PROJECT = 30` | `design_project` | User CUD |
| `ASSEMBLY = 31` | `design_assembly` | User CUD |
| `ASSEMBLY_CONFIG = 33` | `design_assembly_config` + join | User CUD |
| `PART = 32` | `design_part` | User CUD |
| `PART_CONFIG = 34` | `design_part_config` | User CUD |
| `PDM_PROJECT = 52`      | `pdm_project`                 | CUD PDMManager            |
| `SNAPSHOT = 50`         | `pdm_snapshot`                | C PDMManager (immutable)  |
| `DELTA = 51`            | `pdm_delta`                   | C PDMManager (immutable)  |
| `ASSEMBLY_PDM = 35` | `pdm_assembly` | C PDMManager with snapshot |
| `ASSEMBLY_CONFIG_PDM=36`| `pdm_assembly_config` + join | C PDMManager with snapshot |
| `PART_PDM = 37` | `pdm_part` | C PDMManager with snapshot |
| `PART_CONFIG_PDM = 38` | `pdm_part_config` | C PDMManager with snapshot |

DataManager defines a table by the pair `(Subsystem, ResourceType)` in
UniterMessage: `Subsystem::DESIGN + ASSEMBLY` → `design_assembly`,
`Subsystem::PDM + ASSEMBLY_PDM` → `pdm_assembly` etc.

Entries to mirror pdm tables (`ASSEMBLY_PDM`, ...) are usually made
ONLY by PDMManager within the snapshot creation transaction. After
After creating a snapshot, these lines are no longer editable (invariance).
