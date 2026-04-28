# Subsystem::DOCUMENTS - note on the database

Reference diagram: `DOCUMENTS.pdf` (conceptual diagram, shows only
PK/FK and main columns).

## Classes in runtime

- `documents::DocLink` - “folder” of documents attached to one
target resource (Assembly, Part, Project, MaterialTemplate*).
Stores **only** its `id`, `DocLinkTargetType` and collapsed
`std::vector<Doc> docs`. No link fields (target_id, role,
position, doc_id) is not in the runtime class.
- `documents::Doc` - one file in MinIO. Stores **only** your data
  (object_key, sha256, doc_type, name, size_bytes, mime_type,
description, optional local_path). No FK/links to DocLink
There is no runtime class - the link is visible from the outside as `DocLink::docs`.

> Important: PDF only models the conceptual schema of the database. Other fields
> classes (name, size_bytes, mime_type, description, etc.)
> are saved in runtime and in the database, even if not shown in the PDF.

## Tables in the database

### `documents/doc_link` — `ResourceType::DOC_LINK`

| column | type | appointment |
|---------------------------|-----------|------------------------------------------------|
| `id` | PK | server global ID |
| `doc_link_target_type` | INTEGER | `DocLinkTargetType` - target resource type |
| + fields `ResourceAbstract` | ... | is_actual/created_at/updated_at/created_by/updated_by |

The reverse side of the connection is the `doc_link_id` field on the owner’s side
(Assembly, Part, Project, MaterialTemplateSimple,
MaterialTemplateComposite). The `doc_link` itself has a `target_id` column
no: the folder does not know who exactly it is attached to.

### `documents/doc` — `ResourceType::DOC`

| column | type | appointment |
|---------------------------|------------------|---------------------------------------------|
| `id` | PK | server global ID |
| `doc_link_id` | FK | link to `doc_link.id` (1:M, only in the database) |
| `doc_type`                | INTEGER          | `DocumentType`                              |
| `name` | TEXT | human readable name |
| `object_key` | TEXT | object key in MinIO |
| `sha256` | TEXT | SHA-256 content |
| `size_bytes` | INTEGER | file size |
| `mime_type` | TEXT | MIME type |
| `description` | TEXT | comment |
| `local_path` | TEXT? (nullable) | path in the client's local cache |
| + fields `ResourceAbstract` | ... | ... |

**There is no link table between doc_link and doc.** ​​Relationship 1:M
implemented FK `doc_link_id` on the `doc` side. Same file
cannot be in two folders; if this is needed, another one is created
Doc with the same `object_key`/`sha256`.

## Collapse tables → class

**Reading `DocLink`:**

```sql
SELECT * FROM doc_link WHERE id = :id;
SELECT * FROM doc      WHERE doc_link_id = :id AND is_actual = 1;
```

The two selections are combined: the lines from `doc` are added to
`DocLink::docs` at runtime. FK `doc_link_id` from `doc` strings
runtime-Doc **is not transferred** - it is used only for sampling.

**Reading `Doc` directly:** normal SELECT from `doc`. Link to
DocLink is restored either by reading the corresponding DocLink,
or - if you need exactly “which DocLink this Doc belongs to” -
a separate selection `SELECT doc_link_id FROM doc WHERE id = :id`.

## Layout class → tables

When you change the composition of the folder **the entire DocLink** is not overwritten.
The client sends separate CRUD messages:

- add file → `CrudAction::CREATE` with `ResourceType::DOC`
(in payload - Doc + target doc_link_id)
- change metadata → `CrudAction::UPDATE` from `ResourceType::DOC`
- remove file → `CrudAction::DELETE` from `ResourceType::DOC`
- change target type → `CrudAction::UPDATE` from `ResourceType::DOC_LINK`

Since FK `doc_link_id` is missing in runtime-Doc, when CREATE/
UPDATE message Doc the required binding is transmitted as a separate field
UniterMessage (for example, via `add_data["doc_link_id"]`) is
taken into account when implementing serialization of the DOCUMENTS subsystem.

This allows you to synchronize folders between clients via Kafka without
broadcasts of the entire collection.

## Communication with uniterprotocol.h

```
ResourceType::DOC       = 90   (doc table)
ResourceType::DOC_LINK  = 91   (doc_link table)
```

No additions to `uniterprotocol.h` are required: both tables already have
own `ResourceType`, there are no linked tables in the subsystem.
