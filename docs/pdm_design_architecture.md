# Архитектура PDM/DESIGN: взаимодействие подсистем, управление файлами, версионирование

> ⚠️ **УСТАРЕЛО (legacy).** Этот документ описывает предыдущую итерацию
> концепции DESIGN/PDM (Assembly с embedded child_assemblies/parts,
> `Project.active_snapshot_id`, единый `MATERIAL_INSTANCE`). Актуальная
> концепция — в `docs/db/design.md`, `docs/db/pdm.md`,
> `docs/db/pdm_design_logic.md`. Документ оставлен для истории и для
> рабочих процессов, связанных с FileManager/PDMManager, которые пока
> сохраняют валидность.

> Справочник при реализации классов ресурсов `contract/design/`, `contract/pdm/`, схемы SQLite и логики `PDMManager`.

---

## 1. Модель данных: поля ресурсов

### 1.1. ResourceAbstract (базовые поля всех ресурсов)

Наследуются всеми ресурсами ниже — в таблицах не повторяются.

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `id` | `uint64_t` | `id` | `INTEGER PK` | Серверный глобальный ID |
| `is_actual` | `bool` | `actual` | `INTEGER` (0/1) | Soft-delete флаг |
| `created_at` | `QDateTime` | `created_at` | `TEXT` (ISO 8601) | |
| `updated_at` | `QDateTime` | `updated_at` | `TEXT` (ISO 8601) | |
| `created_by` | `uint64_t` | `created_by` | `INTEGER` | FK → employees.id |
| `updated_by` | `uint64_t` | `updated_by` | `INTEGER` | FK → employees.id |

---

### 1.2. Project

**Роль:** источник истины для текущего состояния конструктора. `active_snapshot_id` — «официальная» версия, которая разрешена к использованию другими подсистемами.

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `name` | `QString` | `name` | `TEXT NOT NULL` | Название проекта |
| `description` | `QString` | `description` | `TEXT` | Описание |
| `projectcode` | `QString` | `projectcode` | `TEXT UNIQUE NOT NULL` | Код проекта (ЕСКД-обозначение верхнего уровня) |
| `rootdirectory` | `QString` | `rootdirectory` | `TEXT` | Корневая папка КД (локальный путь или URI) |
| `root_assembly_id` | `uint64_t` | `root_assembly_id` | `INTEGER` | FK → assemblies.id (корневая сборка) |
| `active_snapshot_id` | `std::optional<uint64_t>` | `active_snapshot_id` | `INTEGER NULL` | FK → snapshots.id; NULL пока не утверждён ни один Snapshot |

> **Что добавить в текущий класс:** убрать `std::shared_ptr<Assembly> root_assembly` (eager load), заменить на `uint64_t root_assembly_id`. Добавить `active_snapshot_id`.

---

### 1.3. Assembly

**Роль:** узел дерева сборок. Хранит актуальные object_key файлов — состояние «сейчас».

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `project_id` | `uint64_t` | `project_id` | `INTEGER NOT NULL` | FK → projects.id |
| `parent_assembly_id` | `std::optional<uint64_t>` | `parent_assembly_id` | `INTEGER NULL` | FK → assemblies.id; NULL для корневой |
| `designation` | `QString` | `designation` | `TEXT NOT NULL` | Обозначение по ЕСКД (напр. `СБ-001`) |
| `name` | `QString` | `name` | `TEXT NOT NULL` | Наименование |
| `description` | `QString` | `description` | `TEXT` | |
| `type` | `AssemblyType` | `type` | `INTEGER` | 0=REAL, 1=VIRTUAL |
| `drawing_object_key` | `QString` | `drawing_object_key` | `TEXT` | MinIO key сборочного чертежа (актуальный) |
| `drawing_sha256` | `QString` | `drawing_sha256` | `TEXT` | SHA-256 сборочного чертежа |
| `spec_object_key` | `QString` | `spec_object_key` | `TEXT` | MinIO key спецификации |
| `spec_sha256` | `QString` | `spec_sha256` | `TEXT` | SHA-256 спецификации |
| `mounting_drawing_object_key` | `QString` | `mounting_drawing_object_key` | `TEXT` | MinIO key монтажного чертежа (если есть) |
| `mounting_drawing_sha256` | `QString` | `mounting_drawing_sha256` | `TEXT` | |
| `model_3d_object_key` | `QString` | `model_3d_object_key` | `TEXT` | MinIO key 3D-модели (если есть) |
| `model_3d_sha256` | `QString` | `model_3d_sha256` | `TEXT` | |

