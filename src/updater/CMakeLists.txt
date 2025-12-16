project(uniter_updater LANGUAGES CXX)

if(RELEASE)
    add_executable(Updater
        main.cpp
        upfuncs.cpp
        upfuncs.h
    )

    target_include_directories(Updater
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
    )

    target_link_libraries(Updater
        PRIVATE
            uniter_common
            Qt6::Core
    )

    set_target_properties(Updater PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${BIN_BUILD_DIR}"
    )

    # Общие дефайны
    target_compile_definitions(Updater PRIVATE
        PROJECT_NAME="${PROJECT_NAME}"
        AUTHOR="${AUTHOR}"
        PROJECT_VERSION="${PROJECT_VERSION}"
        BUILD_TIME="${BUILD_TIME}"
    )

    # Релизная логика
    if(DEBUG_INFO)
        # RelWithDebInfo: релиз, но с дебажной инфой и логами
        target_compile_definitions(Updater PRIVATE
            RELEASE_BUILD
            DEBUG_BUILD
        )
    else()
        # Чистый Release: отключаем qDebug
        target_compile_definitions(Updater PRIVATE
            RELEASE_BUILD
            QT_NO_DEBUG_OUTPUT
        )
    endif()
endif()
