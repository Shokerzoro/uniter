# Subsystem::GENERATIVE + GenSubsystem::PRODUCTION

Генеративная подсистема «под каждую площадку». `Plant` из
`Subsystem::MANAGER` порождает свою подсистему PRODUCTION с
`GenSubsystemId == plant.id`.

Код: `src/uniter/contract/production/`.
Схема: `PRODUCTION.pdf`.

## Ресурсы подсистемы

| Класс | ResourceType | Таблица |
|---|---|---|
| `ProductionStock` | `PRODUCTION_STOCK = 71` | `production_stock` |
| `ProductionSupply` | `PRODUCTION_SUPPLY = 72` | `production_supply` |
| `ProductionTask` | `PRODUCTION_TASK = 70` | `production_task` |

## Таблица `production_stock` — складская позиция

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `plant_id` | INTEGER NOT NULL | FK → `manager_plant.id` — площадка-владелец |
| `instance_simple_id` | INTEGER NULL | FK → `material_instances_simple.id` |
| `instance_composite_id` | INTEGER NULL | FK → `material_instances_composite.id` |
| `quantity_items` | INTEGER | штуки (DimensionType::PIECE) |
| `quantity_length` | REAL | метры (DimensionType::LINEAR) |
| `quantity_area` | REAL | м² (DimensionType::AREA) |

**Инвариант:** `instance_simple_id` XOR `instance_composite_id`
(Stock — это позиция на конкретный Instance; общий «материал в целом» на
складе не отслеживается).

Какое количественное поле активно — определяется `dimension_type`
соответствующего MaterialInstance/Template.

## Таблица `production_supply` — поступление (приёмка)

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `plant_id` | INTEGER NOT NULL | FK → `manager_plant.id` |
| `purchase_complex_id` | INTEGER NOT NULL | FK → `supply_purchase_complex.id` |

Связывает комплексную закупочную заявку с площадкой-получателем.
Концептуально: «на площадку X пришла поставка по заявке Y».

TODO(на подумать): сюда, вероятно, понадобятся поля `received_at`,
`quantity_received` и т.д. — но в текущей схеме они не указаны,
добавляются при расширении.

## Таблица `production_task` — производственное задание

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `plant_id` | INTEGER NOT NULL | FK → `manager_plant.id` |
| `snapshot_original_id` | INTEGER NOT NULL | FK → `pdm_snapshot.id` — снэпшот, под который задание было создано |
| `snapshot_current_id` | INTEGER NOT NULL | FK → `pdm_snapshot.id` — актуальный снэпшот, по которому идёт производство |

`snapshot_original_id` и `snapshot_current_id` совпадают в момент создания
задания. Они расходятся, когда поверх задания «накатывается» новый APPROVED
Snapshot того же проекта: фиксируется, от какой КД задача стартовала (original)
и по какой сейчас выполняется (current). Это обеспечивает воспроизводимость
истории и одновременно переход на свежую КД без «потери корней».

TODO(на подумать): поля `code`, `name`, `quantity`, `status`,
`planned_start`/`planned_end`, `actual_start`/`actual_end` — они есть в
старой модели и нужны для ERP. В PRODUCTION.pdf (структурной) их нет,
так как схема показывает только связи между таблицами. Оставляем в классе
со статусом «прикладные атрибуты задания», не нарушающие ключевую структуру.

## Что удалено из старой версии

Древовидные структуры `ProductionAssemblyNode` и `ProductionPartNode`
упразднены. Они денормализовывали дерево КД внутри одной записи и
дублировали связи из `design_assembly` / `design_part`. При необходимости
отслеживать прогресс по узлам — это отдельные ресурсы
`ProductionTaskAssemblyProgress` и `ProductionTaskPartProgress` с FK
`production_task_id` (это вне текущей модели, но зафиксировано как TODO).

Файл `productiontypes.h` сохранён — в нём остался только
`TaskStatus`-enum.

## Связи

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
