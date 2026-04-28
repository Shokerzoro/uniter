# docs/db - notes on database design

This directory contains notes on how the subsystem classes from `src/uniter/contract/`
mapped to relational database tables. Files here - **reference for the future
SQLite/server database schema design** rather than auto-generated documentation.

## Key principle

**Class in runtime ≠ table in the database.**

We design resource classes with a relational database in mind, but we don't try
do them one-on-one with tables. For ease of use in the application, resources
have a “collapsed” appearance: for example, `TemplateSimple` contains a vector at runtime
`SegmentDefinition`, although in the database these are separate tables `material/segment` and
`material/segment_value`, connected by FK.

When reading from a database, DataManager **collapses** several related tables into one
resource. When recording, it folds back.

Each subsystem note in this directory describes:

1. List of classes in the subsystem and their runtime composition (what fields and vectors).
2. List of tables on the database side (including link tables - M:N).
3. The “collapse” rule - how tables are collected into one class when reading.
4. The “folding out” rule - how the class is divided into tables when recording.
5. Match the `ResourceType` in `uniterprotocol.h` to each table so that
DataManager could perform dot CRUD against linked tables over the protocol.

## Relationship

If the class has `std::vector<Child>`:

- **1:M** — `Child` has an FK for its parent; There is no separate link table.
Example: `DocLink → vector<Doc>`, where `Doc` has `doc_link_id`.
- **M:N** - there is a separate link table with two FKs. It needs to be registered
a separate `ResourceType` to manage communications via CRUD messages.
Example: Simple Material Template Compatibility
  (`TemplateSimple.compatible_template_ids`).

For DataManager to work correctly, each linked table must
match a separate `ResourceType` (or at least a separate logical
CRUD unit) - otherwise you cannot “link/unlink” without overwriting everything
parent resource.

## List of subsystems

| Subsystem | Note |
|---------------|----------------------------------------|
| DOCUMENTS     | [documents.md](./documents.md)         |
| MATERIALS / INSTANCES | [material_instance.md](./material_instance.md) |
| MANAGER       | [manager.md](./manager.md)             |
| DESIGN        | _TBD_                                  |
| PDM           | _TBD_                                  |
| GEN::PRODUCTION | _TBD_                                |
| GEN::INTEGRATION | _TBD_                               |
