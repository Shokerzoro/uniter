# PDM↔DESIGN verification

## Verdict

**MINOR-ISSUES** - one real incorrectness in the documentation (outdated comment in `pdmtypes.h`), one structural ambiguity (missing `snapshot_id` in C++ mirror classes), one incorrect link in the PDF schema (the project_id of `design_part` is not reflected in the PDF). There are no critical errors.

---

## Problems

### [MINOR] Deprecated comment in `pdmtypes.h`: refers to removed field `Project.active_snapshot_id`

- **Where:** `src/uniter/contract/pdm/pdmtypes.h`, line 12
- **Problem:** The comment for `SnapshotStatus::APPROVED` reads:
> `APPROVED - approved by the person in charge; Project.active_snapshot_id points here.`

The `active_snapshot_id` field has been removed from `design::Project` and is now replaced by `pdm_project_id`. Once replaced, this comment points to a non-existent field and is misleading when implementing the assertion loop.

- **Recommendation:** Replace with:
  ```
  APPROVED - approved by the responsible person; pdm_project.head_snapshot_id may
  point to this snapshot or a later one; the status is stored inside
pdm_snapshot.status. When implementing the approval cycle, consider
pdm_project.approved_snapshot_id (see pdm_design_logic.md §5).
  ```

---

### [MINOR] `snapshot_id` for PDM mirrors is not represented in C++ classes - there is no explicit transfer contract

- **Where:** `docs/db/pdm.md` line 46–53; `docs/db/pdm_design_logic.md` line 45–49; files `design/assembly.h`, `design/part.h`, `design/assemblyconfig.h`, `design/partconfig.h`
- **Problem:** The documentation says that PDM mirrors (`pdm_assembly`, etc.) "at the field level differ only in the addition of the FK `snapshot_id`". However, the classes `design::Assembly`, `design::Part`, `design::AssemblyConfig`, `design::PartConfig` do not contain this field - they are directly reused as PDM resources. The documentation itself marks this as a `TODO`:
> "`snapshot_id` - this field lives in the database table, in the C++ class it is passed through a separate DataManager context or, if necessary, through `std::optional<uint64_t> snapshot_id`"

This leaves an open question: how does PDMManager pass the `snapshot_id` of new pdm rows via UniterMessage when creating a snapshot? If the field is not in the class, there is no way to serialize it in a standard CRUD message. You will have to either add `snapshot_id` to the class itself (contaminating the DESIGN class with PDM semantics), or pass it through `add_data` messages (non-standard).

- **Recommendation:** Before implementing DataManager, make an explicit decision and document it in documentation: either introduce `std::optional<uint64_t> snapshot_id` into the base class (with NOTE that for DESIGN resources it is always NULL), or document the transfer convention via `add_data["snapshot_id"]` for `ResourceType::*_PDM`. The current `TODO` status is fine, but it must be resolved before writing the DataManager.

---

### [MINOR] `design/part` does not have `project_id` in PDF schema DESIGN-4.pdf - document/code discrepancy

- **Where:** `DESIGN-4.pdf` (table `design/part`); `src/uniter/contract/design/part.h` line 56; `docs/db/design.md` line 145
- **Problem:** In the PDF schema DESIGN-4.pdf, the `design/part` table contains the columns: `id (PK)`, `doc_link_id (FK) - mult`, `instance_composite_id (FK)`, `instance_simple_id (FK)`. There is no `project_id` column in PDF. However, `design.md` and `part.h` declare `project_id: INTEGER NOT NULL FK → design_project.id`. This is not a logic error (FK on project is needed to classify parts into projects and for correct loading with FULL_SYNC), but a discrepancy between the PDF diagram and the documentation/code.
- **Recommendation:** Add `project_id (FK)` to the PDF-scheme during the next revision of DESIGN-4.pdf, or make a note in `design.md` that the PDF is an incomplete scheme (shows only structurally significant FKs, omitting project-level classification FKs).

---

## Confirmed correct solutions

- **Snapshot invariance:** `pdm_snapshot.root_assembly_id` correctly points to `pdm_assembly.id` (not `design_assembly.id`) - confirmed in `snapshot.h:63`, `pdm.md:88–92`, `pdm_design_logic.md:32–33`. Any edits to `design_*` do not affect frozen copies.

- **Mechanism for creating a snapshot (§3 pdm_design_logic.md):** the sequence of steps 3.1 and 3.2 is logically correct. FK order: first the pdm lines are copied, then the `root_assembly_id` of the snapshot is added (step 4 for the copied id), then the `pdm_project` is updated. There is no race condition - the entire procedure is performed within one PDMManager transaction, there is no external visibility of intermediate states (mirrors are created only by PDMManager).

- **Doubly linked list snapshot/delta:** `prev_delta_id/next_delta_id` fields of `pdm_snapshot` and symmetric fields of `pdm_delta` are correctly documented as intentional denormalization. The update procedure when inserting a new delta (steps 3.2.5) is described in detail: `prev_head.next_snapshot_id`, `prev_head.next_delta_id`, `new_snapshot.prev_delta_id` are updated. The `Snapshot` and `Delta` classes in C++ have all the required fields.

