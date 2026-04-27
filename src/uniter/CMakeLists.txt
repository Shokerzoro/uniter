project(uniter_app LANGUAGES CXX)

# ─── Submodule: контракт (sources.cmake устанавливает CONTRACT_SOURCES) ────────
# contract/sources.cmake устанавливает CONTRACT_SOURCES и CONTRACT_INCLUDE_DIR.
include(contract/sources.cmake)

# ─── Submodule: база данных (sources.cmake устанавливает DATABASE_SOURCES) ─────
# database/sources.cmake устанавливает DATABASE_SOURCES и DATABASE_INCLUDE_DIR.
include(database/sources.cmake)

include(sources.cmake)

# WIN32 — скрываем консольное окно (эквивалент qt_add_executable WIN32)
add_executable(Uniter WIN32
    main.cpp

    # UI Layer
    ${STATIC_WIDGETS_SOURCES}
    ${GENERATIVE_WIDGETS_SOURCES}
    ${DYNAMIC_WIDGETS_SOURCES}
    #BUSYNESS LOGIC LAYER
    ${BLOGIC_SOURCES}
    # Data layer
    ${DATA_SOURCES}
    # Control layer
    ${CONTROL_SOURCES}
    # Net layer
    ${NETWORK_SOURCES}
    # Qt-адаптеры к std-ресурсам из contract
    ${CONTRACT_QT_SOURCES}
    # Contract (client/server shared: protocol + resources)
    ${CONTRACT_SOURCES}
    # Database layer (IDatabase + mappers)
    ${DATABASE_SOURCES}
)

target_include_directories(Uniter
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CONTRACT_INCLUDE_DIR}   # корень submodule contract/ — для #include "design/assembly.h" и т.п.
        ${DATABASE_INCLUDE_DIR}   # корень submodule database/ — для #include "idatabase.h" и т.п.
)

# ─── Зависимости через DevKit ────────────────────────────────────────────────
# uniter_common уже тянет Qt::Core через PUBLIC — здесь линкуем остальные модули.

# Qt: Debug → shared, Release / RelWithDebInfo → static
if(DEVKIT_QT_DYNAMIC)
    devkit_use_qt_shared(Uniter Core Widgets Gui Sql)
else()
    devkit_use_qt_static(Uniter Core Widgets Gui Sql)
endif()

# tinyxml2: Debug → shared, Release / RelWithDebInfo → static
if(DEVKIT_QT_DYNAMIC)
    devkit_use_tinyxml2_shared(Uniter)
else()
    devkit_use_tinyxml2_static(Uniter)
endif()

target_link_libraries(Uniter
    PRIVATE
        uniter_common
)

# ОБЯЗАТЕЛЬНО последним — добавляет HarfBuzz+FreeType если подключён MuPDF,
# при наличии только Qt — просто финализирует AUTOMOC.
devkit_finalize(Uniter)

# ─── Макросы отладки / релиза ─────────────────────────────────────────────
if(RELEASE AND NOT DEBUG_INFO)
    target_compile_definitions(Uniter PRIVATE
        QT_NO_DEBUG_OUTPUT
        RELEASE_BUILD)
elseif(RELEASE AND DEBUG_INFO)
    target_compile_definitions(Uniter PRIVATE
        RELEASE_BUILD
        DEBUG_BUILD)
else()
    target_compile_definitions(Uniter PRIVATE
        DEBUG_BUILD)
endif()

target_compile_definitions(Uniter PRIVATE
    PROJECT_NAME="${PROJECT_NAME}"
    AUTHOR="${AUTHOR}"
    PROJECT_VERSION="${PROJECT_VERSION}"
    BUILD_TIME="${BUILD_TIME}"
)

# ─── Статическая линковка runtime для Release ─────────────────────────────
# -static-libgcc -static-libstdc++ добавляем только в Release,
# чтобы exe не зависел от MinGW runtime DLL.
if(NOT DEVKIT_QT_DYNAMIC)
    target_link_options(Uniter PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()