**Отдельная таблица `assembly_children`** (связь N:M Assembly → child Assembly с количеством):

| Колонка | Тип | Описание |
|---|---|---|
| `parent_id` | `INTEGER` | FK → assemblies.id |
| `child_id` | `INTEGER` | FK → assemblies.id |
| `quantity` | `INTEGER` | Количество вхождений |
| `config` | `TEXT` | Исполнение (если применимо) |

> **Что добавить в текущий класс:** добавить `designation`, файловые поля с `object_key`/`sha256` вместо `shared_ptr<FileVersion>`. `FileVersion` как отдельная сущность упразднена — история версий файлов хранится в `Delta` (см. ниже). Убрать `part_id_ctr` (серверная ответственность), убрать `assembly_id` (дублирует `id`).

---

### 1.4. Part (PartDef)

**Роль:** листовой узел дерева. Хранит актуальные object_key чертежа и 3D-модели.

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `project_id` | `uint64_t` | `project_id` | `INTEGER NOT NULL` | FK → projects.id |
| `assembly_id` | `uint64_t` | `assembly_id` | `INTEGER NOT NULL` | FK → assemblies.id (родительская сборка) |
| `designation` | `QString` | `designation` | `TEXT NOT NULL` | Обозначение ЕСКД (напр. `СБ-001-01`) |
| `name` | `QString` | `name` | `TEXT NOT NULL` | Наименование |
| `litera` | `QString` | `litera` | `TEXT` | Литера КД (О, О1, А и т.д.) |
| `organization` | `QString` | `organization` | `TEXT` | Организация-разработчик |
| `material_instance_id` | `std::optional<uint64_t>` | `material_instance_id` | `INTEGER NULL` | FK → material_instances.id |
| `drawing_object_key` | `QString` | `drawing_object_key` | `TEXT` | MinIO key чертежа детали (актуальный) |
| `drawing_sha256` | `QString` | `drawing_sha256` | `TEXT` | SHA-256 чертежа |
| `drawing_modified_at` | `QDateTime` | `drawing_modified_at` | `TEXT` | Дата модификации файла чертежа |
| `model_3d_object_key` | `QString` | `model_3d_object_key` | `TEXT` | MinIO key 3D-модели (опционально) |
| `model_3d_sha256` | `QString` | `model_3d_sha256` | `TEXT` | |

**Отдельная таблица `part_configs`** (конфигурации исполнений одной детали):

| Колонка | Тип | Описание |
|---|---|---|
| `part_id` | `INTEGER` | FK → parts.id |
| `config_id` | `TEXT` | Идентификатор исполнения (`01`, `02`, ...) |
| `length_mm` | `REAL` | |
| `width_mm` | `REAL` | |
| `height_mm` | `REAL` | |
| `mass_kg` | `REAL` | |

**Отдельная таблица `part_signatures`**:

| Колонка | Тип | Описание |
|---|---|---|
| `part_id` | `INTEGER` | FK → parts.id |
| `role` | `TEXT` | Роль подписанта (Разработал, Проверил, ...) |
| `name` | `TEXT` | ФИО |
| `date` | `TEXT` | Дата подписи (ISO 8601) |

