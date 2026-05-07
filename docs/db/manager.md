# Subsystem::MANAGER — заметка по БД

Схема-эталон: `MANAGER-2.pdf` (концептуальная схема, показывает только
PK/FK и основные столбцы).

## Классы в рантайме

- `manager::Employee` — сотрудник / пользователь. Содержит личные
  данные (name / surname / patronymic / email) и свёрнутый список
  `assignments: std::vector<EmployeeAssignment>`.
- `manager::EmployeeAssignment` — назначение на подсистему: `subsystem`,
  `genSubsystem`, `genId`, плюс свёрнутый список прав `permissions:
  std::vector<uint8_t>`.
- `manager::Plant` — производственная площадка (без изменений).
- `manager::Integration` — внешняя интеграция (без изменений).

Никаких связочных классов (`EmployeePermission`, `EmployeeAssignmentLink`,
`Permission`) в рантайме нет — всё свёрнуто внутрь Employee.

> PDF моделирует только концептуальную схему. Личные данные
> (name / surname / patronymic / email) и внутренние поля Plant /
> Integration сохраняются в рантайме и в БД, даже если не показаны
> в PDF.

## Таблицы в БД

### `manager/employee` — `ResourceType::EMPLOYEES` (10)

| колонка                   | тип   | назначение                                   |
|---------------------------|-------|----------------------------------------------|
| `id`                      | PK    |                                              |
| `name`                    | TEXT  |                                              |
| `surname`                 | TEXT  |                                              |
| `patronymic`              | TEXT  |                                              |
| `email`                   | TEXT  | уникальный логин                             |
| + поля `ResourceAbstract` | …     | `created_by` / `updated_by` — FK → `employee.id` (self-reference) |

### `manager/employee_assignment` — `ResourceType::EMPLOYEE_ASSIGNMENT`

| колонка                   | тип     | назначение                                   |
|---------------------------|---------|----------------------------------------------|
| `id`                      | PK      |                                              |
| `subsystem`               | INT     | `contract::Subsystem`                        |
| `gensubsystem`            | INT     | `contract::GenSubsystemType` (`NOTGEN` / `PRODUCTION` / `INTERGATION`) |
| `gensubsystem_id`         | BIGINT? | id генеративной подсистемы (NULL, если `gensubsystem = NOTGEN`) |
| + поля `ResourceAbstract` | …       |                                              |

### `manager/permissions` — `ResourceType::PERMISSION`

| колонка                   | тип   | назначение                                        |
|---------------------------|-------|---------------------------------------------------|
| `id`                      | PK    |                                                   |
| `assignment_id`           | FK    | → `employee_assignment.id`                        |
| `permission`              | INT   | значение соответствующего `*Permission` enum      |
| + поля `ResourceAbstract` | …     |                                                   |

Связь `employee_assignment` → `permissions` — 1:M (одно назначение даёт
набор разрешений). FK `assignment_id` в БД хранится на стороне
`permissions`, в рантайм `EmployeeAssignment::permissions`
переносится только значение `permission`.

### `manager/employee_assignment_link` (junction M:N)

| колонка                   | тип   | назначение                                        |
|---------------------------|-------|---------------------------------------------------|
| `id`                      | PK    |                                                   |
| `employee_id`             | FK    | → `employee.id`                                   |
| `assignment_id`           | FK    | → `employee_assignment.id`                        |
| + поля `ResourceAbstract` | …     |                                                   |

Связочная таблица между сотрудником и его назначениями. В рантайме
собственного класса нет: при чтении Employee его assignments
подтягиваются join-ом (см. ниже). Однако на уровне протокола ей
полезно иметь `ResourceType` — операции «назначить существующего
сотрудника на существующий assignment» удобно выполнять
CRUD-сообщениями на junction, не трогая сам assignment.

### `manager/plant` — `ResourceType::PRODUCTION` (11)

Без изменений — см. `src/uniter/contract/manager/plant.h`.

### `manager/integration` — `ResourceType::INTEGRATION` (12)

Без изменений — см. `src/uniter/contract/manager/integration.h`.

## Сворачивание таблиц → классы

**Чтение `Employee` целиком:**

```sql
-- сам сотрудник
SELECT * FROM manager/employee WHERE id = :id;

-- все assignment-ы сотрудника через junction
SELECT ea.*
  FROM manager/employee_assignment_link link
  JOIN manager/employee_assignment ea ON ea.id = link.assignment_id
  WHERE link.employee_id = :id AND ea.is_actual = 1;

-- permissions по каждому assignment
SELECT assignment_id, permission
  FROM manager/permissions
  WHERE assignment_id IN (:assignment_ids) AND is_actual = 1;
```

Свёртка:
- каждая строка `employee_assignment` → объект `EmployeeAssignment`
  с полями `subsystem`, `genSubsystem`, `genId`;
- строки `permissions` с совпадающим `assignment_id` кладутся в
  `EmployeeAssignment::permissions` как `uint8_t` (одно значение
  `permission` на строку);
- все `EmployeeAssignment` → в `Employee::assignments`.

FK (`permissions.assignment_id`, `employee_assignment_link.employee_id`,
`employee_assignment_link.assignment_id`) в рантайм-классы **не
переносятся** — они использованы только для выборки.

## Раскладывание класс → таблицы

CRUD распределяется по ресурсам:

- личные данные сотрудника
  → `CrudAction::UPDATE` с `ResourceType::EMPLOYEES`
    (меняются только колонки таблицы `employee`);
- создать / изменить / удалить назначение
  → CRUD с `ResourceType::EMPLOYEE_ASSIGNMENT`
    (таблица `employee_assignment`);
- выдать / отозвать конкретное право
  → CRUD с `ResourceType::PERMISSION`
    (строка `permissions`, assignment_id в полезной нагрузке);
- назначить существующего сотрудника на существующий assignment
  → CRUD на junction `manager/employee_assignment_link`
    (если понадобится — см. раздел «Ресурсы в uniterprotocol.h»).

Такая декомпозиция позволяет выдать одному сотруднику право на
материалы, не переотправляя весь Employee через Kafka.

## Удалённое дублирование

Ранее `manager/permissions.h` определял локальный `enum class
Subsystem`, дублирующий `uniter::contract::Subsystem` из
`uniterprotocol.h`. Дубликат **удалён**: `EmployeeAssignment::subsystem`
напрямую использует `contract::Subsystem` — единая точка истины.

Проверка внешних потребителей (grep по `src/`): ни
`manager::Subsystem`, ни `employees::Subsystem` нигде не
использовались.

## Связь с uniterprotocol.h

Существующие:
```
ResourceType::EMPLOYEES   = 10   (таблица manager/employee)
ResourceType::PRODUCTION  = 11   (таблица manager/plant)
ResourceType::INTEGRATION = 12   (таблица manager/integration)
```

Добавить (см. задачу по uniterprotocol.h):
```
ResourceType::EMPLOYEE_ASSIGNMENT       — manager/employee_assignment
ResourceType::PERMISSION                — manager/permissions
ResourceType::EMPLOYEE_ASSIGNMENT_LINK  — manager/employee_assignment_link
                                          (junction, CRUD на сам факт назначения)
```

Подсистема в `Subsystem` (уже есть): `MANAGER`.
