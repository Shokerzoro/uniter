project(uniter_updater LANGUAGES CXX)

# Updater собирается только в Release / RelWithDebInfo
if(RELEASE)
    add_executable(Updater
        main.cpp
        upfuncs.cpp
        upfuncs.h
        typesheader.h
    )

    target_include_directories(Updater
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
    )

    # ─── Зависимости через DevKit ──────────────────────────────────────────────
    # Release всегда статический — DEVKIT_QT_DYNAMIC == OFF
    devkit_use_qt_static(Updater Core)
    devkit_use_tinyxml2_static(Updater)

    target_link_libraries(Updater
        PRIVATE
            uniter_common
    )

    devkit_finalize(Updater)

    # ─── Статическая линковка runtime ─────────────────────────────────────────
    target_link_options(Updater PRIVATE
        -static-libgcc
        -static-libstdc++
    )

    set_target_properties(Updater PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${BIN_BUILD_DIR}"
    )

    # ─── Макросы релиза ────────────────────────────────────────────────────────
    target_compile_definitions(Updater PRIVATE
        PROJECT_NAME="${PROJECT_NAME}"
        AUTHOR="${AUTHOR}"
        PROJECT_VERSION="${PROJECT_VERSION}"
        BUILD_TIME="${BUILD_TIME}"
    )

    if(DEBUG_INFO)
        # RelWithDebInfo: релиз с дебажными логами
        target_compile_definitions(Updater PRIVATE
            RELEASE_BUILD
            DEBUG_BUILD
        )
    else()
        # Чистый Release: без qDebug
        target_compile_definitions(Updater PRIVATE
            RELEASE_BUILD
            QT_NO_DEBUG_OUTPUT
        )
    endif()
endif()
