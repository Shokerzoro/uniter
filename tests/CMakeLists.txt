
find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(TEST_SOURCES
    test_main.cpp
    appmanagertest.cpp
)

set(MANAGERS
    ../src/uniter/managers/appmanager.cpp
    ../src/uniter/managers/appmanager.h
)

set(RESOURCES
    ../src/uniter/resources/resourceabstract.cpp
    ../src/uniter/resources/resourceabstract.h
    ../src/uniter/resources/employee/employee.cpp
    ../src/uniter/resources/employee/employee.h

    ../src/uniter/resources/supply/purchase.cpp
    ../src/uniter/resources/supply/purchase.cpp
)

add_executable(UniterTests
    ${TEST_SOURCES}
    ${MANAGERS}
    ${RESOURCES}
)

target_link_libraries(UniterTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        Qt6::Core
        Qt6::Test
        tinyxml2
)

target_include_directories(UniterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/uniter
)

include(GoogleTest)
add_test(NAME UniterTests COMMAND UniterTests)
