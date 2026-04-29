#include "../src/uniter/contract/manager/employee.h"
#include "../src/uniter/data/dataadapter.h"
#include "../src/uniter/data/datamanager.h"

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <chrono>
#include <memory>
#include <vector>

Q_DECLARE_METATYPE(uniter::data::SingleResourceAdapter*)
Q_DECLARE_METATYPE(uniter::data::VectorResourceAdapter*)
Q_DECLARE_METATYPE(uniter::contract::CrudAction)

namespace {

using namespace uniter;

std::shared_ptr<contract::manager::Employee> makeEmployee(uint64_t id, std::string firstName)
{
    const auto now = std::chrono::system_clock::now();
    return std::make_shared<contract::manager::Employee>(
        id,
        true,
        now,
        now,
        1,
        1,
        "login" + std::to_string(id),
        "employee" + std::to_string(id) + "@uniter.local",
        "password-hash",
        std::move(firstName),
        "Tester",
        std::nullopt,
        std::vector<contract::manager::EmployeeAssignment>{});
}

std::shared_ptr<contract::UniterMessage> makeManagerMessage(contract::CrudAction action,
                                                            std::shared_ptr<contract::ResourceAbstract> resource)
{
    auto message = std::make_shared<contract::UniterMessage>();
    message->subsystem = contract::Subsystem::MANAGER;
    message->GenSubsystem = contract::GenSubsystem::NOTGEN;
    message->GenSubsystemId = 0;
    message->crudact = action;
    message->status = contract::MessageStatus::NOTIFICATION;
    message->resource = std::move(resource);
    return message;
}

class DataManagerObserverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(tempDir_.isValid());
        qputenv("TEMP_DIR", tempDir_.path().toUtf8());

        qRegisterMetaType<uniter::data::SingleResourceAdapter*>();
        qRegisterMetaType<uniter::data::VectorResourceAdapter*>();
        qRegisterMetaType<uniter::contract::CrudAction>();

        manager_ = data::DataManager::instance();
        manager_->onResetDatabase();
    }

    void TearDown() override
    {
        manager_->onResetDatabase();
        qunsetenv("TEMP_DIR");
    }

    contract::SubsystemKey employeeListKey() const
    {
        return contract::SubsystemKey{
            contract::Subsystem::MANAGER,
            contract::GenSubsystem::NOTGEN,
            std::nullopt,
            contract::ResourceType::EMPLOYEES
        };
    }

    data::DataManager* manager_ = nullptr;
    QTemporaryDir tempDir_;
};

TEST_F(DataManagerObserverTest, SingleResourceAdapterReceivesCreateUpdateAndDeleteByExactKey)
{
    data::SingleResourceAdapter adapter(employeeListKey(), 42);
    QSignalSpy spy(&adapter, &data::SingleResourceAdapter::signalDataUpdated);

    const auto createMessage = makeManagerMessage(contract::CrudAction::CREATE,
                                                  makeEmployee(42, "Created"));
    manager_->notifyObserversForTest(*createMessage);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(adapter.resource(), createMessage->resource);
    EXPECT_EQ(spy.takeFirst().at(1).value<contract::CrudAction>(), contract::CrudAction::CREATE);

    const auto updateMessage = makeManagerMessage(contract::CrudAction::UPDATE,
                                                  makeEmployee(42, "Updated"));
    manager_->notifyObserversForTest(*updateMessage);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(adapter.resource(), updateMessage->resource);
    EXPECT_EQ(spy.takeFirst().at(1).value<contract::CrudAction>(), contract::CrudAction::UPDATE);

    const auto deleteMessage = makeManagerMessage(contract::CrudAction::DELETE,
                                                  makeEmployee(42, "Deleted"));
    manager_->notifyObserversForTest(*deleteMessage);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(adapter.resource(), nullptr);
    EXPECT_EQ(spy.takeFirst().at(1).value<contract::CrudAction>(), contract::CrudAction::DELETE);
}

TEST_F(DataManagerObserverTest, SingleResourceAdapterIgnoresDifferentResourceId)
{
    data::SingleResourceAdapter adapter(employeeListKey(), 42);
    QSignalSpy spy(&adapter, &data::SingleResourceAdapter::signalDataUpdated);

    const auto message = makeManagerMessage(contract::CrudAction::UPDATE,
                                            makeEmployee(7, "Other"));
    manager_->notifyObserversForTest(*message);

    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(adapter.resource(), nullptr);
}

TEST_F(DataManagerObserverTest, VectorResourceAdapterCreatesUpdatesAndDeletesMatchingItem)
{
    data::VectorResourceAdapter adapter(employeeListKey(), {makeEmployee(7, "Existing")});
    QSignalSpy spy(&adapter, &data::VectorResourceAdapter::signalDataUpdated);

    const auto createMessage = makeManagerMessage(contract::CrudAction::CREATE,
                                                  makeEmployee(42, "Created"));
    manager_->notifyObserversForTest(*createMessage);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(adapter.resources().size(), 2);
    EXPECT_EQ(adapter.resources().back(), createMessage->resource);
    EXPECT_EQ(spy.takeFirst().at(2).value<contract::CrudAction>(), contract::CrudAction::CREATE);

    const auto updateMessage = makeManagerMessage(contract::CrudAction::UPDATE,
                                                  makeEmployee(42, "Updated"));
    manager_->notifyObserversForTest(*updateMessage);

    ASSERT_EQ(spy.count(), 1);
    ASSERT_EQ(adapter.resources().size(), 2);
    EXPECT_EQ(adapter.resources().back(), updateMessage->resource);
    EXPECT_EQ(spy.takeFirst().at(2).value<contract::CrudAction>(), contract::CrudAction::UPDATE);

    const auto deleteMessage = makeManagerMessage(contract::CrudAction::DELETE,
                                                  makeEmployee(42, "Deleted"));
    manager_->notifyObserversForTest(*deleteMessage);

    ASSERT_EQ(spy.count(), 1);
    ASSERT_EQ(adapter.resources().size(), 1);
    EXPECT_EQ(adapter.resources().front()->id, 7);
    EXPECT_EQ(spy.takeFirst().at(2).value<contract::CrudAction>(), contract::CrudAction::DELETE);
}

TEST_F(DataManagerObserverTest, InitializeDataDoesNotEmitSignals)
{
    data::SingleResourceAdapter singleAdapter(employeeListKey(), 42);
    data::VectorResourceAdapter vectorAdapter(employeeListKey());
    QSignalSpy singleSpy(&singleAdapter, &data::SingleResourceAdapter::signalDataUpdated);
    QSignalSpy vectorSpy(&vectorAdapter, &data::VectorResourceAdapter::signalDataUpdated);

    singleAdapter.initializeData(makeEmployee(42, "Initial"));
    vectorAdapter.initializeData(
        std::vector<std::shared_ptr<contract::ResourceAbstract>>{makeEmployee(42, "Initial")});

    EXPECT_EQ(singleSpy.count(), 0);
    EXPECT_EQ(vectorSpy.count(), 0);
    ASSERT_NE(singleAdapter.resource(), nullptr);
    EXPECT_EQ(vectorAdapter.resources().size(), 1);
}

} // namespace
