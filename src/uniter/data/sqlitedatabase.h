#ifndef UNITER_DATA_SQLITEDATABASE_H
#define UNITER_DATA_SQLITEDATABASE_H

#include "../database/idatabase.h"
#include <QSqlDatabase>
#include <QString>
#include <string>

namespace uniter::data {

class SqliteDataBase final : public database::IDataBase
{
public:
    SqliteDataBase();
    ~SqliteDataBase() override;

    SqliteDataBase(const SqliteDataBase&) = delete;
    SqliteDataBase& operator=(const SqliteDataBase&) = delete;
    SqliteDataBase(SqliteDataBase&&) = delete;
    SqliteDataBase& operator=(SqliteDataBase&&) = delete;

    void Open(const std::string& database_path) override;
    void Close() override;
    void SetUserContext(const std::string& user_hash) override;

private:
    database::SqlResult ExecInstruction(const std::string& instruction) override;
    void BeginTransaction() override;
    void CommitTransaction() override;
    void RollbackTransaction() override;

    bool execPragma(const QString& instruction);
    bool ensureCurrentUserTable();
    database::SqlResult makeErrorResult(const QString& message) const;

    QSqlDatabase db_;
    QString connectionName_;
    QString databasePath_;
    QString userHash_;
};

} // namespace uniter::data

#endif // UNITER_DATA_SQLITEDATABASE_H