> **Что добавить в текущий класс:** добавить `designation`, `litera`, `organization`, `material_instance_id`, заменить `vector<shared_ptr<FileVersion>>` на плоские поля `drawing_object_key`/`drawing_sha256`/`drawing_modified_at` и `model_3d_object_key`/`model_3d_sha256`. Убрать `source_project_id` (дублирует `project_id`).

---

### 1.5. Snapshot

**Роль:** зафиксированный срез проекта в момент парсинга/утверждения. Аналог ветки git в том смысле, что один Project может иметь несколько Snapshot (разные версии), и каждый Snapshot независимо движется через жизненный цикл DRAFT → APPROVED → ARCHIVED.

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `project_id` | `uint64_t` | `project_id` | `INTEGER NOT NULL` | FK → projects.id |
| `version` | `uint32_t` | `version` | `INTEGER NOT NULL` | Монотонно возрастающий номер версии |
| `previous_snapshot_id` | `std::optional<uint64_t>` | `previous_snapshot_id` | `INTEGER NULL` | FK → snapshots.id; NULL для первого |
| `xml_object_key` | `QString` | `xml_object_key` | `TEXT NOT NULL` | MinIO key XML-файла снэпшота |
| `xml_sha256` | `QString` | `xml_sha256` | `TEXT NOT NULL` | SHA-256 XML-файла |
| `status` | `SnapshotStatus` | `status` | `INTEGER` | 0=DRAFT, 1=APPROVED, 2=ARCHIVED |
| `approved_by` | `std::optional<uint64_t>` | `approved_by` | `INTEGER NULL` | FK → employees.id |
| `approved_at` | `std::optional<QDateTime>` | `approved_at` | `TEXT NULL` | |
| `error_count` | `uint32_t` | `error_count` | `INTEGER` | Количество ошибок валидации ЕСКД |
| `warning_count` | `uint32_t` | `warning_count` | `INTEGER` | Количество предупреждений |

Enum `SnapshotStatus`: `DRAFT = 0`, `APPROVED = 1`, `ARCHIVED = 2`.

> Snapshot не хранит inline-список деталей/сборок — только `xml_object_key`. Структура читается из MinIO при необходимости (просмотр версии, сравнение).

---

### 1.6. Delta

**Роль:** инкрементальные изменения между базовым Snapshot и следующим. Delta = «что изменилось» при переходе от `snapshot_id` к следующей версии.

**Таблица `deltas`:**

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `snapshot_id` | `uint64_t` | `snapshot_id` | `INTEGER NOT NULL` | FK → snapshots.id (базовая версия, ОТ которой считается дельта) |
| `next_snapshot_id` | `uint64_t` | `next_snapshot_id` | `INTEGER NOT NULL` | FK → snapshots.id (версия, ДО которой применяется дельта) |
| `changes_count` | `uint32_t` | `changes_count` | `INTEGER` | Количество изменений в дельте |

**Таблица `delta_changes`** (одна строка = одно изменение в дельте):

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `delta_id` | `uint64_t` | `delta_id` | `INTEGER NOT NULL` | FK → deltas.id |
| `designation` | `QString` | `designation` | `TEXT NOT NULL` | Обозначение изменённого элемента (Part или Assembly) |
| `element_type` | `DeltaElementType` | `element_type` | `INTEGER` | 0=PART, 1=ASSEMBLY |
| `change_type` | `DeltaChangeType` | `change_type` | `INTEGER` | 0=ADD, 1=MODIFY, 2=DELETE |
| `changed_fields` | `QString` | `changed_fields` | `TEXT` | JSON-список изменённых полей (напр. `["drawing","litera"]`) |

**Таблица `delta_file_changes`** (файловые изменения внутри одного delta_change):

