project(uniter_app LANGUAGES CXX)

# ─── Submodule: контракт и слой БД ────────────────────────────────────────────
# Определяет таргеты uniter::contract и uniter::database.
# tinyxml2 должен быть доступен к этому моменту (stub или реальный от DevKit).
include(contract/sources.cmake)
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
    # Qt-адаптеры к std-ресурсам из uniter-contract
    ${CONTRACT_QT_SOURCES}
)

target_include_directories(Uniter
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# ─── Зависимости через DevKit ────────────────────────────────────────────────
# uniter_common уже тянет Qt::Core через PUBLIC — здесь линкуем остальные модули.

# Qt: Debug → shared, Release / RelWithDebInfo → static
if(DEVKIT_QT_DYNAMIC)
    devkit_use_qt_shared(Uniter Core Widgets Network Gui)
else()
    devkit_use_qt_static(Uniter Core Widgets Network Gui)
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
        uniter::contract    # общий с сервером контракт (submodule src/uniter/contract)
        uniter::database    # общий с сервером слой БД   (submodule src/uniter/database)
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
