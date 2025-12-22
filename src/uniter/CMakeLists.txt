project(uniter_app LANGUAGES CXX)

set(BUILDER_SOURCES
    builder/appbuilder.cpp
    builder/appbuilder.h
)

set(NETWORK_SOURCES
    network/tcpconnector.cpp
    network/tcpconnector.h
    network/mainnetworker.cpp
    network/mainnetworker.h
    network/updaterworker.cpp
    network/updaterworker.h
    network/unetmestags.h
)

set(MESSAGES_SOURCES
    messages/unitermessage.h
    messages/internalmessage.cpp
    messages/internalmessage.h
)

set(MANAGERS_SOURCES
    managers/appmanager.cpp
    managers/appmanager.h
    managers/configmanager.cpp
    managers/configmanager.h
)

set(STATIC_WIDGETS_SOURCES
    widgets_static/mainwindow.cpp
    widgets_static/mainwindow.h
    widgets_static/authwin/authwidget.cpp
    widgets_static/authwin/authwidget.h
    widgets_static/status/updatecontainer.cpp
    widgets_static/status/updatecontainer.h
    widgets_static/status/update_indicator.cpp
    widgets_static/status/update_indicator.h
    widgets_static/workwin/workspace.cpp
    widgets_static/workwin/workspace.h
)

set(GENERATIVE_WIDGETS_SOURCES
    widgets_generative/generativetab.cpp
    widgets_generative/generativetab.h
)

set(DYNAMIC_WIDGETS_SOURCES
    widgets_dynamic/dynamicwidget.cpp
    widgets_dynamic/dynamicwidget.h
)

set(UNITER_SOURCES
    main.cpp
    ${NETWORK_SOURCES}
    ${MESSAGES_SOURCES}
    ${MANAGERS_SOURCES}
    ${STATIC_WIDGETS_SOURCES}
    ${GENERATIVE_WIDGETS_SOURCES}
    ${DYNAMIC_WIDGETS_SOURCES}
)

qt_add_executable(Uniter WIN32 ${UNITER_SOURCES}
    messages/unitermessage.cpp
    resources/resourceabstract.h )

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
