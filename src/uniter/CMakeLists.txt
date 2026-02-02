project(uniter_app LANGUAGES CXX)


include(sources.cmake)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(Uniter WIN32
    main.cpp

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

target_include_directories(Uniter
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(Uniter
    PRIVATE
        uniter_common
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        tinyxml2
)

target_link_libraries(Uniter PRIVATE Qt6::Widgets)
target_link_libraries(Uniter PRIVATE Qt6::Core)

# Логика вывода/макросов
if(RELEASE AND NOT DEBUG_INFO)
    # Чистый релиз: отключаем qDebug
    target_compile_definitions(Uniter PRIVATE
        QT_NO_DEBUG_OUTPUT
        RELEASE_BUILD)
elseif(RELEASE AND DEBUG_INFO)
    # RelWithDebInfo: релизная сборка, но с дебаг-инфой и логами
    target_compile_definitions(Uniter PRIVATE
        RELEASE_BUILD
        DEBUG_BUILD)
else()
    # Debug / Profile: всё дебажное включено
    target_compile_definitions(Uniter PRIVATE
        DEBUG_BUILD)
endif()

target_compile_definitions(Uniter PRIVATE
    PROJECT_NAME="${PROJECT_NAME}"
    AUTHOR="${AUTHOR}"
    PROJECT_VERSION="${PROJECT_VERSION}"
    BUILD_TIME="${BUILD_TIME}"
)
