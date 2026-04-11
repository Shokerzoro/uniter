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
# ВАЖНО: все Qt-модули объединены в ОДИН вызов devkit_use_qt_*.
# Повторный вызов devkit_use_qt_* для того же таргета порождает повторный
# find_package(Qt6), который пытается создать уже существующий IMPORTED
# таргет EntryPointMinGW32 — это приводит к CMake Error.
# Qt6::Test включён сюда же, а не отдельным вызовом.

if(DEVKIT_QT_DYNAMIC)
    devkit_use_qt_shared(UniterTests Core Widgets Network Test)
    devkit_use_tinyxml2_shared(UniterTests)
else()
    devkit_use_qt_static(UniterTests Core Widgets Network Test)
    devkit_use_tinyxml2_static(UniterTests)
endif()

target_link_libraries(UniterTests
    PRIVATE
        uniter_common
        GTest::gtest_main
        GTest::gmock_main
)

devkit_finalize(UniterTests)

target_include_directories(UniterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/uniter
)

include(GoogleTest)
add_test(NAME UniterTests COMMAND UniterTests)
