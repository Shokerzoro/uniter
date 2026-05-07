# Subsystem::DOCUMENTS — заметка по БД

Схема-эталон: `DOCUMENTS.pdf` (концептуальная схема, показывает только
PK/FK и основные столбцы).

## Классы в рантайме

- `documents::DocLink` — «папка» документов, прикреплённая к одному
  целевому ресурсу (Assembly, Part, Project, MaterialTemplate*).
  Хранит **только** свой `id`, `DocLinkTargetType` и свёрнутый
  `std::vector<Doc> docs`. Никаких связочных полей (target_id, role,
  position, doc_id) в рантайм-классе нет.
- `documents::Doc` — один файл в MinIO. Хранит **только** свои данные
  (object_key, sha256, doc_type, name, size_bytes, mime_type,
  description, optional local_path). Никаких FK / ссылок на DocLink в
  рантайм-классе нет — связка видна снаружи как `DocLink::docs`.

> Важно: PDF моделирует только концептуальную схему БД. Прочие поля
> классов (name, size_bytes, mime_type, description и т.п.)
> сохраняются в рантайме и в БД, даже если не показаны в PDF.

## Таблицы в БД

### `documents/doc_link` — `ResourceType::DOC_LINK`

| колонка                   | тип       | назначение                                     |
|---------------------------|-----------|------------------------------------------------|
| `id`                      | PK        | серверный глобальный ID                        |
| `doc_link_target_type`    | INTEGER   | `DocLinkTargetType` — тип целевого ресурса     |
| + поля `ResourceAbstract` | …         | is_actual / created_at / updated_at / created_by / updated_by |

Обратная сторона связи — поле `doc_link_id` на стороне владельца
(Assembly, Part, Project, MaterialTemplateSimple,
MaterialTemplateComposite). У самого `doc_link` колонки `target_id`
нет: папка не знает, к кому именно она прикреплена.

### `documents/doc` — `ResourceType::DOC`

| колонка                   | тип              | назначение                                  |
|---------------------------|------------------|---------------------------------------------|
| `id`                      | PK               | серверный глобальный ID                     |
| `doc_link_id`             | FK               | ссылка на `doc_link.id` (1:M, только в БД)  |
| `doc_type`                | INTEGER          | `DocumentType`                              |
| `name`                    | TEXT             | человекочитаемое имя                        |
| `object_key`              | TEXT             | ключ объекта в MinIO                        |
| `sha256`                  | TEXT             | SHA-256 содержимого                         |
| `size_bytes`              | INTEGER          | размер файла                                |
| `mime_type`               | TEXT             | MIME-тип                                    |
| `description`             | TEXT             | комментарий                                 |
| `local_path`              | TEXT? (nullable) | путь в локальном кэше клиента               |
| + поля `ResourceAbstract` | …                | …                                           |

**Связочной таблицы между doc_link и doc нет.** Отношение 1:M
реализовано FK `doc_link_id` на стороне `doc`. Один и тот же файл
не может входить в две папки; если это нужно — создаётся ещё один
Doc с тем же `object_key`/`sha256`.

## Сворачивание таблиц → класс

**Чтение `DocLink`:**

```sql
SELECT * FROM doc_link WHERE id = :id;
SELECT * FROM doc      WHERE doc_link_id = :id AND is_actual = 1;
```

Две выборки объединяются: строки из `doc` складываются в
`DocLink::docs` в рантайме. FK `doc_link_id` из строк `doc` в
рантайм-Doc **не переносится** — он использован только для выборки.

**Чтение `Doc` напрямую:** обычный SELECT из `doc`. Привязка к
DocLink восстанавливается либо при чтении соответствующего DocLink,
либо — если нужно именно «к какому DocLink относится этот Doc» —
отдельной выборкой `SELECT doc_link_id FROM doc WHERE id = :id`.

## Раскладывание класс → таблицы

При изменении состава папки **не перезаписывается вся DocLink**.
Клиент шлёт отдельные CRUD-сообщения:

- добавить файл       → `CrudAction::CREATE` с `ResourceType::DOC`
                        (в полезной нагрузке — Doc + target doc_link_id)
- изменить метаданные → `CrudAction::UPDATE` с `ResourceType::DOC`
- убрать файл         → `CrudAction::DELETE` с `ResourceType::DOC`
- изменить тип цели   → `CrudAction::UPDATE` с `ResourceType::DOC_LINK`

Поскольку FK `doc_link_id` в рантайм-Doc отсутствует, при CREATE/
UPDATE сообщения Doc нужная привязка передаётся отдельным полем
UniterMessage (например, через `add_data["doc_link_id"]`) — это
учитывается при реализации сериализации подсистемы DOCUMENTS.

Это позволяет синхронизировать папки между клиентами по Kafka без
трансляции всей коллекции.

## Связь с uniterprotocol.h

```
ResourceType::DOC       = 90   (таблица doc)
ResourceType::DOC_LINK  = 91   (таблица doc_link)
```

Добавлений в `uniterprotocol.h` не требуется: обе таблицы уже имеют
собственные `ResourceType`, связочных таблиц в подсистеме нет.
