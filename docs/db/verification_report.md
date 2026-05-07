# Верификация PDM↔DESIGN

## Вердикт

**MINOR-ISSUES** — одна реальная некорректность в документации (устаревший комментарий в `pdmtypes.h`), одна структурная неоднозначность (отсутствие `snapshot_id` в C++ классах зеркал), одна некорректная ссылка в схеме PDF (project_id у `design_part` не отражён в PDF). Критических ошибок нет.

---

## Проблемы

### [MINOR] Устаревший комментарий в `pdmtypes.h`: ссылается на удалённое поле `Project.active_snapshot_id`

- **Где:** `src/uniter/contract/pdm/pdmtypes.h`, строка 12
- **Проблема:** Комментарий к `SnapshotStatus::APPROVED` гласит:
  > `APPROVED — утверждён ответственным; Project.active_snapshot_id указывает сюда.`

  Поле `active_snapshot_id` было удалено из `design::Project` — вместо него теперь `pdm_project_id`. После замены этот комментарий указывает на несуществующее поле и вводит в заблуждение при реализации цикла утверждения.

- **Рекомендация:** Заменить на:
  ```
  APPROVED — утверждён ответственным; pdm_project.head_snapshot_id может
  указывать на этот или более поздний снэпшот; статус хранится внутри
  pdm_snapshot.status. При реализации цикла утверждения рассмотреть
  pdm_project.approved_snapshot_id (см. pdm_design_logic.md §5).
  ```

---

### [MINOR] `snapshot_id` у PDM-зеркал не представлен в C++ классах — нет явного контракта передачи

- **Где:** `docs/db/pdm.md` строка 46–53; `docs/db/pdm_design_logic.md` строка 45–49; файлы `design/assembly.h`, `design/part.h`, `design/assemblyconfig.h`, `design/partconfig.h`
- **Проблема:** Документация говорит, что PDM-зеркала (`pdm_assembly` и др.) «на уровне полей отличаются только добавлением FK `snapshot_id`». Однако классы `design::Assembly`, `design::Part`, `design::AssemblyConfig`, `design::PartConfig` этого поля не содержат — они напрямую переиспользуются как PDM-ресурсы. Документация сама помечает это `TODO`:
  > «`snapshot_id` — это поле живёт в таблице БД, в классе C++ передаётся через отдельный контекст DataManager или, при необходимости, через `std::optional<uint64_t> snapshot_id`»

  Это оставляет открытым вопрос: как PDMManager при создании snapshot передаёт `snapshot_id` новых pdm-строк через UniterMessage? Если поля нет в классе — нет и возможности сериализовать его в стандартном CRUD-сообщении. Придётся либо добавлять `snapshot_id` в сам класс (загрязняя DESIGN-класс PDM-семантикой), либо передавать его через `add_data` сообщения (нестандартно).

- **Рекомендация:** До начала реализации DataManager принять явное решение и зафиксировать его в документации: либо ввести `std::optional<uint64_t> snapshot_id` в базовый класс (с NOTE, что для DESIGN-ресурсов оно всегда NULL), либо документировать соглашение о передаче через `add_data["snapshot_id"]` при `ResourceType::*_PDM`. Текущий статус `TODO` — нормально, но он должен быть разрешён до написания DataManager.

---

### [MINOR] `design/part` не имеет `project_id` в PDF-схеме DESIGN-4.pdf — расхождение документ/код

- **Где:** `DESIGN-4.pdf` (таблица `design/part`); `src/uniter/contract/design/part.h` строка 56; `docs/db/design.md` строка 145
- **Проблема:** В PDF-схеме DESIGN-4.pdf таблица `design/part` содержит колонки: `id (PK)`, `doc_link_id (FK) - mult`, `instance_composite_id (FK)`, `instance_simple_id (FK)`. Колонки `project_id` в PDF нет. Тем не менее `design.md` и `part.h` объявляют `project_id: INTEGER NOT NULL FK → design_project.id`. Это не ошибка логики (FK на project нужен для классификации деталей по проектам и для корректной загрузки при FULL_SYNC), но расхождение между PDF-схемой и документацией/кодом.
- **Рекомендация:** Добавить `project_id (FK)` в PDF-схему при следующей ревизии DESIGN-4.pdf, либо сделать примечание в `design.md`, что PDF — неполная схема (показывает только структурно значимые FK, опуская классификационные FK уровня проекта).

---

## Подтверждённо корректные решения

- **Инвариантность snapshot:** `pdm_snapshot.root_assembly_id` корректно указывает на `pdm_assembly.id` (не на `design_assembly.id`) — подтверждается в `snapshot.h:63`, `pdm.md:88–92`, `pdm_design_logic.md:32–33`. Любые правки `design_*` не затрагивают замороженные копии.

- **Механизм создания snapshot (§3 pdm_design_logic.md):** последовательность шагов 3.1 и 3.2 логически корректна. Порядок FK: сначала копируются pdm-строки, потом проставляется `root_assembly_id` снэпшота (шаг 4 по скопированному id), затем обновляется `pdm_project`. Race condition отсутствует — вся процедура выполняется внутри одной транзакции PDMManager, никакой внешней видимости промежуточных состояний нет (зеркала создаются только PDMManager).

- **Двусвязный список snapshot/delta:** поля `prev_delta_id/next_delta_id` у `pdm_snapshot` и симметричные у `pdm_delta` корректно задокументированы как намеренная денормализация. Порядок обновления при вставке новой дельты (шаги 3.2.5) описан исчерпывающе: обновляются `prev_head.next_snapshot_id`, `prev_head.next_delta_id`, `new_snapshot.prev_delta_id`. Классы `Snapshot` и `Delta` в C++ имеют все необходимые поля.

