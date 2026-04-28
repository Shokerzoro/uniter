# Subsystem::MATERIALS + Subsystem::INSTANCES - note on the database

Reference diagram: `MATERIAL_INSTANCE.pdf` (conceptual diagram, shows
PK/FK and main columns only).

## Classes in runtime

- `materials::TemplateBase` — base class of the material template. Contains
general fields (name/description/dimension_type/source/version) and
a collapsed "folder" of documents `documents::DocLink doc_link`.
- `materials::TemplateSimple` - a simple template (GOST / OST / TU ...).
Contains collapsed `prefix_segments`, `suffix_segments`
(from tables `material/segment` + `material/segment_value`),
as well as a collapsed list of id ids of compatible simple templates
`compatible_template_ids` (from the `material/template_compatibility` table).
- `materials::TemplateComposite` - composite template (fraction of two
simple). Stores `top_template_id` + `bottom_template_id` as FK on
`material/template_simple.id` (these FKs remain in runtime - without them
Composite does not know which primes it is composed of).
- `materials::SegmentDefinition` — one segment record, after
collapsing: segment's own fields + `allowed_values` from
`segment_value`. FK on `template_simple` / on `segment` - only in the database.
- `InstanceBase` / `InstanceSimple` / `InstanceComposite` - instances
links to material. `template_id` is the FK for the template, and it remains
in runtime: without it, Instance does not know what exactly it specializes.

> PDF models only the conceptual diagram. Other data
> (name, description, dimension_type, source, version, year, PrefName,
> Quantity, etc.) are stored in classes and in corresponding tables,
> even if not shown in PDF.

## Tables in the database

### `material/template_simple` — `ResourceType::MATERIAL_TEMPLATE_SIMPLE` (20)

| column | type | appointment |
|---------------------------|---------|-----------------------------------------------|
| `id`                      | PK      |                                               |
| `doc_link_id` | FK | → `documents/doc_link.id` (collapses into `doc_link`) |
| `dimension_type`          | INT     | `materials::DimensionType`                    |
| `source`                  | INT     | `GostSource`                                  |
| `standart_type`           | INT     | `StandartType` (STANDALONE/ASSORTMENT/MATERIAL/NONE) |
| `standard_type`           | INT     | `GostStandardType` (GOST/OST/…)               |
| `standard_number`         | TEXT    |                                               |
| `year`                    | TEXT    |                                               |
| `name`                    | TEXT    |                                               |
| `description`             | TEXT    |                                               |
| `version`                 | INT     |                                               |
| + fields `ResourceAbstract` | ... |                                               |

### `material/template_composite` — `ResourceType::MATERIAL_TEMPLATE_COMPOSITE` (21)

| column | type | appointment |
|---------------------------|---------|-----------------------------------------------|
| `id`                      | PK      |                                               |
| `top_template_id`         | FK      | → `material/template_simple.id` (ASSORTMENT)  |
| `bottom_template_id`      | FK      | → `material/template_simple.id` (MATERIAL)    |
| `doc_link_id`             | FK      | → `documents/doc_link.id`                     |
| `dimension_type`          | INT     | `materials::DimensionType`                    |
| `source`                  | INT     | `GostSource`                                  |
| `name`                    | TEXT    |                                               |
| `description`             | TEXT    |                                               |
| `version`                 | INT     |                                               |
| `pref_name`               | TEXT    | `PrefName`                                    |
| + fields `ResourceAbstract` | ... |                                               |

### `material/segment` — `ResourceType::SEGMENT`

| column | type | appointment |
|-----------------|-------|-----------------------------------------------------------|
| `id`            | PK    |                                                           |
| `template_id`   | FK    | → `material/template_simple.id`                           |
| `type`          | INT   | `SegmentType` (PREFIX / SUFFIX)                           |
| `order` | INT | serial number; UNIQUE (template_id, type, order) |
| `code` | TEXT | machine name |
| `name` | TEXT | human readable name |
| `description`   | TEXT  |                                                           |
| `value_type`    | INT   | `SegmentValueType`                                        |
| `is_active`     | BOOL  |                                                           |
| + fields `ResourceAbstract` | ... |                                                       |

### `material/segment_value` — `ResourceType::SEGMENT_VALUE`

| column | type | appointment |
|----------------|-------|------------------------------------------------------------|
| `id`           | PK    |                                                            |
| `segment_id`   | FK    | → `material/segment.id`                                    |
| `code` | TEXT | value key (for map::key) |
| `value` | TEXT | string representation of value |
| + fields `ResourceAbstract` | ... |                                                        |

### `material/template_compatibility` — `ResourceType::TEMPLATE_COMPATIBILITY`

Link table M:N between `template_simple` ↔ `template_simple`
(pairs ASSORTMENT ↔ MATERIAL).

| column | type | appointment |
|--------------------------|-----|------------------------------------------|
| `id`                     | PK  |                                          |
| `template_simple_id_a`   | FK  | → `material/template_simple.id`          |
| `template_simple_id_b`   | FK  | → `material/template_simple.id`          |
| + fields `ResourceAbstract` | ... |                                          |

