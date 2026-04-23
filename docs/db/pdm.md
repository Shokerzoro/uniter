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
| `Diagnostic` | `DIAGNOSTIC = 57` | `pdm_diagnostic` |

## Зеркальные таблицы (копии DESIGN в контексте снэпшота)

| Таблица | Роль | Маппится на ResourceType |
|---|---|---|
| `pdm_assembly` | Копия `design_assembly` на момент snapshot | `ASSEMBLY_PDM = 53` |
| `pdm_assembly_config` | Копия `design_assembly_config` | `ASSEMBLY_CONFIG_PDM = 54` |
| `pdm_part` | Копия `design_part` | `PART_PDM = 55` |
| `pdm_part_config` | Копия `design_part_config` | `PART_CONFIG_PDM = 56` |
| `pdm_assembly_config_parts` | join | — |
| `pdm_assembly_config_children` | join | — |
| `pdm_assembly_config_standard_products` | join | — |
| `pdm_assembly_config_materials` | join | — |
| `pdm_snapshot_diagnostics` | join snapshot ↔ diagnostic | — |

**Нумерация PDM-зеркал.** Раньше коды `ASSEMBLY_PDM` / `ASSEMBLY_CONFIG_PDM` /
`PART_PDM` / `PART_CONFIG_PDM` были 35–38 (диапазон DESIGN). После рефакторинга
они перенесены в диапазон PDM (50–59): 53 / 54 / 55 / 56 соответственно.
Это приводит коды в соответствие с фактической подсистемой-владельцем.

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

Диагностика в Snapshot больше НЕ денормализована (удалены поля
`error_count` / `warning_count`). Вместо них введён отдельный ресурс
`Diagnostic` (см. ниже) и связь N:M через `pdm_snapshot_diagnostics`.
Рантайм-класс `Snapshot` сворачивает эту связь в
`std::vector<std::shared_ptr<Diagnostic>> diagnostics`.

TODO(на подумать): прикладные поля `version`, `status` (DRAFT/APPROVED/
ARCHIVED), `approved_by`, `approved_at` структурной схемой не требуются,
но нужны для рабочего цикла snapshot'а (жизненный цикл DRAFT → APPROVED
→ ARCHIVED). Оставлены в классе как прикладные атрибуты.

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

`changed_elements` — «что именно изменилось». В БД раскрывается в
две отдельные таблицы:

| Таблица | Назначение |
|---|---|
| `pdm_delta_changes` | Пары «было → стало»: `(delta_id, element_type, old_ref, new_ref)` |
| `pdm_delta_removed` | Удалённые: `(delta_id, element_type, element_ref)` |

`element_type` — значение enum `DeltaElementType` (ASSEMBLY=0,
ASSEMBLY_CONFIG=1, PART=2, PART_CONFIG=3). `*_ref` — FK на соответствующую
PDM-зеркальную таблицу (`pdm_assembly.id`, `pdm_assembly_config.id`,
`pdm_part.id`, `pdm_part_config.id`).

В runtime-классе `Delta` эти две таблицы сворачиваются в:

```cpp
struct DeltaKey { DeltaElementType type; uint64_t id; };
using ChangesMap = std::map<
    DeltaKey,
    std::pair<std::shared_ptr<ResourceAbstract>,  // было
              std::shared_ptr<ResourceAbstract>>  // стало
>;
struct RemovedItem { DeltaElementType type; std::shared_ptr<ResourceAbstract> item; };

ChangesMap changes;                 // pdm_delta_changes
std::vector<RemovedItem> removed;   // pdm_delta_removed
```

Такая структура позволяет PDMManager'у итерировать по дельтам и
последовательно применять изменения: для каждой пары (old → new)
заменить ссылку, для каждого элемента в removed — удалить. Старые
структуры `DeltaChange` / `DeltaFileChange` удалены — изменения файлов
выражаются через замену PART/PART_CONFIG, а не отдельной записью.

## Таблица `pdm_diagnostic` — ошибка/замечание парсинга КД

Отдельный PDM-ресурс (`ResourceType::DIAGNOSTIC = 57`). Заменяет ранее
денормализованные в Snapshot поля `error_count` / `warning_count`.