| Поле C++ | Тип C++ | Колонка SQLite | Тип SQLite | Описание |
|---|---|---|---|---|
| `delta_change_id` | `uint64_t` | `delta_change_id` | `INTEGER NOT NULL` | FK → delta_changes.id |
| `file_type` | `DocumentType` | `file_type` | `INTEGER` | Тип документа (PART_DRAWING, ASSEMBLY_DRAWING, MODEL_3D, ...) |
| `old_object_key` | `QString` | `old_object_key` | `TEXT` | MinIO key до изменения (пусто при ADD) |
| `old_sha256` | `QString` | `old_sha256` | `TEXT` | |
| `new_object_key` | `QString` | `new_object_key` | `TEXT` | MinIO key после изменения (пусто при DELETE) |
| `new_sha256` | `QString` | `new_sha256` | `TEXT` | |

---

## 2. Файлы: дублирование и его смысл

Разработчик сформулировал это как «некоторое дублирование, но не критично». Это не дублирование — это три разных семантических уровня:

| Где хранится | Что хранит | Смысл |
|---|---|---|
| `Part.drawing_object_key` / `Assembly.drawing_object_key` | Актуальный object_key **сейчас** | «Вот файл, с которым работаем сегодня» — живое состояние DESIGN |
| `Snapshot.xml_object_key` | object_key XML-файла снэпшота | «Вот зафиксированный состав проекта на момент версии N» — XML содержит обозначения и ссылки на object_key файлов, существовавших на тот момент |
| `delta_file_changes.new_object_key` | object_key файла в конкретном изменении | «Вот что конкретно изменилось в файле при переходе от версии N к N+1» |

**Почему не дублирование:**

- DESIGN-запись хранит **один** актуальный ключ. При обновлении чертежа `drawing_object_key` перезаписывается. Старый ключ при этом не теряется — он зафиксирован в `delta_file_changes.old_object_key`.
- Snapshot XML хранит ссылки на object_key файлов **на момент снэпшота**, не копии файлов. Объекты в MinIO неизменны (immutable by key) — один и тот же ключ всегда отдаёт один и тот же файл. Поэтому ссылка в Snapshot остаётся валидной даже после обновления DESIGN.
- Delta хранит пары `{old_key, new_key}` для каждого изменённого файла — это и есть механизм итерирования по истории.

**Итоговая формула:**

```
DESIGN.Part.drawing_object_key == Snapshot_N.xml содержит этот же ключ
                                == delta_file_changes.new_object_key (последней Delta)
```

Они равны, но по разным причинам. DESIGN — «что сейчас», Snapshot — «что было зафиксировано», Delta — «как мы туда пришли». При следующем парсинге DESIGN получит новый ключ, Snapshot зафиксирует старый, Delta запишет переход.

---

## 3. Утверждённый Snapshot и межсистемные ссылки

### Что означает `active_snapshot_id`

`Project.active_snapshot_id` — FK на Snapshot со статусом `APPROVED`. Это «официальная» версия проекта: та, которую приняли к производству, закупкам и т.д.

- Пока Snapshot не утверждён (`status = DRAFT`), `active_snapshot_id` равен NULL или указывает на предыдущий APPROVED Snapshot.
- Утверждение Snapshot (approve) = транзакция: `snapshot.status → APPROVED` + `project.active_snapshot_id = snapshot.id` + предыдущий APPROVED → `ARCHIVED`.

### Как ProductionTask ссылается на состав

`Task` (в `task.h`) уже содержит `project_id`. При создании задания ERP использует `project.active_snapshot_id` для получения конкретного состава. Правильная схема хранения:

```
Task {
    project_id,
    snapshot_id   ← добавить: ID снэпшота, зафиксированного при создании задания
}
```

Это критично: если Project получит новый APPROVED Snapshot, уже созданные задания должны ссылаться на снэпшот, который действовал **при создании задания**, а не на текущий. В `add_data_convention.md` это уже предусмотрено (`"snapshot_id"` в add_data при CREATE PRODUCTION_TASK) — нужно зафиксировать `snapshot_id` как обязательное поле в самом ресурсе `Task`.

