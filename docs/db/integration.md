# Subsystem::GENERATIVE + GenSubsystem::INTEGRATION

Задача интеграции — выгрузить конкретный ресурс внешнему партнёру.
Генеративная подсистема: у каждой интеграции (Integration из Subsystem::MANAGER)
свой экземпляр подсистемы со своими IntegrationTask.

Код: `src/uniter/contract/integration/`.
Схема: `INTEGRATION.pdf`.

## Таблица `integration_task`

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `integration_id` | INTEGER NOT NULL | FK → `manager_integration.id` — порождающая Integration |
| `target_subsystem` | INTEGER | `contract::Subsystem` («куда смотрит» задача) |
| `target_resource_type` | INTEGER | `contract::ResourceType` (project, template, purchase, snapshot...) |
| `any_resource_id` | INTEGER NOT NULL | FK → id ресурса из таблицы, определяемой парой (subsystem, resource_type) |

**C++ класс:** `contract::integration::IntegrationTask`
(`ResourceType::INTEGRATION_TASK = 80`).

## Полиморфный FK `any_resource_id`

`any_resource_id` — это внешний ключ на таблицу, которая определяется
парой `(target_subsystem, target_resource_type)`. Формально в БД не объявляется
`FOREIGN KEY`-констреинт (целевая таблица неизвестна до рантайма); ссылочную
целостность проверяет DataManager при CREATE/UPDATE задачи.

Примеры:
| target_subsystem | target_resource_type | целевая таблица |
|---|---|---|
| DESIGN | PROJECT | `design_project` |
| MATERIALS | MATERIAL_TEMPLATE_SIMPLE | `material_templates_simple` |
| PURCHASES | PURCHASE_GROUP | `supply_purchase_complex` |
| PDM | SNAPSHOT | `pdm_snapshot` |

## Что удалено из старой версии класса

Согласно требованию «удалить из кода проекта лишнее, привести в соответствие
со схемой БД», из `IntegrationTask` убраны поля, которых нет в схеме:
`status`, `payload_ref`, `planned_at`, `transmitted_at`, `last_error`.
Перечисление `IntegrationTaskStatus` упразднено (файл `integrationtypes.h`
удалён — в нём больше нет содержимого).

TODO(на подумать): если позже понадобится отслеживание статуса отправки,
это отдельный ресурс `IntegrationTaskAttempt` с FK `integration_task_id`
(история попыток) — но это не входит в текущую модель.

## Связи

```
manager_integration 1 ─ N integration_task  (через integration_id)
integration_task    N ─ 1 <произвольная_таблица> (через (target_subsystem, target_resource_type, any_resource_id))
```
