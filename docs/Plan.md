# ODB-First Implementation Plan

## Goal

Build the shared persistence layer as an ODB-backed API that can be used by both client and server.
The first deliverable is one working executor/repository class for one subsystem.
After that, the same pattern is replicated across the rest of the subsystems.

## Phase 0. Add ODB in DevKit

## Phase 00. Add ODB in CMakeList of Uniter  project

## Phase 000. Remove Metadata Only constructors

## Phase 1. Canonical shared model

- Keep `src/uniter/contract/` and `src/uniter/database/` shared between client and server.
- Use only standard C++ types in the shared layer.
- Do not leak Qt into the shared contract or persistence path.
- Make the contract layer the source of truth for ODB annotations and persistence mapping.

## Phase 2. First ODB-backed executor

- Implement the first executor/repository class for one subsystem, starting with `DESIGN`.
- The first target should be one representative resource class, preferably `Project`, because it proves:
  - insert with known PK
  - insert without PK
  - read by id
  - update
  - delete
  - filtered listing
- Place the implementation under `src/uniter/database/design/`.
- Make it use ODB-generated code instead of hand-written SQL mapper logic.
- The class must expose at minimum:
  - `create(resource_without_pk)`
  - `create_with_pk(resource_with_known_pk)`
  - `get(id)`
  - `exists(id)`
  - `update(resource)`
  - `remove(id)`
  - `list(...)` / `find(...)`

## Phase 3. Database descriptor and backend binding

- Every executor must receive a database descriptor for the concrete backend.
- The descriptor must carry:
  - backend kind: SQLite or PostgreSQL
  - path or connection string
  - schema / tenant context
- Keep the architecture identical to the previous executor model:
  - the executor is the business-facing database facade
  - the descriptor decides where the executor works
- This must allow the same executor class to run on:
  - local SQLite
  - server PostgreSQL

## Phase 4. Schema bootstrap and cleanup

- Extend `DataManager` so initialization checks the schema before declaring resources loaded.
- Required init behavior:
  - open the concrete database descriptor
  - verify that required tables exist
  - create missing tables
  - validate basic structural compatibility
  - report success only after the schema is ready
- Add a full-clear operation for all resource tables.
- Decide and document the backend strategy:
  - table-by-table cleanup, or
  - schema drop/recreate for local storage
- The clear operation must be owned by the persistence layer, not by UI code.

## Phase 5. Resource and subsystem keys

- Introduce `SubsystemKey`.
- Introduce `ResourceKey`.
- Use these keys as the basis for DataManager subscriptions and UI notifications.
- Ensure the key model is stable enough for client-server reuse.

## Phase 6. Replicate across subsystems

- After the first executor works, replicate the pattern for the remaining subsystems.
- Keep the same API shape for every subsystem executor.
- Preserve the same contract/database ownership split:
  - contract defines the resource
  - database defines how it is stored

## Phase 7. Server tenancy

- Add a database-context abstraction for company/user isolation on the server.
- Prefer per-schema/company isolation for PostgreSQL.
- Keep the resource model unchanged.
- Do not make the resource classes aware of tenancy internals.

## Acceptance Criteria

- The first ODB-backed executor can create, read, update, delete, and list its resources.
- The executor works through a database descriptor.
- The same code path can be configured for SQLite and PostgreSQL.
- `DataManager` can bootstrap schema and clear all tables.
- `SubsystemKey` and `ResourceKey` exist and can drive subscriptions.
- The documentation in `docs/Documentation.md` reflects the final direction.

## Assumptions

- The first pilot subsystem is `DESIGN`.
- The pilot resource is `Project`.
- The shared model is std-only.
- ODB is the long-term persistence mechanism, not a temporary side path.