### Жизненный цикл Snapshot

```
DRAFT → APPROVED → ARCHIVED

DRAFT:    создан PDMManager после парсинга; видим только PDM-менеджерам
APPROVED: утверждён ответственным; project.active_snapshot_id → этот Snapshot
          другие подсистемы (ERP, производство) получают его как актуальный состав
ARCHIVED: при утверждении следующей версии; данные сохранены, Task'и ссылаются на него
```

Snapshot никогда не удаляется — только архивируется. Это обеспечивает воспроизводимость: по любому историческому Task всегда можно восстановить состав на момент его создания.

### Что происходит при утверждении нового Snapshot

1. PDMManager создаёт новый Snapshot (`status = DRAFT`).
2. Ответственный просматривает diff (Delta между предыдущим и новым), принимает решение.
3. PDMManager вызывает `approve(snapshot_id)`:
   - `UPDATE snapshots SET status=ARCHIVED WHERE id = project.active_snapshot_id`
   - `UPDATE snapshots SET status=APPROVED, approved_by=..., approved_at=... WHERE id = snapshot_id`
   - `UPDATE projects SET active_snapshot_id = snapshot_id WHERE id = project_id`
4. Kafka рассылает NOTIFICATION: `CRUD UPDATE, ResourceType::PROJECT` — все клиенты обновляют `project.active_snapshot_id`.
5. UI конструктора и ERP перечитывают состав через новый `active_snapshot_id`.

---

## 4. Как итерироваться по изменениям

**Вопрос:** если `object_key` хранятся в DESIGN-записях (Part, Assembly), как пройтись по истории изменений конкретного файла?

**Ответ:** через цепочку Delta. Delta содержит собственные ссылки на изменённые файлы (`old_object_key` / `new_object_key`) — это не избыточность, а механизм версионирования.

### Алгоритм: история файла чертежа детали по designation

Задача: получить полную историю чертежа детали `СБ-001-01`.

1. По `designation = 'СБ-001-01'` найти `Part` в таблице `parts` → получить `id` и текущий `drawing_object_key` (это состояние «сейчас»).
2. Определить Snapshot-цепочку для `project_id`:
   ```sql
   SELECT id, version, previous_snapshot_id
   FROM snapshots
   WHERE project_id = ?
   ORDER BY version DESC
   ```
3. Для каждого перехода `(snapshot_id → next_snapshot_id)` найти Delta:
   ```sql
   SELECT dc.id, dc.change_type, dfc.old_object_key, dfc.new_object_key, dfc.new_sha256
   FROM delta_changes dc
   JOIN delta_file_changes dfc ON dfc.delta_change_id = dc.id
   JOIN deltas d ON d.id = dc.delta_id
   WHERE dc.designation = 'СБ-001-01'
     AND dfc.file_type = 2  -- PART_DRAWING
   ORDER BY d.next_snapshot_id DESC
   ```
4. Результат — упорядоченный список изменений:

   | Версия (next) | Тип изменения | old_object_key | new_object_key |
   |---|---|---|---|
   | 3 | MODIFY | `drawings/SB-001-01/v2.pdf` | `drawings/SB-001-01/v3.pdf` |
   | 2 | MODIFY | `drawings/SB-001-01/v1.pdf` | `drawings/SB-001-01/v2.pdf` |
   | 1 | ADD | `` | `drawings/SB-001-01/v1.pdf` |

5. Для скачивания любой версии — запрос presigned URL по `object_key` версии через `GET_MINIO_PRESIGNED_URL`.

**Важно:** поле `designation` в `delta_changes` — не FK, а текстовый идентификатор. Это намеренно: Delta описывает состояние в момент создания Snapshot, когда Part ещё мог не существовать как DESIGN-запись (например, при первом парсинге). Связь через `designation` устойчива к переименованиям id на сервере.

---

## 5. Процессы PDMManager (алгоритмы)

### 5.1. Первый парсинг / создание проекта

