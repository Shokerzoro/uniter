# Google Tests — остаётся через FetchContent (объявлен в корневом CMakeLists.txt).
# Здесь только описываем таргет UniterTests.

include(../src/uniter/sources.cmake)

add_executable(UniterTests
    test_main.cpp
    appmanagertest.cpp

    # UI Layer
    ${STATIC_WIDGETS_SOURCES}
    ${GENERATIVE_WIDGETS_SOURCES}
    ${DYNAMIC_WIDGETS_SOURCES}

    # Data layer
    ${DATA_SOURCES}
    # Control layer
    ${CONTROL_SOURCES}
    # Net layer
    ${NETWORK_SOURCES}
    # Contract (client/server)
    ${CONTRACT_SOURCES}
)

# ─── Зависимости через DevKit ────────────────────────────────────────────────
# Тесты всегда собираются вместе с основным проектом — режим линковки Qt
# и tinyxml2 совпадает с таргетом Uniter.

if(DEVKIT_QT_DYNAMIC)
    devkit_use_qt_shared(UniterTests Core Widgets Network)
    devkit_use_tinyxml2_shared(UniterTests)
else()
    devkit_use_qt_static(UniterTests Core Widgets Network)
    devkit_use_tinyxml2_static(UniterTests)
endif()

target_link_libraries(UniterTests
    PRIVATE
        uniter_common
        GTest::gtest_main
        GTest::gmock_main
)

# Qt Test module (только shared/static, уже подключён через devkit выше,
# но Qt6::Test нужен отдельно т.к. не входит в набор Uniter)
if(DEVKIT_QT_DYNAMIC)
    devkit_use_qt_shared(UniterTests Test)
else()
    devkit_use_qt_static(UniterTests Test)
endif()

devkit_finalize(UniterTests)

target_include_directories(UniterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/uniter
)

include(GoogleTest)
add_test(NAME UniterTests COMMAND UniterTests)
