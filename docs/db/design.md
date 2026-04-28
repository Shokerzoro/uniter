# Subsystem::DESIGN

Design subsystem. Source of truth for **current** state
company products. Projects, assemblies, parts, their configurations. CD files
are linked via `documents_doc_link`.

Code: `src/uniter/contract/design/`.
Scheme: `DESIGN.pdf`. Logic for interaction with PDM: `pdm_design_logic.md`.

## Key principle (new)

`design` stores **all possible** projects, assemblies, parts created
in the company. The structure of assemblies is moved to the configuration level:

* `design_assembly` contains **only general assembly data** (designation,
name, description, type - everything that does not depend on execution).
* `design_assembly_config` contains the **assembly structure** of a specific
executions: what subassemblies, parts, standard products, materials in
they enter her.
* `design_part` - part (general data).
* `design_part_config` - design of the part with its own size/weight.

M:N relationships between `assembly_config` and its contents (parts, child-assemblies,
standard-products, materials) - through separate join tables; in a C++ class
they are materialized into `std::vector<...>` at runtime.

## Subsystem resources

| Class | ResourceType | Table(s) |
|---|---|---|
| `Project` | `PROJECT = 30` | `design_project` |
| `Assembly` | `ASSEMBLY = 31` | `design_assembly` |
| `AssemblyConfig` | `ASSEMBLY_CONFIG = 33` | `design_assembly_config` + join-tables |
| `Part` | `PART = 32` | `design_part` |
| `PartConfig` | `PART_CONFIG = 34` | `design_part_config` |

## Table `design_project` - project

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `name` | TEXT | |
| `description` | TEXT | |
| `designation` | TEXT | Project designation according to ESKD (top level) |
| `root_assembly_id` | INTEGER NOT NULL | FK → `design_assembly.id` - root assembly |
| `pdm_project_id` | INTEGER NULL | FK → `pdm_project.id` - associated PDM project (versions, snapshots) |

**Replacement:** used to be `active_snapshot_id` (FK to `pdm_snapshot`).
The project now refers to the PDM project as a whole (`pdm_project_id`),
and the “which version of head” control is inside `pdm_project`.

NULL `pdm_project_id` means “the project has not yet launched a versioned
PDM branch"; such a project is freely edited, there are no snapshots.

## Table `design_assembly` - assembly (general data)

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `project_id` | INTEGER NOT NULL | FK → `design_project.id` |
| `designation` | TEXT | Designation according to ESKD |
| `name` | TEXT | |
| `description` | TEXT | |
| `type` | INTEGER | `AssemblyType` (REAL/VIRTUAL) |

**NOT HERE:** `parent_assembly_id`, `child_assemblies[]`, `parts[]`.
Parenting and content are in `design_assembly_config`.

**Documents.** Files (assembly drawing, specification, 3D) are linked
via `documents_doc_link` (`target_type=ASSEMBLY`, `target_id=assembly.id`).
In the PDF diagram this is shown as `doc_link_id(FK) - mult` - such a “column”
actually means M:N via callbacks: one Assembly has
several DocLinks. In a C++ class - `std::vector<DocLink> linked_documents`
(denormalization for UI).

## Table `design_assembly_config` - assembly execution (structure)

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `assembly_id` | INTEGER NOT NULL | FK → `design_assembly.id` - which assembly this design belongs to |
| `config_code` | TEXT | Execution code (“01”, “02”, ...) |

This is a "build execution" entry. Contents (M:N) are stored in separate
join tables:

### Join `design_assembly_config_parts`
| Column | Type | Description |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` |
| `part_id` | INTEGER | FK → `design_part.id` |
| `quantity` | INTEGER | Number of occurrences |
| `part_config_code` | TEXT NULL | Selected part design (FK to `design_part_config.config_code` via `part_id`) |

### Join `design_assembly_config_children` (subassemblies)
| Column | Type | Description |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` - parent |
| `child_assembly_id` | INTEGER | FK → `design_assembly.id` - child assembly |
| `quantity` | INTEGER | |
| `child_config_code` | TEXT NULL | Selected child assembly execution |

### Join `design_assembly_config_standard_products`
Standard products - links to **simple** Instance. According to ESKD standard
the product (for example, “Bolt M8×30 ​​GOST 7798-70”) is completely described by one
standard - there is no division into “range + brand”, the whole set
characteristics are specified by prefixes/suffixes within one simple
Template. Therefore, the link goes to `material_instances_simple`, and not
to composite.

| Column | Type | Description |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` |
| `instance_simple_id` | INTEGER | FK → `material_instances_simple.id` |
| `quantity` | INTEGER | |

### Join `design_assembly_config_materials`
Materials (sheets, rods, wire, etc.) - also links to **simple**
Instance: sheet, rod, etc. - These are one-part standard materials.
Composite-Instance (sortment + brand) describes a semi-finished product (for example,
"Circle 20 / Steel 20"); such references are not expected in `assembly_config`
(the semi-finished product is expanded to the material via PartConfig).

| Column | Type | Description |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` |
| `instance_simple_id` | INTEGER | FK → `material_instances_simple.id` |
| `quantity_items` | INTEGER NULL | for PIECE |
| `quantity_length` | REAL NULL | for LINEAR |
| `quantity_area` | REAL NULL | for AREA |