### `instance/instance_simple` — `ResourceType::MATERIAL_INSTANCE` (60)

| column | type | appointment |
|----------------------|-------|------------------------------------------------|
| `id`                 | PK    |                                                |
| `template_simple_id` | FK | → `material/template_simple.id` (stored as `template_id` at runtime) |
| `name`               | TEXT  |                                                |
| `description`        | TEXT  |                                                |
| `dimension_type`     | INT   | `materials::DimensionType`                     |
| + Quantity fields (items / length / fig.area) | ... (by dimension_type) |
| + fields `ResourceAbstract` | ... |                                               |

Segment values ​​(`prefix_values` / `suffix_values`) are stored separately
subtable `instance_simple_segment_value` (one ResourceType on the client
level is not needed - it can be edited atomically with InstanceSimple).

### `instance/instance_composite` — `ResourceType::MATERIAL_INSTANCE`

| column | type | appointment |
|--------------------------|-------|---------------------------------------------------|
| `id`                     | PK    |                                                   |
| `template_composite_id` | FK | → `material/template_composite.id` (stored as `template_id` at runtime) |
| `name`                   | TEXT  |                                                   |
| `description`            | TEXT  |                                                   |
| `dimension_type`         | INT   | `materials::DimensionType`                        |
| + Quantity fields | ... |                                                    |
| + fields `ResourceAbstract` | ... |                                                    |

`top_values` / `bottom_values` - similarly, a subtable of values.

## Collapse tables → classes

**Reading `TemplateSimple`:**

```sql
-- the template itself
SELECT * FROM material/template_simple WHERE id = :id;
-- segments + values
SELECT s.*, v.id AS sv_id, v.code AS sv_code, v.value AS sv_value
  FROM material/segment s
  LEFT JOIN material/segment_value v ON v.segment_id = s.id
  WHERE s.template_id = :id AND s.is_active = 1
  ORDER BY s.type, s."order";
-- compatibility
SELECT template_simple_id_b AS other
  FROM material/template_compatibility WHERE template_simple_id_a = :id
UNION
SELECT template_simple_id_a AS other
  FROM material/template_compatibility WHERE template_simple_id_b = :id;
-- documents folder
SELECT * FROM documents/doc_link WHERE id = :doc_link_id;
SELECT * FROM documents/doc      WHERE doc_link_id = :doc_link_id AND is_actual = 1;
```

Convolution:
- segments with `type = PREFIX` → `TemplateSimple::prefix_segments` (+ `prefix_order`);
- segments with `type = SUFFIX` → `TemplateSimple::suffix_segments` (+ `suffix_order`);
- `segment_value.value` by `segment_value.id` → `SegmentDefinition::allowed_values`;
- compatibility selection → `TemplateSimple::compatible_template_ids`;
- `doc_link` + `docs[]` → `TemplateBase::doc_link`.

FK (`segment.template_id`, `segment_value.segment_id`,
`template_compatibility.template_simple_id_*`,
`template_simple.doc_link_id`) to runtime classes **not transferred** -
they are used for sampling purposes only.

**Reading `TemplateComposite`:** separate SELECT + loading
`doc_link`. `top_template_id` / `bottom_template_id` **remain** in
runtime like FK for simple templates.

**Reading `InstanceSimple`/`InstanceComposite`:** SELECT from
corresponding table + loading segment values. `template_id`
remains in runtime.

## Layout class → tables

CRUD is atomically handled differently depending on the level:

- the template itself (`TemplateSimple` / `TemplateComposite`)
  → `ResourceType::MATERIAL_TEMPLATE_SIMPLE` / `..._COMPOSITE`;
- separate segment → `ResourceType::SEGMENT`;
- individual segment value → `ResourceType::SEGMENT_VALUE`;
- compatibility pair → `ResourceType::TEMPLATE_COMPATIBILITY`;
- documents in the folder → `ResourceType::DOC` (see DOCUMENTS).

This allows you to change the template structure in parts without mailing
whole TemplateSimple via Kafka.

Instance resources (`InstanceSimple`/`InstanceComposite`) are edited
atomic: their segment values ​​are passed within the Instance itself.

## Communication with uniterprotocol.h

Existing:
```
ResourceType::MATERIAL_TEMPLATE_SIMPLE    = 20
ResourceType::MATERIAL_TEMPLATE_COMPOSITE = 21
ResourceType::MATERIAL_INSTANCE           = 60
ResourceType::DOC                         = 90
ResourceType::DOC_LINK                    = 91
```

You need to add (see task on uniterprotocol.h):
```
ResourceType::SEGMENT — material/segment
ResourceType::SEGMENT_VALUE — material/segment_value
ResourceType::TEMPLATE_COMPATIBILITY - material/template_compatibility
```

Subsystem in `Subsystem` (already exists): `MATERIALS`, `INSTANCES`.
