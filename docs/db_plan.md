# План архитектуры слоя данных и БД

Документ фиксирует целевое направление работ по `DataManager`, `IDataBase`, SQLite-реализации, executors, SQL-файлам и CodeGen. Цель ближайшего этапа - сначала выстроить интерфейсы и границы ответственности, чтобы после реализации executor хотя бы одной подсистемы его можно было тестировать независимо от UI и AppManager.

## Верхнеуровневая ответственность

### DataManager

`DataManager` - компонент приложения, который принадлежит клиентской части и связывает AppManager, сетевой слой, БД и UI.

Он должен:

1. Инициализироваться для конкретного пользователя.
   - Принимать `userHash` от AppManager.
   - Открывать локальный файл БД, привязанный к этому пользователю.
   - Не смешивать данные разных пользователей.
   - Инициализировать таблицы через executors подсистем.
   - Поднимать executors подсистем.

2. Удалять локальные данные по требованию AppManager.
   - Очищать ресурсные таблицы перед `FULL_SYNC` или при сбросе сессии.
   - Сохранять структуру БД, служебные таблицы и домены enum-ов, если это выбранная политика.
   - Сообщать AppManager результат через `signalDatabaseCleared`.

3. Работать с данными через CRUD по всем ресурсам.
   - Разбирать входящий `UniterMessage`.
   - Маршрутизировать его по `Subsystem` / `ResourceType` в нужный executor.
   - Выполнять CUD из Kafka/server connector.
   - Выполнять READ по запросам UI.
   - После изменения данных уведомлять подписчиков.

4. Предоставлять API для UI.
   - Подписка на конкретный ресурс.
   - Подписка на список ресурсов.
   - Иерархическую подписку пока убрать из обязательного API, пока нет стабильной модели деревьев.

### IDataBase

`IDataBase` - абстракция конкретного движка БД и дескриптор открытого подключения. Она не знает бизнес-ресурсы, не владеет схемой подсистем и не принимает на себя ответственность за DDL конкретных подсистем.

Она должна:

- открыть/закрыть соединение;
- выбрать физическое хранилище для конкретного пользователя;
- выполнить SQL-инструкцию и вернуть унифицированный `SqlResult`;
- управлять транзакциями;
- предоставить минимальные engine-specific сервисы, которые нужны всем executors.

`IDataBase` должна быть владельцем низкоуровневого доступа к движку: SQLite, PostgreSQL, тестовая in-memory реализация.

### Executors

Executors - слой управления данными конкретной подсистемы. Они не открывают БД, но владеют SQL своей подсистемы: и DDL, и DML.

Они:

- получают ссылку на `IDataBase`;
- обеспечивают создание/проверку таблиц своей подсистемы;
- обеспечивают очистку данных своей подсистемы;
- используют DDL-инструкции своей подсистемы;
- используют DML-инструкции;
- маппят `contract::*` ресурсы в SQL;
- маппят `SqlResult` обратно в `contract::*` ресурсы;
- инкапсулируют правила конкретной подсистемы.

Пример: `ManagerExecutor` отвечает за `Employee`, `Plant`, `Integration`, назначения и права manager-подсистемы.

## Разделение DDL и DML

Нужно жестко разделить:

- DDL - управление структурой БД: `CREATE TABLE`, `CREATE INDEX`, `DROP TABLE`, `ALTER TABLE`, домены, миграции.
- DML - операции с данными: `INSERT`, `SELECT`, `UPDATE`, soft delete, join-запросы.

DDL используется executor-ом той подсистемы, которой принадлежат таблицы. Это сохраняет локальность: структура таблиц, CRUD-инструкции и C++-маппинг одной подсистемы живут рядом и изменяются вместе.

DML используется executors.

## Чего не хватает в IDataBase

Текущий `IDataBase` умеет только `Open()`, `Close()`, приватный `ExecInstruction()` и базовые транзакционные hooks. Этого недостаточно для пунктов 1-2 DataManager, но расширять его до владельца DDL подсистем не нужно.

Минимально нужен такой уровень интерфейса:

```cpp
struct DataBaseConfig {
    std::string baseDirectory;
    std::string userHash;
    std::string databaseName;
    bool createIfMissing = true;
};

enum class DataBaseOpenStatus {
    Opened,
    Created,
    Error
};

struct DataBaseStatus {
    bool ok = false;
    std::string message;
};
```

В `IDataBase`:

```cpp
virtual DataBaseStatus Open(const DataBaseConfig& config) = 0;
virtual void Close() = 0;
virtual bool IsOpen() const = 0;

virtual DataBaseStatus InitializeStorage() = 0;
virtual DataBaseStatus DropAllData() = 0;

virtual DataBaseStatus CheckTableExists(const std::string& tableName) = 0;

virtual SqlResult Exec(const std::string& instruction) = 0;
```

Важно: `InitializeStorage()` здесь означает подготовку физического хранилища, а не создание таблиц подсистем. Для SQLite это создание каталога/файла, открытие соединения и engine pragmas. Создание таблиц выполняют executors.

