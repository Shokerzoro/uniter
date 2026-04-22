# Subsystem::DESIGN

Конструкторская подсистема. Источник истины для **текущего** состояния
изделий компании. Проекты, сборки, детали, их конфигурации. Файлы КД
привязываются через `documents_doc_link`.

Код: `src/uniter/contract/design/`.
Схема: `DESIGN.pdf`. Логика взаимодействия с PDM: `pdm_design_logic.md`.

## Ключевой принцип (новый)

В `design` хранятся **все возможные** проекты, сборки, детали, созданные
в компании. Структура сборок вынесена на уровень конфигураций:

* `design_assembly` содержит **только общие данные** сборки (обозначение,
  имя, описание, тип — всё, что не зависит от исполнения).
* `design_assembly_config` содержит **структуру сборки** конкретного
  исполнения: какие подсборки, детали, стандартные изделия, материалы в
  неё входят.
* `design_part` — деталь (общие данные).
* `design_part_config` — исполнение детали со своим габаритом/массой.

Связи M:N между `assembly_config` и его содержимым (parts, child-assemblies,
standard-products, materials) — через отдельные join-таблицы; в классе C++
они материализуются в `std::vector<...>` в рантайме.

## Ресурсы подсистемы

| Класс | ResourceType | Таблица(ы) |
|---|---|---|
| `Project` | `PROJECT = 30` | `design_project` |
| `Assembly` | `ASSEMBLY = 31` | `design_assembly` |
| `AssemblyConfig` | `ASSEMBLY_CONFIG = 33` | `design_assembly_config` + join-таблицы |
| `Part` | `PART = 32` | `design_part` |
| `PartConfig` | `PART_CONFIG = 34` | `design_part_config` |

## Таблица `design_project` — проект

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `name` | TEXT | |
| `description` | TEXT | |
| `designation` | TEXT | Обозначение проекта по ЕСКД (верхний уровень) |
| `root_assembly_id` | INTEGER NOT NULL | FK → `design_assembly.id` — корневая сборка |
| `pdm_project_id` | INTEGER NULL | FK → `pdm_project.id` — связанный PDM-проект (версии, снэпшоты) |

**Замена:** раньше было `active_snapshot_id` (FK на `pdm_snapshot`).
Теперь проект ссылается на PDM-проект в целом (`pdm_project_id`),
а управление «какая версия head» — внутри `pdm_project`.

NULL `pdm_project_id` означает «у проекта ещё не запущена версионированная
PDM-ветка»; такой проект редактируется свободно, снэпшотов нет.

## Таблица `design_assembly` — сборка (общие данные)

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `project_id` | INTEGER NOT NULL | FK → `design_project.id` |
| `designation` | TEXT | Обозначение по ЕСКД |
| `name` | TEXT | |
| `description` | TEXT | |
| `type` | INTEGER | `AssemblyType` (REAL/VIRTUAL) |

**Здесь НЕТ:** `parent_assembly_id`, `child_assemblies[]`, `parts[]`.
Родительство и содержимое — в `design_assembly_config`.

**Документы.** Файлы (сборочный чертёж, спецификация, 3D) привязываются
через `documents_doc_link` (`target_type=ASSEMBLY`, `target_id=assembly.id`).
В PDF-схеме это показано как `doc_link_id(FK) - mult` — такая «колонка»
фактически означает M:N через обратные запросы: один Assembly имеет
несколько DocLink'ов. В классе C++ — `std::vector<DocLink> linked_documents`
(денормализация для UI).

## Таблица `design_assembly_config` — исполнение сборки (структура)

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `assembly_id` | INTEGER NOT NULL | FK → `design_assembly.id` — к какой сборке относится это исполнение |
| `config_code` | TEXT | Код исполнения («01», «02», ...) |

Это запись «исполнение сборки». Содержимое (M:N) хранится в отдельных
join-таблицах:

### Join `design_assembly_config_parts`
| Колонка | Тип | Описание |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` |
| `part_id` | INTEGER | FK → `design_part.id` |
| `quantity` | INTEGER | Количество вхождений |
| `part_config_code` | TEXT NULL | Выбранное исполнение детали (FK к `design_part_config.config_code` через `part_id`) |

### Join `design_assembly_config_children` (подсборки)
| Колонка | Тип | Описание |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` — родитель |
| `child_assembly_id` | INTEGER | FK → `design_assembly.id` — дочерняя сборка |
| `quantity` | INTEGER | |
| `child_config_code` | TEXT NULL | Выбранное исполнение дочерней сборки |

