# Логика взаимодействия DESIGN ↔ PDM

Документ описывает, как взаимодействуют подсистемы DESIGN (источник истины
для текущего состояния конструктора) и PDM (версионирование через
неизменяемые снэпшоты и дельты). Сопроводительные документы:
`design.md`, `pdm.md`.

## 1. Ключевая идея

* **DESIGN** хранит **текущее** (актуальное, редактируемое) состояние
  проектов, сборок, деталей и их исполнений. Таблицы:
  `design_project`, `design_assembly`, `design_assembly_config` (+ join),
  `design_part`, `design_part_config`.
* **PDM** хранит **замороженные копии** данных DESIGN на момент парсинга
  (snapshot) и **инкрементальные дельты** между снэпшотами. Таблицы:
  `pdm_project`, `pdm_snapshot`, `pdm_delta`, а также **зеркальные таблицы**
  `pdm_assembly`, `pdm_assembly_config`, `pdm_part`, `pdm_part_config`
  (+ pdm-варианты join-таблиц).
* Переход DESIGN → PDM реализуется **копированием**: при создании snapshot
  все актуальные записи DESIGN, относящиеся к проекту, копируются в
  соответствующие pdm-таблицы с проставлением FK `snapshot_id`.

## 2. Зачем два набора таблиц (design_* и pdm_*)

Если бы snapshot ссылался напрямую на `design_assembly.id`, любое
редактирование текущей сборки испортило бы содержимое снэпшота
(нарушение инвариантности). Чтобы этого избежать:

1. PDMManager создаёт новые строки в `pdm_assembly` / `pdm_part` /
   `pdm_assembly_config` / `pdm_part_config`, копируя все данные из
   соответствующих `design_*` записей.
2. `pdm_snapshot.root_assembly_id` указывает на id из `pdm_assembly`,
   а не из `design_assembly`.
3. После этого `design_*` можно свободно редактировать — снэпшот не
   потеряет целостность.

Классы в C++ переиспользуются (`uniter::contract::design::Assembly`,
`AssemblyConfig`, `Part`, `PartConfig`). Различие между «DESIGN-записью»
и «PDM-копией» маркируется:

* `ResourceType` в UniterMessage: `ASSEMBLY` vs `ASSEMBLY_PDM`,
  `ASSEMBLY_CONFIG` vs `ASSEMBLY_CONFIG_PDM`, и т.д.;
* таблицей-приёмником в DataManager: `design_assembly` vs `pdm_assembly`.

Сами поля класса одинаковы; PDM-копия дополнительно хранит FK
`snapshot_id` — это поле живёт в таблице БД, в классе C++ передаётся
через отдельный контекст DataManager или, при необходимости, через
`std::optional<uint64_t> snapshot_id` (TODO: решить при реализации
DataManager).

## 3. Механизм создания snapshot

Вызывается PDMManager при парсинге PDF-каталога проекта.

Входные данные: `design_project.id` (идентификатор проекта, для которого
создаётся snapshot). Рассматриваются два случая.

### 3.1. Первый snapshot проекта

```
design_project.pdm_project_id == NULL  (pdm-ветка ещё не запущена)
```

Шаги:

1. PDMManager создаёт новый `pdm_project` с `base_snapshot_id = NULL`,
   `head_snapshot_id = NULL`.
2. Создаёт новый `pdm_snapshot`:
   - `prev_snapshot_id = NULL`, `next_snapshot_id = NULL`,
     `prev_delta_id = NULL`, `next_delta_id = NULL`;
   - `version = 1`, `status = DRAFT`.
3. **Копирует** в pdm-таблицы все актуальные сущности DESIGN, относящиеся
   к `design_project.id`:
   - `design_assembly` → `pdm_assembly` (со snapshot_id = новый snapshot);
   - `design_assembly_config` → `pdm_assembly_config`;
   - `design_part` → `pdm_part`;
   - `design_part_config` → `pdm_part_config`;
   - `design_assembly_config_*` (четыре join-таблицы) → `pdm_assembly_config_*`.
   При копировании FK внутри pdm-строк переписываются на id новых
   pdm-строк (id design_* больше не валиден).
4. Находит скопированную корневую сборку в `pdm_assembly` (по соответствию
   `design_project.root_assembly_id`) и прописывает её id в
   `pdm_snapshot.root_assembly_id`.
5. Обновляет `pdm_project`:
   `base_snapshot_id = head_snapshot_id = новый snapshot.id`.
6. Обновляет `design_project.pdm_project_id = новый pdm_project.id`.

Дельта на этом шаге **не создаётся** (первый снэпшот).

### 3.2. Последующие снэпшоты

```
design_project.pdm_project_id != NULL
pdm_project.head_snapshot_id  != NULL  (есть предыдущий head)
```

Шаги:

1. Создаётся новый `pdm_snapshot` (как в 3.1.2, но `version = prev.version + 1`,
   `prev_snapshot_id = pdm_project.head_snapshot_id`).
2. Копируются актуальные DESIGN-сущности проекта в pdm-таблицы (как в 3.1.3).
3. `pdm_snapshot.root_assembly_id` устанавливается на id скопированной
   корневой сборки в `pdm_assembly`.