1. Пользователь указывает корневую папку с PDF-файлами КД.
2. PDMManager формирует список PDF → передаёт root XMLElement в библиотеку-компилятор чертежей.
3. Компилятор возвращает заполненный XML-документ (структура Snapshot XML в памяти).
4. PDMManager валидирует по ЕСКД → фиксирует `error_count`, `warning_count`.
5. Загрузка файлов в MinIO:
   - Для каждого PDF: `GET_MINIO_PRESIGNED_URL (PUT)` → `MinioConnector::put(url, localPath)`.
   - Сохранить `object_key` и `sha256` для каждого файла.
6. Создание DESIGN-записей (через ServerConnector, CRUD CREATE):
   - `Project` → получить `project.id`.
   - `Assembly[]` (обход дерева в ширину, родители до детей) → получить `assembly.id` для каждого.
   - `Part[]` → получить `part.id` для каждого.
   - В каждую запись прописать `drawing_object_key`, `drawing_sha256`.
7. Сохранить Snapshot XML-файл в MinIO → получить `xml_object_key`, `xml_sha256`.
8. Создать запись `Snapshot` (CRUD CREATE): `project_id`, `version=1`, `previous_snapshot_id=NULL`, `xml_object_key`, `status=DRAFT`.
9. Нет Delta для первой версии (нет базы для сравнения).

### 5.2. Повторный парсинг / создание Delta + нового Snapshot

1. Пользователь запускает повторный парсинг для существующего проекта.
2. Компилятор строит новый XML-документ в памяти (аналогично п.2-3 выше).
3. PDMManager загружает текущий Snapshot XML из MinIO (по `active_snapshot_id.xml_object_key`).
4. **Сравнение двух XML-документов** (diff по designation):
   - Новые designation → `change_type = ADD`.
   - Удалённые designation → `change_type = DELETE`.
   - Одинаковые designation с изменёнными полями → `change_type = MODIFY`. Для файлов: сравниваем SHA-256.
5. Загрузить в MinIO только изменённые/новые файлы.
6. Обновить DESIGN-записи через CRUD UPDATE:
   - Изменённые Part/Assembly: обновить `drawing_object_key`, `drawing_sha256`, `drawing_modified_at`.
   - Новые Part/Assembly: CRUD CREATE.
   - Удалённые: CRUD DELETE (soft delete через `actual = false`).
7. Сохранить новый Snapshot XML в MinIO.
8. Создать запись `Snapshot` (CRUD CREATE): `version = prev.version + 1`, `previous_snapshot_id = prev.id`, `status = DRAFT`.
9. Создать запись `Delta` + `delta_changes[]` + `delta_file_changes[]` (CRUD CREATE).

### 5.3. Утверждение Snapshot (approve)

1. Пользователь с правами PDM-менеджера выбирает Snapshot со статусом DRAFT.
2. PDMManager запрашивает Delta (если есть) для отображения diff пользователю.
3. Пользователь подтверждает → PDMManager отправляет:
   - `CRUD UPDATE, ResourceType::SNAPSHOT` с `status = APPROVED`, `approved_by`, `approved_at`.
   - `CRUD UPDATE, ResourceType::PROJECT` с `active_snapshot_id = snapshot.id`.
   - `CRUD UPDATE, ResourceType::SNAPSHOT` для предыдущего APPROVED с `status = ARCHIVED`.
4. Kafka рассылает NOTIFICATION по всем трём UPDATE.
5. DataManager на всех клиентах применяет изменения; UI конструктора и ERP перечитывают активный состав.

### 5.4. Откат к предыдущей версии

1. PDMManager находит Snapshot с нужной версией (`previous_snapshot_id` цепочка).
2. Загружает его XML из MinIO → строит список Part/Assembly.
3. Для каждой детали/сборки: `CRUD UPDATE` в DESIGN с `object_key` и `sha256` из той версии (берутся из `delta_file_changes.old_object_key` цепочки Delta в обратном порядке).
4. Создаёт новый Snapshot (версия N+1) на основе откаченного состава — откат фиксируется как новая версия, а не удаление истории.
5. `status = DRAFT` → проходит через стандартный approve-процесс.

