# Subsystem::PURCHASES (supply)

Procurement subsystem. The code is located in `src/uniter/contract/supply/`.
Scheme (structural): `SUPPLY.pdf`.

## Tables

### `supply_purchase_complex` - Complex purchasing application

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | `actual, created_at, updated_at, created_by, updated_by` |
| `name` | TEXT | |
| `description` | TEXT | |
| `status` | INTEGER | `PurchStatus` (DRAFT/PLACED/CANCELLED) |

Communication with simple requests is reverse, via FK `purchase_complex_id`
in `supply_purchase_simple`. There is no separate join table: one simple one
the application belongs to no more than one complex.

**C++ class:** `contract::supply::PurchaseComplex`
(`ResourceType::PURCHASE_GROUP = 40`).

### `supply_purchase_simple` - Simple purchase order

| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PK | |
| *general fields ResourceAbstract* | | |
| `name` | TEXT | |
| `description` | TEXT | |
| `status` | INTEGER | `PurchStatus` |
| `purchase_complex_id` | INTEGER NULL | FK → `supply_purchase_complex.id` (if the application is part of a complex one) |
| `doc_link_id` | INTEGER NULL | FK → `documents_doc_link.id` (invoice/invoice) |
| `instance_simple_id` | INTEGER NULL | FK → `material_instances_simple.id` |
| `instance_composite_id` | INTEGER NULL | FK → `material_instances_composite.id` |
| `plant_id` | INTEGER NULL | FK → `manager_plant.id` (destination site, opt) |

**Invariant:** `instance_simple_id` XOR `instance_composite_id` - filled
exactly one (the request refers to either a simple or a composite Instance).

**C++ class:** `contract::supply::Purchase`
(`ResourceType::PURCHASE = 41`).

## Connections (key)

```
supply_purchase_complex 1 ─ N supply_purchase_simple
supply_purchase_simple N ─ 1 material_instances_simple (via instance_simple_id)
supply_purchase_simple N ─ 1 material_instances_composite (via instance_composite_id)
supply_purchase_simple N ─ 1 documents_doc_link (via doc_link_id)
supply_purchase_simple N ─ 1 manager_plant (via plant_id)
```

## Notes on logic

* When creating a `PurchaseComplex` the `Purchase` themselves are created separately
CREATE messages. `PurchaseComplex` does not contain `std::vector<uint64_t>`
with member ids - the connection is materialized by a reverse SELECT on `purchase_complex_id`.
* Reverse denormalization of `PurchaseComplex.purchases[]` (which used to be
was in the code) removed: the source of truth is FK in `supply_purchase_simple`.
* One `Purchase` cannot be part of two `PurchaseComplex` at the same time
(restriction at FK level: `purchase_complex_id` - single field).
