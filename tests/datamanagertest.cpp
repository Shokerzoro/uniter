#include <gtest/gtest.h>

namespace {

// TODO: DataManager observer tests before ManagerExecutor stabilization:
// - SingleResourceAdapter receives a CREATE/UPDATE notification for exact id.
// - SingleResourceAdapter clears stored resource on DELETE.
// - VectorResourceAdapter updates one element on CREATE/UPDATE.
// - VectorResourceAdapter removes only the matching element on DELETE.
// - Initial adapter data fill does not emit signalDataUpdated.
TEST(DataManagerObserverTest, DISABLED_ObserverMechanismBeforeManagerExecutorStabilization)
{
}

} // namespace