### Join `design_assembly_config_standard_products`
Стандартные изделия — ссылки на **составные** Instance (например, болт
M8×30 ГОСТ 7798-70 — это composite: «болт + ГОСТ»).
| Колонка | Тип | Описание |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` |
| `instance_composite_id` | INTEGER | FK → `material_instances_composite.id` |
| `quantity` | INTEGER | |

### Join `design_assembly_config_materials`
Материалы (листы, прутки, провод и т.д.) — ссылки на **простые** Instance.
| Колонка | Тип | Описание |
|---|---|---|
| `assembly_config_id` | INTEGER | FK → `design_assembly_config.id` |
| `instance_simple_id` | INTEGER | FK → `material_instances_simple.id` |
| `quantity_items` | INTEGER NULL | для PIECE |
| `quantity_length` | REAL NULL | для LINEAR |
| `quantity_area` | REAL NULL | для AREA |

> Почему standard_products = composite, а materials = simple. По ЕСКД
> стандартные изделия всегда идентифицируются двухчастным обозначением
> «название / ГОСТ» — это структурный composite. Листовой/прутковый
> материал — одночастное обозначение с сегментами ГОСТа (simple).
> Это соответствует схеме DESIGN.pdf: два разных столбца у `assembly_config`.

В C++ содержимое материализуется в `AssemblyConfig` как:
```cpp
std::vector<AssemblyConfigPartRef>     parts;
std::vector<AssemblyConfigChildRef>    child_assemblies;
std::vector<AssemblyConfigStandardRef> standard_products;
std::vector<AssemblyConfigMaterialRef> materials;
```

## Таблица `design_part` — деталь (общие данные)

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `project_id` | INTEGER NOT NULL | FK → `design_project.id` |
| `designation` | TEXT | Обозначение ЕСКД |
| `name` | TEXT | |
| `description` | TEXT | |
| `litera` | TEXT | Литера КД (О/О1/А/…) |
| `organization` | TEXT | Организация-разработчик |
| `instance_simple_id` | INTEGER NULL | FK → `material_instances_simple.id` — материал детали |
| `instance_composite_id` | INTEGER NULL | FK → `material_instances_composite.id` — составной материал детали |

**Инвариант:** `instance_simple_id` XOR `instance_composite_id` (может быть
и «ни одного» для отвлечённых деталей уровня спецификации, но
одновременно оба — запрещено).

**Здесь НЕТ:** `assembly_id`. Деталь НЕ принадлежит одной сборке —
она может входить во множество сборок/исполнений через join-таблицы
`design_assembly_config_parts`.

Документы — через `documents_doc_link` (`target_type=PART`).
Подписи чертежа (`PartSignature`) — отдельная таблица
`design_part_signatures` (см. ниже). В структурной схеме не указана,
но остаётся функциональной необходимостью.

## Таблица `design_part_config` — исполнение детали

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `part_id` | INTEGER NOT NULL | FK → `design_part.id` |
| `config_code` | TEXT | «01», «02», … |
| `length_mm` | REAL | |
| `width_mm` | REAL | |
| `height_mm` | REAL | |
| `mass_kg` | REAL | |

В C++: ресурс `design::PartConfig` — полноценный ResourceAbstract-наследник
(ResourceType::PART_CONFIG). Раньше это была вложенная struct — теперь
отдельный ресурс, CRUD идёт через UniterMessage.

## Таблица `design_part_signatures` (прикладная, не в структурной схеме)

| Колонка | Тип | Описание |
|---|---|---|
| `part_id` | INTEGER | FK → `design_part.id` |
| `role` | TEXT | Разработал / Проверил / … |
| `name` | TEXT | ФИО |
| `date` | TEXT | ISO 8601 |

В C++ — `std::vector<PartSignature> signatures` в `Part`. TODO(на подумать):
вынести в отдельный ресурс `PartSignature` (ResourceType) если понадобится
CRUD по отдельной подписи.

## Связи (обзор)

```
design_project              1 ─ 1 design_assembly          (root_assembly_id)
design_project              1 ─ 0..1 pdm_project           (pdm_project_id)
design_assembly             1 ─ N design_assembly_config   (assembly_id)
design_assembly_config      N ─ M design_part              (join: ..._parts)
design_assembly_config      N ─ M design_assembly          (join: ..._children)
design_assembly_config      N ─ M material_instances_composite (join: ..._standard_products)
design_assembly_config      N ─ M material_instances_simple    (join: ..._materials)
design_part                 1 ─ N design_part_config       (part_id)
design_part                 N ─ 1 material_instances_simple    (instance_simple_id)
design_part                 N ─ 1 material_instances_composite (instance_composite_id)
```