---

## 6. Схема ссылок

```
                     ┌─────────────────────────────────────────────────────┐
                     │                   DESIGN                            │
                     │                                                      │
                     │  Project ──────────────────────────────────────────►│ active_snapshot_id ─────►  Snapshot (PDM)
                     │    │                                                 │
                     │    └─► root_assembly_id ──────────────────────────► Assembly
                     │                                                      │    │
                     │                                    child_assemblies ◄┘    │
                     │                                                           └─► Part
                     │                                                                │
                     │                                             drawing_object_key │
                     │                                             model_3d_object_key│
                     └───────────────────────────────────────────────────────────────┘
                                                                           │
                                                                           ▼
                                                                      MinIO (объекты)
                                                                           ▲
                     ┌─────────────────────────────────────────────────────┤
                     │                     PDM                             │
                     │                                                      │
                     │  Snapshot ──────────────────────────────────────────┘
                     │    │  xml_object_key (XML-файл снэпшота)
                     │    │  project_id ──────────────────────────────────► Project (DESIGN)
                     │    │  previous_snapshot_id ────────────────────────► Snapshot (себя)
                     │    │
                     │    └─► Delta
                     │          │  snapshot_id (базовая версия)
                     │          │  next_snapshot_id
                     │          │
                     │          └─► delta_changes[]
                     │                │  designation ──── (текстовая ссылка) ──► Part / Assembly
                     │                └─► delta_file_changes[]
                     │                      old_object_key ──────────────────────► MinIO
                     │                      new_object_key ──────────────────────► MinIO
                     └──────────────────────────────────────────────────────────────────
                     
                     ┌──────────────────────────────────────────────────────────────────┐
                     │                   Межсистемные ссылки                            │
                     │                                                                   │
                     │  Task (GENERATIVE::PRODUCTION)                                   │
                     │    project_id ─────────────────────────────────────► Project     │
                     │    snapshot_id ────────────────────────────────────► Snapshot    │
                     │    (берёт состав из Snapshot, зафиксированного при создании)     │
                     │                                                                   │
                     │  ProcurementRequest (PURCHASES)                                  │
                     │    part_id ────────────────────────────────────────► Part        │
                     │    material_instance_id ───────────────────────────► MaterialInstance │
                     └──────────────────────────────────────────────────────────────────┘
```

---

## 7. Что нужно реализовать в contract/

### 7.1. Существующие классы — что изменить

| Класс | Файл | Изменения |
|---|---|---|
| `Project` | `contract/design/project.h` | Убрать `shared_ptr<Assembly> root_assembly`. Добавить: `uint64_t root_assembly_id`, `std::optional<uint64_t> active_snapshot_id` |
| `Assembly` | `contract/design/assembly.h` | Добавить: `QString designation`, `QString drawing_object_key`, `QString drawing_sha256`, `QString spec_object_key`, `QString spec_sha256`, `QString mounting_drawing_object_key`, `QString mounting_drawing_sha256`, `QString model_3d_object_key`, `QString model_3d_sha256`. Убрать: `shared_ptr<FileVersion>` поля, `assembly_id` (дублирует `id`), `part_id_ctr` |
| `Part` | `contract/design/part.h` | Добавить: `QString designation`, `QString litera`, `QString organization`, `uint64_t project_id`, `std::optional<uint64_t> material_instance_id`, `QString drawing_object_key`, `QString drawing_sha256`, `QDateTime drawing_modified_at`, `QString model_3d_object_key`, `QString model_3d_sha256`. Убрать: `vector<shared_ptr<FileVersion>>` поля, `source_project_id` (переименовать в `project_id`), `part_id` (дублирует `id`) |
| `FileVersion` | `contract/design/fileversion.h` | **Упразднить** как сущность. История версий файлов теперь в `Delta`. Поля approval workflow (proposed_by, approved_by) перенести в `Part`/`Assembly` при необходимости |
| `Task` | `contract/plant/task.h` | Добавить: `uint64_t snapshot_id` (ID Snapshot, зафиксированного при создании задания) |

