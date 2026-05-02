#include "../src/uniter/data/sqlitedatabase.h"
#include "../src/uniter/database/documents/documentsexecutor.h"
#include "../src/uniter/database/iresexecutor.h"

#include <gtest/gtest.h>
#include <QTemporaryDir>

namespace {

using namespace uniter;

int64_t asInt64(const database::Cell& cell)
{
    return std::get<int64_t>(cell);
}

std::string asString(const database::Cell& cell)
{
    return std::get<std::string>(cell);
}

TEST(DocumentsExecutorTest, FullDatabaseCycle)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    data::SqliteDataBase db;
    db.Open((dir.path() + "/documents-cycle.sqlite").toStdString());
    db.SetUserContext("documents-cycle-user");

    auto createManagerEmployees = database::IResExecutor::Exec(db, R"SQL(
CREATE TABLE IF NOT EXISTS manager_employees (
    id INTEGER PRIMARY KEY
);
)SQL");
    ASSERT_TRUE(createManagerEmployees.success) << createManagerEmployees.error_message;

    auto seedManagerEmployees = database::IResExecutor::Exec(db, R"SQL(
INSERT OR IGNORE INTO manager_employees(id) VALUES (0), (1), (2);
)SQL");
    ASSERT_TRUE(seedManagerEmployees.success) << seedManagerEmployees.error_message;

    database::DocumentsExecutor executor;

    auto init = executor.Initialize(db);
    ASSERT_TRUE(init.success) << init.message;

    auto verify = executor.Verify(db);
    ASSERT_TRUE(verify.success) << verify.message;

    auto createLink = database::IResExecutor::Exec(db, R"SQL(
INSERT INTO documents_doc_link (
    id, actual, created_by, updated_by, created_at, updated_at, doclink_type
) VALUES (100, 1, 1, 1, 1, 1, 0);
)SQL");
    ASSERT_TRUE(createLink.success) << createLink.error_message;

    auto createDoc = database::IResExecutor::Exec(db, R"SQL(
INSERT INTO documents_doc (
    id, actual, created_by, updated_by, created_at, updated_at,
    doc_link_id, doc_type, name, object_key, sha256, size_bytes, mime_type, description, local_path
) VALUES (
    200, 1, 1, 1, 1, 1,
    100, 0, 'DocName', 'minio/key', 'hash', 42, 'application/pdf', 'desc', '/tmp/file.pdf'
);
)SQL");
    ASSERT_TRUE(createDoc.success) << createDoc.error_message;

    auto readDoc = database::IResExecutor::Exec(db, R"SQL(
SELECT doc_link_id, doc_type, name, object_key, size_bytes
FROM documents_doc
WHERE id = 200;
)SQL");
    ASSERT_TRUE(readDoc.success) << readDoc.error_message;
    ASSERT_EQ(readDoc.result.size(), 1u);
    EXPECT_EQ(asInt64(readDoc.result[0][0]), 100);
    EXPECT_EQ(asInt64(readDoc.result[0][1]), 0);
    EXPECT_EQ(asString(readDoc.result[0][2]), "DocName");
    EXPECT_EQ(asString(readDoc.result[0][3]), "minio/key");
    EXPECT_EQ(asInt64(readDoc.result[0][4]), 42);

    auto updateDoc = database::IResExecutor::Exec(db, R"SQL(
UPDATE documents_doc
SET name = 'UpdatedDoc',
    size_bytes = 64,
    updated_at = 2000,
    updated_by = 2
WHERE id = 200;
)SQL");
    ASSERT_TRUE(updateDoc.success) << updateDoc.error_message;

    auto readUpdatedDoc = database::IResExecutor::Exec(db, R"SQL(
SELECT name, size_bytes, updated_by
FROM documents_doc
WHERE id = 200;
)SQL");
    ASSERT_TRUE(readUpdatedDoc.success) << readUpdatedDoc.error_message;
    ASSERT_EQ(readUpdatedDoc.result.size(), 1u);
    EXPECT_EQ(asString(readUpdatedDoc.result[0][0]), "UpdatedDoc");
    EXPECT_EQ(asInt64(readUpdatedDoc.result[0][1]), 64);
    EXPECT_EQ(asInt64(readUpdatedDoc.result[0][2]), 2);

    auto deleteDoc = database::IResExecutor::Exec(db, "DELETE FROM documents_doc WHERE id = 200;");
    ASSERT_TRUE(deleteDoc.success) << deleteDoc.error_message;
    auto checkNoDoc = database::IResExecutor::Exec(db, "SELECT COUNT(*) FROM documents_doc WHERE id = 200;");
    ASSERT_TRUE(checkNoDoc.success) << checkNoDoc.error_message;
    ASSERT_EQ(checkNoDoc.result.size(), 1u);
    EXPECT_EQ(asInt64(checkNoDoc.result[0][0]), 0);

    auto clear = executor.ClearData(db);
    ASSERT_TRUE(clear.success) << clear.message;

    auto checkAfterClear = database::IResExecutor::Exec(db, R"SQL(
SELECT
    (SELECT COUNT(*) FROM documents_doc_link) AS links_count,
    (SELECT COUNT(*) FROM documents_doc) AS docs_count;
)SQL");
    ASSERT_TRUE(checkAfterClear.success) << checkAfterClear.error_message;
    ASSERT_EQ(checkAfterClear.result.size(), 1u);
    EXPECT_EQ(asInt64(checkAfterClear.result[0][0]), 0);
    EXPECT_EQ(asInt64(checkAfterClear.result[0][1]), 0);

    auto drop = executor.DropStructures(db);
    ASSERT_TRUE(drop.success) << drop.message;

    auto verifyAfterDrop = executor.Verify(db);
    EXPECT_FALSE(verifyAfterDrop.success);
    EXPECT_EQ(verifyAfterDrop.code, database::ExecutorStatusCode::SchemaInvalid);

    auto reinit = executor.Initialize(db);
    ASSERT_TRUE(reinit.success) << reinit.message;
    auto reverify = executor.Verify(db);
    ASSERT_TRUE(reverify.success) << reverify.message;
}

} // namespace