> Both standard_products and materials in `assembly_config` refer to
> to `material_instances_simple`. Splitting into two join tables
> saved solely for the sake of ESKD semantics (standard product vs
> material); Technically, both connections are simple-instance.
> Composite Instance is reserved for semi-finished products (“range + brand”)
> and binds to Part, not AssemblyConfig.

In C++, the content is materialized in `AssemblyConfig` as:
```cpp
std::vector<AssemblyConfigPartRef>     parts;
std::vector<AssemblyConfigChildRef>    child_assemblies;
std::vector<AssemblyConfigStandardRef> standard_products; // ref → InstanceSimple
std::vector<AssemblyConfigMaterialRef> materials;         // ref → InstanceSimple
```

## Collapse `Assembly.configs` / `Part.configs` in C++

In the database, the connection `design_assembly` → `design_assembly_config` (1:M) is represented
FK `assembly_config.assembly_id`. In the C++ class `design::Assembly` this relationship
collapsed into `std::vector<AssemblyConfig> configs` - execution vector
materialized by the DataManager when the assembly is loaded. Same for
`Part` → `PartConfig` (`std::vector<PartConfig> configs` in `Part`).

Class ≠ table: in FK runtime from `*_config` tables to its parent
the record is not stored - the configuration position in the parent container is already
knows who it belongs to. When writing to the database, DataManager restores
FK from object position.

## Table `design_part` - part (general data)

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `project_id` | INTEGER NOT NULL | FK → `design_project.id` |
| `designation` | TEXT | Designation ESKD |
| `name` | TEXT | |
| `description` | TEXT | |
| `litera` | TEXT | Letter KD (O/O1/A/…) |
| `organization` | TEXT | Development organization |
| `instance_simple_id` | INTEGER NULL | FK → `material_instances_simple.id` - part material |
| `instance_composite_id` | INTEGER NULL | FK → `material_instances_composite.id` - composite material of the part |

**Invariant:** `instance_simple_id` XOR `instance_composite_id` (can be
and “none” for abstract specification-level details, but
both at the same time is prohibited).

**NO HERE:** `assembly_id`. The part does NOT belong to the same assembly -
it can be included in multiple builds/executions via join tables
`design_assembly_config_parts`.

Documents - via `documents_doc_link` (`target_type=PART`).

Drawing signatures (developed / checked / ...) in the runtime class Part
are no longer stored - they are not part of the design information,
the required DESIGN subsystem. To reproduce the signature in the issued
complete CD, this data is taken from the metadata PDF files associated
with `design_part` via `documents_doc_link`.

## Table `design_part_config` - part design

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `part_id` | INTEGER NOT NULL | FK → `design_part.id` |
| `config_code` | TEXT | «01», «02», … |
| `length_mm` | REAL | |
| `width_mm` | REAL | |
| `height_mm` | REAL | |
| `mass_kg` | REAL | |

In C++: the `design::PartConfig` resource is a full-fledged ResourceAbstract descendant
(ResourceType::PART_CONFIG). Previously it was a nested struct - now
separate resource, CRUD goes through UniterMessage.

## Connections (review)

```
design_project              1 ─ 1 design_assembly          (root_assembly_id)
design_project              1 ─ 0..1 pdm_project           (pdm_project_id)
design_assembly             1 ─ N design_assembly_config   (assembly_id)
design_assembly_config      N ─ M design_part              (join: ..._parts)
design_assembly_config      N ─ M design_assembly          (join: ..._children)
design_assembly_config      N ─ M material_instances_simple (join: ..._standard_products)
design_assembly_config      N ─ M material_instances_simple (join: ..._materials)
design_part                 N ─ 1 design_project           (project_id)
design_part                 1 ─ N design_part_config       (part_id)
design_part                 N ─ 1 material_instances_simple    (instance_simple_id)
design_part                 N ─ 1 material_instances_composite (instance_composite_id)
```

### Resource metadata

DESIGN resources in C++ carry generic metadata `subsystem/gen_subsystem/
resource_type` (filled in automatically in the constructor of each
successor of `ResourceAbstract`):

| Class | Subsystem | GenSubsystem | ResourceType |
|---|---|---|---|
| `Project` | DESIGN | NOTGEN | PROJECT |
| `Assembly` | DESIGN | NOTGEN | ASSEMBLY |
| `AssemblyConfig` | DESIGN | NOTGEN | ASSEMBLY_CONFIG |
| `Part` | DESIGN | NOTGEN | PART |
| `PartConfig` | DESIGN | NOTGEN | PART_CONFIG |

These fields are not stored in the database - they are uniquely defined by the table.
Runtime uses them for routing CRUD messages and logging.