### 7.2. Новые классы — создать

| Класс | Файл | ResourceType (уже есть) |
|---|---|---|
| `Snapshot` | `contract/pdm/snapshot.h` | `ResourceType::SNAPSHOT = 50` ✓ |
| `Delta` | `contract/pdm/delta.h` | `ResourceType::DELTA = 51` ✓ |

**`Snapshot`** — наследует `ResourceAbstract`. Поля: `project_id`, `version`, `previous_snapshot_id`, `xml_object_key`, `xml_sha256`, `status` (enum `SnapshotStatus`), `approved_by`, `approved_at`, `error_count`, `warning_count`.

**`Delta`** — наследует `ResourceAbstract`. Поля: `snapshot_id`, `next_snapshot_id`, `changes_count`. Вложенные структуры:

```cpp
struct DeltaFileChange {
    uint64_t       id;
    DocumentType   file_type;
    QString        old_object_key;
    QString        old_sha256;
    QString        new_object_key;
    QString        new_sha256;
};

struct DeltaChange {
    uint64_t                     id;
    QString                      designation;
    DeltaElementType             element_type;   // PART / ASSEMBLY
    DeltaChangeType              change_type;    // ADD / MODIFY / DELETE
    QStringList                  changed_fields;
    std::vector<DeltaFileChange> file_changes;
};

class Delta : public ResourceAbstract {
    uint64_t                    snapshot_id;
    uint64_t                    next_snapshot_id;
    std::vector<DeltaChange>    changes;
    // ...
};
```

**Новые enum'ы** (добавить в `contract/pdm/pdmtypes.h` или аналогичный файл):

```cpp
enum class SnapshotStatus : uint8_t { DRAFT = 0, APPROVED = 1, ARCHIVED = 2 };
enum class DeltaElementType : uint8_t { PART = 0, ASSEMBLY = 1 };
enum class DeltaChangeType  : uint8_t { ADD = 0, MODIFY = 1, DELETE = 2 };
```

### 7.3. Вспомогательные структуры Part/Assembly — добавить

```cpp
// В contract/design/part.h
struct PartConfig {
    QString config_id;   // "01", "02", ...
    double  length_mm = 0;
    double  width_mm  = 0;
    double  height_mm = 0;
    double  mass_kg   = 0;
};

struct PartSignature {
    QString role;
    QString name;
    QDateTime date;
};
```

### 7.4. ResourceType — статус

Все необходимые `ResourceType` уже определены в `uniterprotocol.h`:
- `PROJECT = 30`, `ASSEMBLY = 31`, `PART = 32` — есть ✓
- `SNAPSHOT = 50`, `DELTA = 51` — есть ✓

Добавить `Subsystem::PDM = 7` — уже есть ✓

### 7.5. Структура директорий contract/ после изменений

```
contract/
├── design/
│   ├── project.h / .cpp
│   ├── assembly.h / .cpp
│   ├── part.h / .cpp
│   └── designtypes.h       ← AssemblyType, DocumentType (из fileversion.h), PartConfig, PartSignature
├── pdm/
│   ├── snapshot.h / .cpp   ← новый
│   ├── delta.h / .cpp      ← новый
│   └── pdmtypes.h          ← новый: SnapshotStatus, DeltaElementType, DeltaChangeType
├── resourceabstract.h / .cpp
├── unitermessage.h / .cpp
└── uniterprotocol.h
```

`fileversion.h` — удалить после переноса `DocumentType` в `designtypes.h`.
