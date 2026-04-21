# Subsystem::MATERIALS + Subsystem::INSTANCES — заметка по БД

Схема-эталон: `MATERIAL_INSTANCE.pdf` (концептуальная схема, показывает
только PK/FK и основные столбцы).

## Классы в рантайме

- `materials::TemplateBase` — базовый класс шаблона материала. Содержит
  общие поля (name/description/dimension_type/source/version) и
  свёрнутую «папку» документов `documents::DocLink doc_link`.
- `materials::TemplateSimple` — простой шаблон (ГОСТ / ОСТ / ТУ …).
  Содержит свёрнутые `prefix_segments`, `suffix_segments`
  (из таблиц `material/segment` + `material/segment_value`),
  а также свёрнутый список id совместимых простых шаблонов
  `compatible_template_ids` (из таблицы `material/template_compatibility`).
- `materials::TemplateComposite` — составной шаблон (дробь из двух
  простых). Хранит `top_template_id` + `bottom_template_id` как FK на
  `material/template_simple.id` (эти FK остаются в рантайме — без них
  Composite не знает, из каких простых он составлен).
- `materials::SegmentDefinition` — одна запись сегмента, после
  сворачивания: собственные поля сегмента + `allowed_values` из
  `segment_value`. FK на `template_simple` / на `segment` — только в БД.
- `InstanceBase` / `InstanceSimple` / `InstanceComposite` — экземпляры
  ссылок на материал. `template_id` — это FK на шаблон, и он остаётся
  в рантайме: без него Instance не знает, что именно он специализирует.

> PDF моделирует только концептуальную схему. Прочие данные
> (name, description, dimension_type, source, version, year, PrefName,
> Quantity и т.п.) хранятся в классах и в соответствующих таблицах,
> даже если не показаны в PDF.

## Таблицы в БД

### `material/template_simple` — `ResourceType::MATERIAL_TEMPLATE_SIMPLE` (20)

| колонка                   | тип     | назначение                                    |
|---------------------------|---------|-----------------------------------------------|
| `id`                      | PK      |                                               |
| `doc_link_id`             | FK      | → `documents/doc_link.id` (свернётся в `doc_link`) |
| `dimension_type`          | INT     | `materials::DimensionType`                    |
| `source`                  | INT     | `GostSource`                                  |
| `standart_type`           | INT     | `StandartType` (STANDALONE/ASSORTMENT/MATERIAL/NONE) |
| `standard_type`           | INT     | `GostStandardType` (GOST/OST/…)               |
| `standard_number`         | TEXT    |                                               |
| `year`                    | TEXT    |                                               |
| `name`                    | TEXT    |                                               |
| `description`             | TEXT    |                                               |
| `version`                 | INT     |                                               |
| + поля `ResourceAbstract` | …       |                                               |

### `material/template_composite` — `ResourceType::MATERIAL_TEMPLATE_COMPOSITE` (21)

| колонка                   | тип     | назначение                                    |
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
| + поля `ResourceAbstract` | …       |                                               |

### `material/segment` — `ResourceType::SEGMENT`

| колонка         | тип   | назначение                                                |
|-----------------|-------|-----------------------------------------------------------|
| `id`            | PK    |                                                           |
| `template_id`   | FK    | → `material/template_simple.id`                           |
| `type`          | INT   | `SegmentType` (PREFIX / SUFFIX)                           |
| `order`         | INT   | порядковый номер; UNIQUE (template_id, type, order)       |
| `code`          | TEXT  | машинное имя                                              |
| `name`          | TEXT  | человекочитаемое имя                                      |
| `description`   | TEXT  |                                                           |
| `value_type`    | INT   | `SegmentValueType`                                        |
| `is_active`     | BOOL  |                                                           |
| + поля `ResourceAbstract` | … |                                                       |

### `material/segment_value` — `ResourceType::SEGMENT_VALUE`

| колонка        | тип   | назначение                                                 |
|----------------|-------|------------------------------------------------------------|
| `id`           | PK    |                                                            |
| `segment_id`   | FK    | → `material/segment.id`                                    |
| `code`         | TEXT  | ключ значения (для map::key)                               |
| `value`        | TEXT  | строковое представление значения                           |
| + поля `ResourceAbstract` | … |                                                        |

### `material/template_compatibility` — `ResourceType::TEMPLATE_COMPATIBILITY`

Связочная таблица M:N между `template_simple` ↔ `template_simple`
(пары ASSORTMENT ↔ MATERIAL).

| колонка                  | тип | назначение                               |
|--------------------------|-----|------------------------------------------|
| `id`                     | PK  |                                          |
| `template_simple_id_a`   | FK  | → `material/template_simple.id`          |
| `template_simple_id_b`   | FK  | → `material/template_simple.id`          |
| + поля `ResourceAbstract` | … |                                          |

### `instance/instance_simple` — `ResourceType::MATERIAL_INSTANCE` (60)

