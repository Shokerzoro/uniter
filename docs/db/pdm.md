# Subsystem::PDM

Подсистема управления версиями (Product Data Management). Хранит
**неизменяемые** слепки конструкторских проектов (снэпшоты) и
инкрементальные изменения между ними (дельты).

Код: `src/uniter/contract/pdm/`.
Схема: `PDM.pdf`. Логика взаимодействия с DESIGN: `pdm_design_logic.md`.

## Ключевой принцип

Подсистема PDM содержит:

1. Собственные ресурсы: `pdm_project`, `pdm_snapshot`, `pdm_delta`.
2. **Зеркальные копии** ресурсов DESIGN — при создании snapshot все
   связанные `design_*` записи копируются в параллельные `pdm_*_snapshot`
   таблицы, «замораживаются» и привязываются к конкретному `snapshot_id`.

Это обеспечивает неизменность снэпшотов: после создания снэпшота любая
правка в DESIGN не меняет то, что было заморожено.

Ресурсы PDM, не являющиеся зеркалами, ведут себя как обычные CRUD-ресурсы.
Зеркала — пишутся PDMManager'ом при создании snapshot и больше не меняются.

## Ресурсы подсистемы (собственные)

| Класс | ResourceType | Таблица |
|---|---|---|
| `PdmProject` | `PDM_PROJECT = 52` | `pdm_project` |
| `Snapshot` | `SNAPSHOT = 50` | `pdm_snapshot` |
| `Delta` | `DELTA = 51` | `pdm_delta` |

## Зеркальные таблицы (копии DESIGN в контексте снэпшота)

| Таблица | Роль | Маппится на ResourceType |
|---|---|---|
| `pdm_assembly` | Копия `design_assembly` на момент snapshot | `ASSEMBLY_PDM = 35` |
| `pdm_assembly_config` | Копия `design_assembly_config` | `ASSEMBLY_CONFIG_PDM = 36` |
| `pdm_part` | Копия `design_part` | `PART_PDM = 37` |
| `pdm_part_config` | Копия `design_part_config` | `PART_CONFIG_PDM = 38` |
| `pdm_assembly_config_parts` | join | — |
| `pdm_assembly_config_children` | join | — |
| `pdm_assembly_config_standard_products` | join | — |
| `pdm_assembly_config_materials` | join | — |

Классы ресурсов **переиспользуются** из подсистемы DESIGN (`contract::design::*`).
Маркер «это PDM-копия» — ResourceType и таблица, в которой она лежит.
На уровне полей pdm-ресурс отличается только добавлением FK `snapshot_id`
на владеющий снэпшот.

Для маппинга на разные таблицы одним классом используется явный параметр
`table_suffix` или `subsystem_context` в DataManager при CRUD (это
соглашение DataManager'а; сами ресурсы одинаковы).

TODO(на подумать): если окажется, что PDM-зеркало должно иметь существенно
другие поля (например, `approved_at`, `approved_by` на уровне
pdm_assembly_config), вводить отдельные классы `pdm::AssemblyConfig`
и т.д. Сейчас отличия сводятся только к FK `snapshot_id`, поэтому
переиспользуем классы design::*.

## Таблица `pdm_project` — PDM-проект (версионная ветка)

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `base_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` — первый снэпшот ветки |
| `head_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` — актуальный (текущий) снэпшот |

На момент создания PdmProject оба `*_snapshot_id == NULL`. После первого
парсинга PDMManager создаёт `Snapshot #1` и выставляет оба в её id.
Далее `head_snapshot_id` двигается при добавлении новых снэпшотов,
`base_snapshot_id` остаётся неизменным.

Связь обратная со стороны `design_project.pdm_project_id`
(один design_project ↔ один pdm_project).

## Таблица `pdm_snapshot` — неизменяемый срез

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `prev_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` — предыдущий |
| `next_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` — следующий |
| `prev_delta_id` | INTEGER NULL | FK → `pdm_delta.id` — дельта, которая привела в этот snapshot |
| `next_delta_id` | INTEGER NULL | FK → `pdm_delta.id` — дельта, которая отсюда уходит |
| `root_assembly_id` | INTEGER NOT NULL | FK → `pdm_assembly.id` — корневая сборка (замороженная копия!) |

**ВАЖНО:** `root_assembly_id` указывает на `pdm_assembly.id`, не на
`design_assembly.id`. Это гарантирует, что после любых правок в
`design_assembly` снэпшот останется консистентным.

`prev_delta_id` / `next_delta_id` дублируют двустороннюю связь со
стороны `pdm_delta` — это ускоряет итерацию по истории (в обе стороны)
без сложных JOIN'ов.

TODO(на подумать): прикладные поля `version`, `status` (DRAFT/APPROVED/
ARCHIVED), `approved_by`, `approved_at`, `error_count`, `warning_count`
структурной схемой не требуются, но нужны для рабочего цикла snapshot'а
(жизненный цикл DRAFT → APPROVED → ARCHIVED). Оставлены в классе как
прикладные атрибуты.

## Таблица `pdm_delta` — инкрементальное изменение

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `prev_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` — от какого снэпшота |
| `next_snapshot_id` | INTEGER NULL | FK → `pdm_snapshot.id` — к какому снэпшоту |
| `prev_delta_id` | INTEGER NULL | FK → `pdm_delta.id` — соседняя дельта слева |
| `next_delta_id` | INTEGER NULL | FK → `pdm_delta.id` — соседняя дельта справа |
| `changed_elements` | TEXT / JSON | Список изменённых элементов (см. ниже) |

`changed_elements` — «что именно изменилось». Структурная схема не
детализирует формат (помечен `!!!`). Возможные варианты реализации:

* отдельная таблица `pdm_delta_changes` с записями (delta_id, element_type,
  element_designation, change_type, changed_fields);
* JSON-поле внутри `pdm_delta` (быстрее, хуже для запросов).

TODO(решить при реализации PDMManager): остановиться на одном варианте.
Пока в классе `Delta` хранится `std::vector<DeltaChange>` с подробной
структурой (см. `pdm/delta.h`) — это соответствует первому варианту.

## Связи (обзор)

```
pdm_project                  1 ─ 1 pdm_snapshot (base_snapshot_id)
pdm_project                  1 ─ 1 pdm_snapshot (head_snapshot_id)
pdm_snapshot                 1 ─ 0..1 pdm_snapshot (prev_snapshot_id)
pdm_snapshot                 1 ─ 0..1 pdm_snapshot (next_snapshot_id)
pdm_snapshot                 1 ─ 0..1 pdm_delta    (prev_delta_id)
pdm_snapshot                 1 ─ 0..1 pdm_delta    (next_delta_id)
pdm_delta                    N ─ 0..1 pdm_snapshot (prev_snapshot_id)
pdm_delta                    N ─ 0..1 pdm_snapshot (next_snapshot_id)
pdm_snapshot                 1 ─ 1 pdm_assembly    (root_assembly_id)
pdm_assembly                 N ─ 1 pdm_snapshot    (snapshot_id)
pdm_part                     N ─ 1 pdm_snapshot    (snapshot_id)
pdm_assembly_config          N ─ 1 pdm_snapshot    (snapshot_id)
pdm_part_config              N ─ 1 pdm_snapshot    (snapshot_id)
design_project               1 ─ 0..1 pdm_project  (pdm_project_id)
```
