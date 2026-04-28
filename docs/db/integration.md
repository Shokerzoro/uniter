# Subsystem::GENERATIVE + GenSubsystem::INTEGRATION

The task of integration is to upload a specific resource to an external partner.
Generative subsystem: for each integration (Integration from Subsystem::MANAGER)
your own instance of the subsystem with its own IntegrationTask.

Code: `src/uniter/contract/integration/`.
Schematic: `INTEGRATION.pdf`.

## Table `integration_task`

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `integration_id` | INTEGER NOT NULL | FK → `manager_integration.id` - parent Integration |
| `target_subsystem` | INTEGER | `contract::Subsystem` (“where the task is looking”) |
| `target_resource_type` | INTEGER | `contract::ResourceType` (project, template, purchase, snapshot...) |
| `any_resource_id` | INTEGER NOT NULL | FK → resource id from the table defined by the pair (subsystem, resource_type) |

**C++ class:** `contract::integration::IntegrationTask`
(`ResourceType::INTEGRATION_TASK = 80`).

## Polymorphic FK `any_resource_id`

`any_resource_id` is a foreign key to the table that is defined
pair `(target_subsystem, target_resource_type)`. Not formally declared in the database
`FOREIGN KEY`-construct (the target table is unknown until runtime); reference
integrity is checked by the DataManager during CREATE/UPDATE tasks.

Examples:
| target_subsystem | target_resource_type | target table |
|---|---|---|
| DESIGN | PROJECT | `design_project` |
| MATERIALS | MATERIAL_TEMPLATE_SIMPLE | `material_templates_simple` |
| PURCHASES | PURCHASE_GROUP | `supply_purchase_complex` |
| PDM | SNAPSHOT | `pdm_snapshot` |

## What was removed from the old version of the class

According to the requirement “remove unnecessary things from the project code, bring them into compliance
with the database schema", fields that are not in the schema have been removed from `IntegrationTask`:
`status`, `payload_ref`, `planned_at`, `transmitted_at`, `last_error`.
The `IntegrationTaskStatus` enumeration has been removed (file `integrationtypes.h`
deleted - it no longer contains content).

TODO (to think about): if you later need to track the status of the shipment,
this is a separate resource `IntegrationTaskAttempt` with FK `integration_task_id`
(history of attempts) - but this is not included in the current model.

## Connections

```
manager_integration 1 ─ N integration_task (via integration_id)
integration_task    N ─ 1 <arbitrary_table> (via (target_subsystem, target_resource_type, any_resource_id))
```