| колонка              | тип   | назначение                                     |
|----------------------|-------|------------------------------------------------|
| `id`                 | PK    |                                                |
| `template_simple_id` | FK    | → `material/template_simple.id` (хранится как `template_id` в рантайме) |
| `name`               | TEXT  |                                                |
| `description`        | TEXT  |                                                |
| `dimension_type`     | INT   | `materials::DimensionType`                     |
| + поля Quantity (items / length / fig.area)    | …  (по dimension_type)            |
| + поля `ResourceAbstract` | … |                                               |

Значения сегментов (`prefix_values` / `suffix_values`) хранятся отдельной
подтаблицей `instance_simple_segment_value` (один ResourceType на клиентском
уровне не понадобится — редактируется атомарно вместе с InstanceSimple).

### `instance/instance_composite` — `ResourceType::MATERIAL_INSTANCE`

| колонка                  | тип   | назначение                                        |
|--------------------------|-------|---------------------------------------------------|
| `id`                     | PK    |                                                   |
| `template_composite_id`  | FK    | → `material/template_composite.id` (хранится как `template_id` в рантайме) |
| `name`                   | TEXT  |                                                   |
| `description`            | TEXT  |                                                   |
| `dimension_type`         | INT   | `materials::DimensionType`                        |
| + поля Quantity           | …   |                                                    |
| + поля `ResourceAbstract` | …   |                                                    |

`top_values` / `bottom_values` — аналогично, подтаблица значений.

## Сворачивание таблиц → классы

**Чтение `TemplateSimple`:**

```sql
-- сам шаблон
SELECT * FROM material/template_simple WHERE id = :id;
-- сегменты + значения
SELECT s.*, v.id AS sv_id, v.code AS sv_code, v.value AS sv_value
  FROM material/segment s
  LEFT JOIN material/segment_value v ON v.segment_id = s.id
  WHERE s.template_id = :id AND s.is_active = 1
  ORDER BY s.type, s."order";
-- совместимости
SELECT template_simple_id_b AS other
  FROM material/template_compatibility WHERE template_simple_id_a = :id
UNION
SELECT template_simple_id_a AS other
  FROM material/template_compatibility WHERE template_simple_id_b = :id;
-- папка документов
SELECT * FROM documents/doc_link WHERE id = :doc_link_id;
SELECT * FROM documents/doc      WHERE doc_link_id = :doc_link_id AND is_actual = 1;
```

Свёртка:
- сегменты с `type = PREFIX` → `TemplateSimple::prefix_segments` (+ `prefix_order`);
- сегменты с `type = SUFFIX` → `TemplateSimple::suffix_segments` (+ `suffix_order`);
- `segment_value.value` по `segment_value.id` → `SegmentDefinition::allowed_values`;
- выборка совместимости → `TemplateSimple::compatible_template_ids`;
- `doc_link` + `docs[]` → `TemplateBase::doc_link`.

FK (`segment.template_id`, `segment_value.segment_id`,
`template_compatibility.template_simple_id_*`,
`template_simple.doc_link_id`) в рантайм-классы **не переносятся** —
они использованы только для выборки.

**Чтение `TemplateComposite`:** отдельный SELECT + подгрузка
`doc_link`. `top_template_id` / `bottom_template_id` **остаются** в
рантайме как FK на простые шаблоны.

**Чтение `InstanceSimple`/`InstanceComposite`:** SELECT из
соответствующей таблицы + загрузка значений сегментов. `template_id`
остаётся в рантайме.

## Раскладывание класс → таблицы

CRUD атомарно ведётся по-разному в зависимости от уровня:

- сам шаблон (`TemplateSimple` / `TemplateComposite`)
  → `ResourceType::MATERIAL_TEMPLATE_SIMPLE` / `..._COMPOSITE`;
- отдельный сегмент → `ResourceType::SEGMENT`;
- отдельное значение сегмента → `ResourceType::SEGMENT_VALUE`;
- пара совместимости → `ResourceType::TEMPLATE_COMPATIBILITY`;
- документы в папке → `ResourceType::DOC` (см. DOCUMENTS).

Это позволяет изменять структуру шаблона по частям без рассылки
целого TemplateSimple через Kafka.

Instance-ресурсы (`InstanceSimple`/`InstanceComposite`) редактируются
атомарно: их значения сегментов передаются внутри самого Instance.

## Связь с uniterprotocol.h

Существующие:
```
ResourceType::MATERIAL_TEMPLATE_SIMPLE    = 20
ResourceType::MATERIAL_TEMPLATE_COMPOSITE = 21
ResourceType::MATERIAL_INSTANCE           = 60
ResourceType::DOC                         = 90
ResourceType::DOC_LINK                    = 91
```

Нужно добавить (см. задачу по uniterprotocol.h):
```
ResourceType::SEGMENT                 — материал/segment
ResourceType::SEGMENT_VALUE           — материал/segment_value
ResourceType::TEMPLATE_COMPATIBILITY  — материал/template_compatibility
```

Подсистема в `Subsystem` (уже есть): `MATERIALS`, `INSTANCES`.