`CheckTableExists()` допустим как низкоуровневая engine-specific проверка, но решение, какие таблицы обязательны, остается в executor-е подсистемы.

Практичный вариант доступа:

- `IResExecutor` использует protected/static доступ к `Exec`.
- `Transaction` остается friend и вызывает `BeginTransaction`, `CommitTransaction`, `RollbackTransaction`.

## Чего не хватает в SqliteDataBase

SQLite-реализация должна покрыть пункты 1-2 DataManager.

Нужно реализовать:

1. Формирование пути к файлу БД.
   - На вход приходит `userHash`.
   - Файл должен иметь предсказуемое имя, например `uniter_<userHash>.sqlite`.
   - Каталог должен создаваться, если его нет.
   - Нужна валидация `userHash`, чтобы он не мог стать путем.

2. Открытие соединения.
   - Открыть SQLite database file.
   - Включить `PRAGMA foreign_keys = ON`.
   - Настроить разумные pragmas: например `journal_mode = WAL`, `busy_timeout`.

3. Служебная подготовка движка.
   - Создать файл БД и каталог, если их нет.
   - Включить обязательные SQLite pragmas.
   - Подготовить соединение к выполнению SQL.

4. Проверка схемы.
   - Предоставить primitive `CheckTableExists`.
   - Для SQLite выполнить `PRAGMA foreign_key_check`, если executor или DataManager запрашивает общую проверку.
   - Не хранить в `IDataBase` список обязательных таблиц подсистем.

5. Очистка данных.
   - Полная очистка файла/хранилища может быть engine-level операцией.
   - Очистка ресурсных таблиц по подсистемам должна выполняться executors, потому что только они знают порядок удаления с учетом FK.

6. Транзакции.
   - `BeginTransaction()` должен реально выполнять `BEGIN IMMEDIATE` или `BEGIN`.
   - `CommitTransaction()` должен выполнять `COMMIT`.
   - `RollbackTransaction()` должен выполнять `ROLLBACK`.
   - Мьютекс сам по себе не является транзакцией.

7. Ошибки.
   - `SqlResult` должен нести ошибку: message/code.
   - Сейчас `SqlResult` имеет `success`, но не имеет текста ошибки.

## DataManager после подготовки интерфейсов

После появления `IDataBase` и хотя бы одного executor DataManager должен получить внутренние поля:

```cpp
std::unique_ptr<database::IDataBase> db_;
std::unordered_map<contract::Subsystem, std::unique_ptr<database::IResExecutor>> executors_;
```

Инициализация DataManager:

1. Создать `SqliteDataBase`.
2. Вызвать `Open(config)` с `userHash`.
3. Создать/зарегистрировать executors.
4. Вызвать DDL-инициализацию каждого executor-а.
5. Перейти в состояние `LOADED`.

Очистка данных:

1. Открыть транзакцию.
2. Вызвать очистку данных каждого executor-а в согласованном порядке.
3. Зафиксировать транзакцию.
4. Уведомить AppManager через `signalDatabaseCleared`.

Подписки лучше хранить не тремя плоскими списками, а индексами:

```cpp
ResourceKey {
    Subsystem subsystem;
    GenSubsystem genSubsystem;
    std::optional<uint64_t> genId;
    ResourceType resourceType;
    std::optional<uint64_t> resourceId;
}
```

И отдельно:

- подписки на конкретный ресурс;
- подписки на список ресурсов конкретного типа.

Иерархическую подписку пока не реализовывать как обязательную часть API. Ее лучше вернуть после появления стабильной модели деревьев.

## CodeGen

CodeGen нужен после стабилизации интерфейсов `IDataBase`, executor-ов и правил разметки SQL-файлов.

Минимальный первый этап:

1. Enum pairs.
   - Обрабатывать `.h` / `.hpp` рекурсивно.
   - Искать `enum class` с маркером CodeGen.
   - Генерировать `inline constexpr std::array<std::pair<int, std::string_view>, N>`.
   - Использовать begin/end маркеры generated-блока, чтобы надежно перегенерировать.

2. SQL constants.
   - На вход: raw SQL-каталог подсистемы.
   - На выход: generated SQL-каталог той же подсистемы.
   - Каждая SQL-инструкция имеет имя.
   - Placeholder-ы лучше писать явно как `:param_name`, а не заменять произвольные значения из комментария.
   - Генератор заменяет `:param_name` на `%VAL%` и может генерировать массив имен параметров.

## Порядок работ

1. [x] Утвердить и локально разложить структуру каталогов.
2. Утвердить минимальный интерфейс `IDataBase`.
3. Добавить `SqliteDataBase` с открытием пользовательского файла, транзакциями и базовой проверкой схемы.
4. Уточнить базовый интерфейс executor-а: DDL-инициализация, очистка данных, CRUD-маршрутизация.
5. Перенести DDL/DML SQL в разделенные `.sql` файлы внутри подсистем.
6. Реализовать `DataManager` на уровне инициализации, очистки и маршрутизации в executor.
7. Реализовать `ManagerExecutor` как первый рабочий executor.
8. После этого внедрять CodeGen и DataGrip-процесс.
