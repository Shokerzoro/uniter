# Subsystem::PURCHASES (supply)

Подсистема закупок. В коде лежит в `src/uniter/contract/supply/`.
Схема (структурная): `SUPPLY.pdf`.

## Таблицы

### `supply_purchase_complex` — Комплексная закупочная заявка

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | `actual, created_at, updated_at, created_by, updated_by` |
| `name` | TEXT | |
| `description` | TEXT | |
| `status` | INTEGER | `PurchStatus` (DRAFT/PLACED/CANCELLED) |

Связь с простыми заявками — обратная, через FK `purchase_complex_id`
в `supply_purchase_simple`. Отдельной join-таблицы нет: одна простая
заявка принадлежит не более чем одной комплексной.

**C++ класс:** `contract::supply::PurchaseComplex`
(`ResourceType::PURCHASE_GROUP = 40`).

### `supply_purchase_simple` — Простая закупочная заявка

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `name` | TEXT | |
| `description` | TEXT | |
| `status` | INTEGER | `PurchStatus` |
| `purchase_complex_id` | INTEGER NULL | FK → `supply_purchase_complex.id` (если заявка входит в комплексную) |
| `doc_link_id` | INTEGER NULL | FK → `documents_doc_link.id` (счёт / накладная) |
| `instance_simple_id` | INTEGER NULL | FK → `material_instances_simple.id` |
| `instance_composite_id` | INTEGER NULL | FK → `material_instances_composite.id` |
| `plant_id` | INTEGER NULL | FK → `manager_plant.id` (площадка-получатель, опц.) |

**Инвариант:** `instance_simple_id` XOR `instance_composite_id` — заполнено
ровно одно (заявка ссылается либо на простой, либо на составной Instance).

**C++ класс:** `contract::supply::Purchase`
(`ResourceType::PURCHASE = 41`).

## Связи (ключевые)

```
supply_purchase_complex 1 ─ N supply_purchase_simple
supply_purchase_simple  N ─ 1 material_instances_simple   (через instance_simple_id)
supply_purchase_simple  N ─ 1 material_instances_composite (через instance_composite_id)
supply_purchase_simple  N ─ 1 documents_doc_link           (через doc_link_id)
supply_purchase_simple  N ─ 1 manager_plant                (через plant_id)
```

## Заметки по логике

* При создании `PurchaseComplex` сами `Purchase` создаются отдельными
  CREATE-сообщениями. `PurchaseComplex` не содержит `std::vector<uint64_t>`
  с id членов — связь материализуется обратным SELECT по `purchase_complex_id`.
* Обратная денормализация `PurchaseComplex.purchases[]` (которая раньше
  была в коде) убрана: источник истины — FK в `supply_purchase_simple`.
* Одна `Purchase` не может входить в две `PurchaseComplex` одновременно
  (ограничение на уровне FK: `purchase_complex_id` — одиночное поле).
