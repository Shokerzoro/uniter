
find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(TEST_SOURCES
    test_main.cpp
    appmanagertest.cpp
)

set(MANAGERS
    ../src/uniter/managers/appmanager.cpp
    ../src/uniter/managers/appmanager.h
)

add_executable(UniterTests
    ${TEST_SOURCES}
    ${MANAGERS}
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
