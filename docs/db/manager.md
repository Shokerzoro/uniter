# Subsystem::MANAGER - note on the database

Reference diagram: `MANAGER-2.pdf` (conceptual diagram, shows only
PK/FK and main columns).

## Classes in runtime

- `manager::Employee` - employee / user. Contains personal
data (name / surname / patronymic / email) and collapsed list
  `assignments: std::vector<EmployeeAssignment>`.
- `manager::EmployeeAssignment` — assignment to the subsystem: `subsystem`,
`genSubsystem`, `genId`, plus a collapsed list of rights `permissions:
  std::vector<uint8_t>`.
- `manager::Plant` — production site (no changes).
- `manager::Integration` - external integration (no changes).

No link classes (`EmployeePermission`, `EmployeeAssignmentLink`,
There is no `Permission`) in runtime - everything is folded inside Employee.

> PDF models only the conceptual diagram. Personal information
> (name / surname / patronymic / email) and internal fields Plant /
> Integration are saved in runtime and in the database, even if not shown
> in PDF.

## Tables in the database

### `manager/employee` — `ResourceType::EMPLOYEES` (10)

| column | type | appointment |
|---------------------------|-------|----------------------------------------------|
| `id`                      | PK    |                                              |
| `name`                    | TEXT  |                                              |
| `surname`                 | TEXT  |                                              |
| `patronymic`              | TEXT  |                                              |
| `email` | TEXT | unique login |
| + fields `ResourceAbstract` | ... | `created_by` / `updated_by` - FK → `employee.id` (self-reference) |

### `manager/employee_assignment` — `ResourceType::EMPLOYEE_ASSIGNMENT`

| column | type | appointment |
|---------------------------|---------|----------------------------------------------|
| `id`                      | PK      |                                              |
| `subsystem`               | INT     | `contract::Subsystem`                        |
| `gensubsystem`            | INT     | `contract::GenSubsystem` (`NOTGEN` / `PRODUCTION` / `INTERGATION`) |
| `gensubsystem_id` | INTEGER? | id of the generative subsystem (NULL if `gensubsystem = NOTGEN`) |
| + fields `ResourceAbstract` | ... |                                              |

### `manager/permissions` — `ResourceType::PERMISSION`

| column | type | appointment |
|---------------------------|-------|---------------------------------------------------|
| `id`                      | PK    |                                                   |
| `assignment_id`           | FK    | → `employee_assignment.id`                        |
| `permission` | INT | value of the corresponding `*Permission` enum |
| + fields `ResourceAbstract` | ... |                                                   |

The relationship `employee_assignment` → `permissions` is 1:M (one assignment gives
permission set). FK `assignment_id` in the database is stored on the side
`permissions`, at runtime `EmployeeAssignment::permissions`
Only the `permission` value is carried over.

### `manager/employee_assignment_link` (junction M:N)

| column | type | appointment |
|---------------------------|-------|---------------------------------------------------|
| `id`                      | PK    |                                                   |
| `employee_id`             | FK    | → `employee.id`                                   |
| `assignment_id`           | FK    | → `employee_assignment.id`                        |
| + fields `ResourceAbstract` | ... |                                                   |

Link table between an employee and his assignments. At runtime
there is no own class: when reading Employee its assignments
are pulled up by join (see below). However, at the protocol level it
useful to have `ResourceType` - "assign existing" operations
employee for an existing assignment" is convenient to perform
CRUD messages to the junction, without touching the assignment itself.

### `manager/plant` — `ResourceType::PRODUCTION` (11)

No changes - see `src/uniter/contract/manager/plant.h`.

### `manager/integration` — `ResourceType::INTEGRATION` (12)

No changes - see `src/uniter/contract/manager/integration.h`.

## Collapse tables → classes

**Reading `Employee` in its entirety:**

```sql
-- the employee himself
SELECT * FROM manager/employee WHERE id = :id;

-- all employee assignments via junction
SELECT ea.*
  FROM manager/employee_assignment_link link
  JOIN manager/employee_assignment ea ON ea.id = link.assignment_id
  WHERE link.employee_id = :id AND ea.is_actual = 1;

-- permissions for each assignment
SELECT assignment_id, permission
  FROM manager/permissions
  WHERE assignment_id IN (:assignment_ids) AND is_actual = 1;
```

Convolution:
- each `employee_assignment` line → `EmployeeAssignment` object
with fields `subsystem`, `genSubsystem`, `genId`;
- `permissions` lines with matching `assignment_id` are placed in
`EmployeeAssignment::permissions` as `uint8_t` (single value
`permission` per line);
- all `EmployeeAssignment` → in `Employee::assignments`.

FK (`permissions.assignment_id`, `employee_assignment_link.employee_id`,
`employee_assignment_link.assignment_id`) to runtime classes **not
carried over** - they are used for sampling only.

## Layout class → tables

CRUD is distributed across resources:

- personal data of the employee
→ `CrudAction::UPDATE` with `ResourceType::EMPLOYEES`
(only the columns of the `employee` table change);
- create / change / delete assignment
→ CRUD with `ResourceType::EMPLOYEE_ASSIGNMENT`
(table `employee_assignment`);
- issue/revoke a specific right
→ CRUD with `ResourceType::PERMISSION`
(string `permissions`, assignment_id in payload);
- assign an existing employee to an existing assignment
→ CRUD at junction `manager/employee_assignment_link`
(if needed, see the “Resources in uniterprotocol.h” section).

This decomposition allows one employee to be given the right to
materials without re-sending the entire Employee through Kafka.

## Remote duplication

Previously `manager/permissions.h` defined a local `enum class
Subsystem`, duplicating `unit::contract::Subsystem` from
`uniterprotocol.h`. Duplicate **removed**: `EmployeeAssignment::subsystem`
directly uses `contract::Subsystem` - a single point of truth.

Checking external consumers (grep by `src/`): neither
`manager::Subsystem`, nor `employees::Subsystem` anywhere
were used.

## Communication with uniterprotocol.h

Existing:
```
ResourceType::EMPLOYEES   = 10   (manager/employee table)
ResourceType::PRODUCTION  = 11   (manager/plant table)
ResourceType::INTEGRATION = 12   (manager/integration table)
```

Add (see task on uniterprotocol.h):
```
ResourceType::EMPLOYEE_ASSIGNMENT       — manager/employee_assignment
ResourceType::PERMISSION                — manager/permissions
ResourceType::EMPLOYEE_ASSIGNMENT_LINK  — manager/employee_assignment_link
(junction, CRUD on the very fact of assignment)
```

Subsystem in `Subsystem` (already exists): `MANAGER`.