| Колонка | Тип | Описание |
|---|---|---|
| `id` | INTEGER PK | |
| *общие поля ResourceAbstract* | | |
| `severity` | INTEGER | ERROR / WARNING / INFO |
| `category` | INTEGER | FILE_SYSTEM / VERSION_CONTROL / PARSING / VALIDATION / … |
| `type` | TEXT | Строковый код ("NO_FILE", "INFORMAL_CHANGE", …) |
| `path` | TEXT | Путь/адрес объекта (PDF-файл, designation, XPath) |
| `message` | TEXT | Человекочитаемое сообщение (опционально) |

`severity` и `category` — enum'ы в C++ (`DiagnosticSeverity`,
`DiagnosticCategory` в `pdm/pdmtypes.h`). `type` оставлен строкой, чтобы
не разрастать enum при добавлении новых видов диагностики.

### Join `pdm_snapshot_diagnostics` (N:M)

| Колонка | Тип | Описание |
|---|---|---|
| `snapshot_id` | INTEGER | FK → `pdm_snapshot.id` |
| `diagnostic_id` | INTEGER | FK → `pdm_diagnostic.id` |

Один снэпшот может иметь N диагностик; одна Diagnostic может
встречаться в нескольких снэпшотах (если проблема повторяется между
версиями). В runtime `Snapshot` хранит
`std::vector<std::shared_ptr<Diagnostic>> diagnostics`; Diagnostic своих
`snapshot_id` не держит — эту сторону связи восстанавливает
DataManager запросом по join-таблице.

## Runtime-представление: внутри-PDM-связи через `shared_ptr`

В БД все связи Snapshot↔Delta↔PdmProject↔root_assembly хранятся как
FK `uint64_t`. Но в runtime-классах `pdm::Snapshot`, `pdm::Delta`,
`pdm::PdmProject` эти связи сворачиваются в `std::shared_ptr<...>`
на сами классы:

* `Snapshot::prev_snapshot`, `Snapshot::next_snapshot` → `shared_ptr<Snapshot>`
* `Snapshot::prev_delta`, `Snapshot::next_delta` → `shared_ptr<Delta>`
* `Snapshot::root_assembly` → `shared_ptr<design::Assembly>`
* `Delta::prev_snapshot`, `Delta::next_snapshot` → `shared_ptr<Snapshot>`
* `Delta::prev_delta`, `Delta::next_delta` → `shared_ptr<Delta>`
* `PdmProject::base_snapshot`, `PdmProject::head_snapshot` → `shared_ptr<Snapshot>`

Это нужно, чтобы PDMManager мог итерировать по цепочке версий
(`pdmProject.base_snapshot → next_snapshot → …`) без дополнительных
запросов к DataManager на каждой итерации и при необходимости применять
дельты к снэпшотам напрямую.

При загрузке из БД DataManager:
1. Собирает все `Snapshot` / `Delta` / `PdmProject`.
2. По FK восстанавливает указатели между объектами.
3. При сохранении читает `.get()` / `.id` с указателей и пишет FK.

`operator==` у Snapshot / Delta / PdmProject сравнивает указатели по
адресу (`.get()`) — это избегает рекурсии в двунаправленном
списке snapshot↔delta.

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
pdm_snapshot                 N ─ M pdm_diagnostic  (join: pdm_snapshot_diagnostics)
pdm_delta                    1 ─ N pdm_delta_changes  (delta_id)
pdm_delta                    1 ─ N pdm_delta_removed  (delta_id)
design_project               1 ─ 0..1 pdm_project  (pdm_project_id)
```

### Метаданные ресурсов PDM

| Класс | Subsystem | GenSubsystem | ResourceType |
|---|---|---|---|
| `PdmProject` | PDM | NOTGEN | PDM_PROJECT (52) |
| `Snapshot` | PDM | NOTGEN | SNAPSHOT (50) |
| `Delta` | PDM | NOTGEN | DELTA (51) |
| `Diagnostic` | PDM | NOTGEN | DIAGNOSTIC (57) |

Поля заполняются автоматически в конструкторе каждого
наследника `ResourceAbstract` и в БД не хранятся (определяются таблицей).