4. PDMManager формирует `pdm_delta` между предыдущим head и новым snapshot:
   - `prev_snapshot_id = prev_head.id`,
     `next_snapshot_id = new_snapshot.id`;
   - `prev_delta_id` — id предыдущей дельты (у prev_head был `next_delta_id = NULL`
     до этого шага; после этого шага в ней прописывается новая дельта,
     а в новой дельте — её prev_delta_id указывает на предыдущую дельту
     в цепочке, если она существует);
   - `next_delta_id = NULL` (пока не создана следующая дельта).
5. Обновляются перекрёстные FK:
   - `prev_head.next_snapshot_id = new_snapshot.id`,
     `prev_head.next_delta_id = new_delta.id`;
   - `new_snapshot.prev_delta_id = new_delta.id`.
6. `pdm_project.head_snapshot_id = new_snapshot.id`
   (`base_snapshot_id` остаётся тем же).

## 4. Итерация по истории версий

Цепочка снэпшотов и дельт образует **двусвязный список** внутри одного
`pdm_project`:

```
 (base) snapshot_1  ←── delta_1 ──→  snapshot_2  ←── delta_2 ──→  snapshot_3  (head)
```

Каждый элемент цепочки знает и соседей:

| Ресурс       | Поля навигации                                               |
|--------------|---------------------------------------------------------------|
| `pdm_snapshot` | `prev_snapshot_id`, `next_snapshot_id`, `prev_delta_id`, `next_delta_id` |
| `pdm_delta`    | `prev_snapshot_id`, `next_snapshot_id`, `prev_delta_id`, `next_delta_id` |

Поля `prev_delta_id / next_delta_id` у снэпшота **дублируют** двустороннюю
связь со стороны `pdm_delta` (нормализация пожертвована ради скорости
итерации в обе стороны без JOIN'ов).

Итерация вперёд: `snap → snap.next_delta_id → delta.next_snapshot_id → ...`
Итерация назад: `snap → snap.prev_delta_id → delta.prev_snapshot_id → ...`

## 5. Связь design_project ↔ pdm_project

```
design_project.pdm_project_id  (nullable FK → pdm_project.id)
```

* `NULL` = у проекта **не запущена** версионированная PDM-ветка
  (snapshot'ов нет; проект редактируется свободно).
* не-NULL = PDM-ветка запущена, есть хотя бы один snapshot.

Поле `design_project.active_snapshot_id` из предыдущей версии схемы
**удалено**. Вопрос «какой снэпшот актуален» решается внутри
`pdm_project.head_snapshot_id` и не дублируется в DESIGN.

Другим подсистемам (PRODUCTION, ERPManager), нуждающимся в «утверждённом
составе», следует спрашивать:
```
design_project.pdm_project_id → pdm_project.head_snapshot_id → pdm_snapshot
```

Статус `DRAFT/APPROVED/ARCHIVED` у snapshot — это TODO: оставлен в классе
как прикладной атрибут, но структурной схемой не требуется. При реализации
рабочего цикла утверждения можно будет:

* либо хранить status внутри pdm_snapshot (как сейчас в классе);
* либо ввести отдельный `pdm_project.approved_snapshot_id` рядом с
  `head_snapshot_id` (head = последний созданный, approved = последний
  утверждённый).

Решение откладывается до работ над PDMManager.

## 6. CRUD на UniterMessage — что куда пишет DataManager

| ResourceType            | Таблица                       | Записывается при          |
|-------------------------|-------------------------------|---------------------------|
| `PROJECT = 30`          | `design_project`              | CUD пользователя          |
| `ASSEMBLY = 31`         | `design_assembly`             | CUD пользователя          |
| `ASSEMBLY_CONFIG = 33`  | `design_assembly_config` + join | CUD пользователя        |
| `PART = 32`             | `design_part`                 | CUD пользователя          |
| `PART_CONFIG = 34`      | `design_part_config`          | CUD пользователя          |
| `PDM_PROJECT = 52`      | `pdm_project`                 | CUD PDMManager            |
| `SNAPSHOT = 50`         | `pdm_snapshot`                | C PDMManager (immutable)  |
| `DELTA = 51`            | `pdm_delta`                   | C PDMManager (immutable)  |
| `ASSEMBLY_PDM = 35`     | `pdm_assembly`                | C PDMManager при snapshot |
| `ASSEMBLY_CONFIG_PDM=36`| `pdm_assembly_config` + join  | C PDMManager при snapshot |
| `PART_PDM = 37`         | `pdm_part`                    | C PDMManager при snapshot |
| `PART_CONFIG_PDM = 38`  | `pdm_part_config`             | C PDMManager при snapshot |

DataManager определяет таблицу по паре `(Subsystem, ResourceType)` в
UniterMessage: `Subsystem::DESIGN + ASSEMBLY` → `design_assembly`,
`Subsystem::PDM + ASSEMBLY_PDM` → `pdm_assembly` и т.д.

Записи в зеркальные pdm-таблицы (`ASSEMBLY_PDM`, ...) обычно делаются
ТОЛЬКО PDMManager'ом в рамках транзакции создания snapshot. После
создания snapshot эти строки больше не редактируются (инвариантность).
