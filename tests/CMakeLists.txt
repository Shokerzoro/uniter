
find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(TEST_SOURCES
    test_main.cpp
    appmanagertest.cpp
)

include(../src/uniter/sources.cmake)

add_executable(UniterTests
    ${TEST_SOURCES}

    #UI Layer
    ${STATIC_WIDGETS_SOURCES}
    ${GENERATIVE_WIDGETS_SOURCES}
    ${DYNAMIC_WIDGETS_SOURCES}

    #Data layer
    ${DATA_SOURCES}
    #Control layer
    ${CONTROL_SOURCES}
    #Net layer
    ${NETWORK_SOURCES}
    #Contract (client/server)
    ${CONTRACT_SOURCES}
)

target_link_libraries(UniterTests
    PRIVATE
    uniter_common
    GTest::gtest_main
    GTest::gmock_main
    Qt6::Core
    Qt6::Test
    Qt6::Widgets
    Qt6::Network
    tinyxml2
)

target_include_directories(UniterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/uniter
)

include(GoogleTest)
add_test(NAME UniterTests COMMAND UniterTests)
