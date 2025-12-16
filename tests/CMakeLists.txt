add_executable(UniterTests
    test_main.cpp
    test_common.cpp
)

target_link_libraries(UniterTests
    PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        uniter_common
        Qt6::Core
)

target_include_directories(UniterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
)

include(GoogleTest)
gtest_add_tests(TARGET UniterTests)
