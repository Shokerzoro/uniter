#include "sqlitedatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>
#include <utility>

namespace uniter::data {

namespace {

database::Cell cellFromVariant(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return std::monostate{};
    }

    switch (value.typeId()) {
    case QMetaType::Bool:
        return static_cast<int64_t>(value.toBool() ? 1 : 0);
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return static_cast<int64_t>(value.toLongLong());
    case QMetaType::Float:
    case QMetaType::Double:
        return value.toDouble();
    case QMetaType::QByteArray: {
        const QByteArray bytes = value.toByteArray();
        return std::vector<uint8_t>(bytes.begin(), bytes.end());
    }
    default:
        return value.toString().toStdString();
    }
}

database::CellType cellTypeFromField(const QSqlField& field)
{
    switch (field.metaType().id()) {
    case QMetaType::Bool:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return database::CellType::Int64;
    case QMetaType::Float:
    case QMetaType::Double:
        return database::CellType::Double;
    case QMetaType::QByteArray:
        return database::CellType::Blob;
    default:
        return database::CellType::Text;
    }
}

} // namespace

SqliteDataBase::SqliteDataBase()
    : connectionName_(QStringLiteral("uniter_sqlite_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

SqliteDataBase::~SqliteDataBase()
{
    Close();
}

void SqliteDataBase::Open(const std::string& database_path)
{
    Close();

    databasePath_ = QString::fromStdString(database_path);
    const QFileInfo fileInfo(databasePath_);
    const QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        QDir().mkpath(dir.absolutePath());
    }

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        return;
    }

    execPragma(QStringLiteral("PRAGMA foreign_keys = ON;"));
    execPragma(QStringLiteral("PRAGMA journal_mode = WAL;"));
    execPragma(QStringLiteral("PRAGMA busy_timeout = 5000;"));
    ensureCurrentUserTable();
}

void SqliteDataBase::Close()
{
    if (db_.isValid()) {
        if (db_.isOpen()) {
            db_.close();
        }
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }
}

void SqliteDataBase::SetUserContext(const std::string& user_hash)
{
    userHash_ = QString::fromStdString(user_hash);
    if (!db_.isOpen()) {
        return;
    }

    if (!ensureCurrentUserTable()) {
        return;
    }

    QSqlQuery clear(db_);
    if (!clear.exec(QStringLiteral("DELETE FROM uniter_current_user;"))) {
        return;
    }

    QSqlQuery insert(db_);
    insert.prepare(QStringLiteral("INSERT INTO uniter_current_user(user_hash) VALUES(:user_hash);"));
    insert.bindValue(QStringLiteral(":user_hash"), userHash_);
    insert.exec();
}

database::SqlResult SqliteDataBase::ExecInstruction(const std::string& instruction)
{
    database::SqlResult result;

    if (!db_.isOpen()) {
        return makeErrorResult(QStringLiteral("SQLite database is not open"));
    }

    QSqlQuery query(db_);
    if (!query.exec(QString::fromStdString(instruction))) {
        return makeErrorResult(query.lastError().text());
    }

    result.success = true;

    const QSqlRecord record = query.record();
    result.col_num = record.count();
    result.cell_types.reserve(static_cast<size_t>(result.col_num));
    for (int i = 0; i < record.count(); ++i) {
        result.cell_types.push_back({record.fieldName(i).toStdString(), cellTypeFromField(record.field(i))});
    }

    while (query.next()) {
        database::Row row;
        row.reserve(static_cast<size_t>(record.count()));
        for (int i = 0; i < record.count(); ++i) {
            row.push_back(cellFromVariant(query.value(i)));
        }
        result.result.push_back(std::move(row));
    }

    result.row_num = static_cast<int>(result.result.size());
    return result;
}

void SqliteDataBase::BeginTransaction()
{
    QSqlQuery query(db_);
    query.exec(QStringLiteral("BEGIN IMMEDIATE;"));
}

void SqliteDataBase::CommitTransaction()
{
    QSqlQuery query(db_);
    query.exec(QStringLiteral("COMMIT;"));
}

void SqliteDataBase::RollbackTransaction()
{
    QSqlQuery query(db_);
    query.exec(QStringLiteral("ROLLBACK;"));
}

bool SqliteDataBase::execPragma(const QString& instruction)
{
    QSqlQuery query(db_);
    return query.exec(instruction);
}

bool SqliteDataBase::ensureCurrentUserTable()
{
    QSqlQuery query(db_);
    return query.exec(QStringLiteral(
        "CREATE TEMP TABLE IF NOT EXISTS uniter_current_user ("
        "user_hash TEXT NOT NULL"
        ");"));
}

database::SqlResult SqliteDataBase::makeErrorResult(const QString& message) const
{
    database::SqlResult result;
    result.success = false;
    result.error_code = -1;
    result.error_message = message.toStdString();
    return result;
}

} // namespace uniter::data