- **Замена `active_snapshot_id` на `pdm_project_id`:** архитектурно обоснована — устраняет дублирование «какой снэпшот актуален» и упрощает DESIGN (проект не знает о конкретных снэпшотах). `design::Project` корректно содержит только `pdm_project_id` (`project.h:60`). Маршрут «кто спрашивает — тот и читает `pdm_project.head_snapshot_id`» задокументирован в `pdm_design_logic.md §5`.

- **AssemblyConfig/PartConfig как отдельные ресурсы (ResourceType=33/34):** оправдано — позволяет CRUD по исполнению независимо от самой сборки/детали. PDF-схема DESIGN-4.pdf показывает `design_assembly_config` отдельной таблицей с FK `assembly_id`, что соответствует отдельному ResourceType. Для `design_part_config` PDF показывает отдельную таблицу с `part_id (FK)` — тоже корректно.

- **Протокол: operator<<:** все ResourceType, объявленные в enum (DEFAULT, EMPLOYEES, PRODUCTION, INTEGRATION, MATERIAL_TEMPLATE_SIMPLE/COMPOSITE, PROJECT, ASSEMBLY, PART, ASSEMBLY_CONFIG, PART_CONFIG, ASSEMBLY_PDM, ASSEMBLY_CONFIG_PDM, PART_PDM, PART_CONFIG_PDM, PURCHASE_GROUP, PURCHASE, SNAPSHOT, DELTA, PDM_PROJECT, MATERIAL_INSTANCE_SIMPLE/COMPOSITE, PRODUCTION_TASK/STOCK/SUPPLY, INTEGRATION_TASK, DOC, DOC_LINK) — все присутствуют в `operator<<`. Дубликатов нет, пропусков нет.

- **Удаление `MATERIAL_INSTANCE=60`:** значение 60 не занято ни одним новым ResourceType. В коде осталась только одна ссылка — комментарий в `instancebase.h:55`, где говорится `«MATERIAL_INSTANCE = 60 в ResourceType, поэтому должен быть полноценным ресурсом»`. Это устаревший исторический комментарий (описывал старую схему до разделения), но он не вызывает ошибок компиляции или логических коллизий — просто дезориентирует. Фактически `InstanceBase` по-прежнему правильно является наследником `ResourceAbstract`; ResourceType теперь `MATERIAL_INSTANCE_SIMPLE=61` и `MATERIAL_INSTANCE_COMPOSITE=62`.

- **ЕСКД-соответствие standard_products = composite, materials = simple:** корректно. По ЕСКД стандартные изделия (болты, подшипники) всегда идентифицируются двухчастным обозначением «название / ГОСТ» — это именно `MaterialInstanceComposite`. Листовой/прутковый материал — одночастное обозначение (simple). Реализовано в join-таблицах `design_assembly_config_standard_products` (FK → `material_instances_composite`) и `design_assembly_config_materials` (FK → `material_instances_simple`). Подтверждено в `designtypes.h:88–125`.

- **Consistency FK-связей:** все FK, упомянутые в `design.md` и `pdm.md`, присутствуют в соответствующих классах C++. Висячих ссылок нет: `Project.root_assembly_id → design_assembly.id` ✓, `Assembly.project_id → design_project.id` ✓, `AssemblyConfig.assembly_id → design_assembly.id` ✓, `Part.project_id → design_project.id` ✓, `PartConfig.part_id → design_part.id` ✓, `PdmProject.base/head_snapshot_id → pdm_snapshot.id` ✓, `Snapshot.root_assembly_id → pdm_assembly.id` (задокументировано явно) ✓, `Delta.prev/next_snapshot_id → pdm_snapshot.id` ✓.

---

## TODO-пункты, которые следует отметить в документации

- **`instancebase.h:55`** — устаревший комментарий `«MATERIAL_INSTANCE = 60 в ResourceType»`. Обновить на `«MATERIAL_INSTANCE_SIMPLE = 61 / MATERIAL_INSTANCE_COMPOSITE = 62»`.

- **`pdm.md` и `pdm_design_logic.md`** — TODO про `snapshot_id` в PDM-зеркалах должен быть разрешён до начала реализации DataManager, а не при ней. Рекомендуется ввести соглашение `add_data["snapshot_id"]` или явное поле в классе и зафиксировать выбор.

- **`pdm_design_logic.md §3.2, шаг 4 (дельта):** `prev_delta_id` новой дельты** — в тексте описано как «указывает на предыдущую дельту в цепочке, если она существует». Это означает, что при создании второго снэпшота (первая дельта) `new_delta.prev_delta_id = NULL`. При создании третьего снэпшота (вторая дельта) `new_delta.prev_delta_id = delta_1.id`. Логика верна, но в тексте §3.2 шаг 4 абзац про `prev_delta_id` несколько запутан из-за ссылки на «предыдущую дельту в цепочке» одновременно с описанием операции создания первой дельты. Рекомендуется уточнить: «`new_delta.prev_delta_id = prev_head.prev_delta_id` (`NULL` если это первая дельта проекта)».

- **`docs/db/pdm.md` (статус snapshot):** TODO на `version`, `status`, `approved_by`, `approved_at` отмечен корректно. При реализации рабочего цикла следует также определить, нужен ли `pdm_project.approved_snapshot_id` отдельно от `head_snapshot_id`, чтобы различать «последний созданный» и «последний утверждённый».
