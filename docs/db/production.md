# Subsystem::GENERATIVE + GenSubsystem::PRODUCTION

Generative subsystem “for each site”. `Plant` from
`Subsystem::MANAGER` spawns its PRODUCTION subsystem with
`GenSubsystemId == plant.id`.

Code: `src/uniter/contract/production/`.
Schematic: `PRODUCTION.pdf`.

## Subsystem resources

| Class | ResourceType | Table |
|---|---|---|
| `ProductionStock` | `PRODUCTION_STOCK = 71` | `production_stock` |
| `ProductionSupply` | `PRODUCTION_SUPPLY = 72` | `production_supply` |
| `ProductionTask` | `PRODUCTION_TASK = 70` | `production_task` |

## Table `production_stock` - warehouse item

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `plant_id` | INTEGER NOT NULL | FK → `manager_plant.id` - owner site |
| `instance_simple_id` | INTEGER NULL | FK → `material_instances_simple.id` |
| `instance_composite_id` | INTEGER NULL | FK → `material_instances_composite.id` |
| `quantity_items` | INTEGER | pieces (DimensionType::PIECE) |
| `quantity_length` | REAL | meters (DimensionType::LINEAR) |
| `quantity_area` | REAL | m² (DimensionType::AREA) |

**Invariant:** `instance_simple_id` XOR `instance_composite_id`
(Stock is a position on a specific Instance; the general “stuff as a whole” on
warehouse is not tracked).

Which dimension field is active is determined by `dimension_type`
corresponding MaterialInstance/Template.

## Table `production_supply` - receipt (acceptance)

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `plant_id` | INTEGER NOT NULL | FK → `manager_plant.id` |
| `purchase_complex_id` | INTEGER NOT NULL | FK → `supply_purchase_complex.id` |

Links the complex purchasing requisition to the receiving site.
Conceptually: “delivery has arrived at site X based on order Y.”

TODO (to think about it): here you will probably need the `received_at` fields,
`quantity_received` etc. - but they are not indicated in the current scheme,
added during expansion.

## Table `production_task` - production task

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `plant_id` | INTEGER NOT NULL | FK → `manager_plant.id` |
| `snapshot_original_id` | INTEGER NOT NULL | FK → `pdm_snapshot.id` — snapshot for which the task was created |
| `snapshot_current_id` | INTEGER NOT NULL | FK → `pdm_snapshot.id` - current snapshot used for production |

`snapshot_original_id` and `snapshot_current_id` match at the time of creation
tasks. They disperse when a new APPROVED “rolls” over the task
Snapshot of the same project: it is recorded from which CD the task started (original)
and which one is currently running (current). This ensures reproducibility
history and at the same time transition to a fresh CD without “losing roots.”

TODO (to think about): fields `code`, `name`, `quantity`, `status`,
`planned_start`/`planned_end`, `actual_start`/`actual_end` - they are in
old model and needed for ERP. They are not in PRODUCTION.pdf (structural),
since the diagram only shows relationships between tables. Leave it in class
with the status “application task attributes”, which do not violate the key structure.

## What was removed from the old version

Tree structures `ProductionAssemblyNode` and `ProductionPartNode`
abolished. They denormalized the CD tree within one record and
duplicated links from `design_assembly` / `design_part`. If necessary
track progress by node - these are separate resources
`ProductionTaskAssemblyProgress` and `ProductionTaskPartProgress` with FK
`production_task_id` (this is outside the current model, but is committed as a TODO).

The file `productiontypes.h` is saved - only
`TaskStatus`-enum.

## Connections

```
manager_plant               1 ─ N production_stock     (plant_id)
manager_plant               1 ─ N production_supply    (plant_id)
manager_plant               1 ─ N production_task      (plant_id)
material_instances_simple   1 ─ N production_stock     (instance_simple_id)
material_instances_composite 1 ─ N production_stock    (instance_composite_id)
supply_purchase_complex     1 ─ N production_supply    (purchase_complex_id)
pdm_snapshot                1 ─ N production_task      (snapshot_original_id)
pdm_snapshot                1 ─ N production_task      (snapshot_current_id)
```