- **Replacing `active_snapshot_id` with `pdm_project_id`:** is architecturally sound - eliminates duplication of “which snapshot is current” and simplifies DESIGN (the project does not know about specific snapshots). `design::Project` correctly contains only `pdm_project_id` (`project.h:60`). The “who asks, reads `pdm_project.head_snapshot_id`” route is documented in `pdm_design_logic.md §5`.

- **AssemblyConfig/PartConfig as separate resources (ResourceType=33/34):** justified - allows CRUD to be executed independently of the assembly/part itself. The PDF schema DESIGN-4.pdf shows `design_assembly_config` as a separate table with an FK `assembly_id`, which corresponds to a separate ResourceType. For `design_part_config` PDF shows a separate table with `part_id (FK)` - also correct.

- **Protocol: operator<<:** all ResourceTypes declared in enum (DEFAULT, EMPLOYEES, PRODUCTION, INTEGRATION, MATERIAL_TEMPLATE_SIMPLE/COMPOSITE, PROJECT, ASSEMBLY, PART, ASSEMBLY_CONFIG, PART_CONFIG, ASSEMBLY_PDM, ASSEMBLY_CONFIG_PDM, PART_PDM, PART_CONFIG_PDM, PURCHASE_GROUP, PURCHASE, SNAPSHOT, DELTA, PDM_PROJECT, MATERIAL_INSTANCE_SIMPLE/COMPOSITE, PRODUCTION_TASK/STOCK/SUPPLY, INTEGRATION_TASK, DOC, DOC_LINK) are all present in `operator<<`. There are no duplicates, no omissions.

- **Removing `MATERIAL_INSTANCE=60`:** The value 60 is not occupied by any new ResourceType. There is only one link left in the code - a comment in `instancebase.h:55`, which says `"MATERIAL_INSTANCE = 60 in ResourceType, so must be a full-fledged resource"`. This is an outdated historical commentary (describing the old pre-split design), but it doesn't cause compilation errors or logic collisions - it's just confusing. In fact, `InstanceBase` is still correctly a descendant of `ResourceAbstract`; ResourceType is now `MATERIAL_INSTANCE_SIMPLE=61` and `MATERIAL_INSTANCE_COMPOSITE=62`.

- **ESKD compliance standard_products = composite, materials = simple:** is correct. According to ESKD, standard products (bolts, bearings) are always identified by a two-part designation “name / GOST” - this is precisely `MaterialInstanceComposite`. Sheet/rod material - single-part designation (simple). Implemented in the join tables `design_assembly_config_standard_products` (FK → `material_instances_composite`) and `design_assembly_config_materials` (FK → `material_instances_simple`). Confirmed in `designtypes.h:88–125`.

- **Consistency of FK links:** all FKs mentioned in `design.md` and `pdm.md` are present in the corresponding C++ classes. There are no dangling links: `Project.root_assembly_id → design_assembly.id` ✓, `Assembly.project_id → design_project.id` ✓, `AssemblyConfig.assembly_id → design_assembly.id` ✓, `Part.project_id → design_project.id` ✓, `PartConfig.part_id → design_part.id` ✓, `PdmProject.base/head_snapshot_id → pdm_snapshot.id` ✓, `Snapshot.root_assembly_id → pdm_assembly.id` (explicitly documented) ✓, `Delta.prev/next_snapshot_id → pdm_snapshot.id` ✓.

---

## TODO items to note in documentation

- **`instancebase.h:55`** - deprecated comment `"MATERIAL_INSTANCE = 60 in ResourceType"`. Update to `"MATERIAL_INSTANCE_SIMPLE = 61 / MATERIAL_INSTANCE_COMPOSITE = 62"`.

- **`pdm.md` and `pdm_design_logic.md`** - TODO about `snapshot_id` in PDM mirrors should be resolved before the DataManager implementation begins, and not during it. It is recommended to introduce the `add_data["snapshot_id"]` convention or an explicit field in the class and record the selection.

- **`pdm_design_logic.md §3.2, step 4 (delta):** `prev_delta_id` of the new delta** - described in the text as "points to the previous delta in the chain, if one exists." This means that when creating the second snapshot (the first delta) `new_delta.prev_delta_id = NULL`. When creating the third snapshot (second delta) `new_delta.prev_delta_id = delta_1.id`. The logic is correct, but in the text of §3.2, step 4, the paragraph about `prev_delta_id` is somewhat confusing due to the reference to the “previous delta in the chain” at the same time as the description of the operation of creating the first delta. It is recommended to clarify: “`new_delta.prev_delta_id = prev_head.prev_delta_id` (`NULL` if this is the first delta of the project).”

- **`docs/db/pdm.md` (snapshot status):** TODO on `version`, `status`, `approved_by`, `approved_at` is marked correctly. When implementing the workflow, you should also determine whether `pdm_project.approved_snapshot_id` is needed separately from `head_snapshot_id` to distinguish between “last created” and “last approved”.
