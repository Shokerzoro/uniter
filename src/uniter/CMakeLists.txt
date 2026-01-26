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

set(RESOUCES_SOURCES
    messages/unitermessage.cpp
    resources/resourceabstract.h
    resources/material/segment.h
    resources/material/materialtemplatebase.h

    resources/material/materialtemplatecomposite.h
    resources/material/materialtemplatesimple.h
    resources/material/materialtemplatesimple.cpp
    resources/material/materialtemplatecomposite.cpp

    resources/supply/purchase.h resources/supply/purchase.cpp
    resources/supply/purchasecomplex.h resources/supply/purchasecomplex.cpp

    resources/materialinstance/materialinstancebase.h
    resources/materialinstance/materialinstancesimple.h resources/materialinstance/materialinstancesimple.cpp
    resources/materialinstance/materialinstancecomposite.h resources/materialinstance/materialinstancecomposite.cpp
    resources/materialinstance/materialinstancebase.cpp
    resources/resourceabstract.cpp

    resources/design/assembly.h resources/design/assembly.cpp
    resources/design/part.h resources/design/part.cpp
    resources/design/fileversion.h
    resources/design/project.h resources/design/project.cpp

    resources/employee/permissions.h
    resources/employee/employee.h resources/employee/employee.cpp
    resources/plant/plant.h resources/plant/plant.cpp
    resources/plant/task.h resources/plant/task.cpp
)

set(DYNAMIC_WIDGETS_SOURCES
    widgets_dynamic/dynamicwidget.cpp
    widgets_dynamic/dynamicwidget.h
)

set(GENERATIVE_WIDGETS_SOURCES
    widgets_generative/generativetab.cpp
    widgets_generative/generativetab.h
)

set(STATIC_WIDGETS_SOURCES
    widgets_static/mainwindow.cpp
    widgets_static/mainwindow.h

    widgets_static/authwin/authwidget.cpp
    widgets_static/authwin/authwidget.h

    widgets_static/offlinewin/offlinewdg.h
    widgets_static/offlinewin/offlinewdg.cpp

    widgets_static/workwin/workwidget.cpp
    widgets_static/workwin/workwidget.h

    widgets_static/workwin/statusbar/statusbar.h
    widgets_static/workwin/statusbar/statusbar.cpp

    widgets_static/workwin/workarea/workarea.h
    widgets_static/workwin/workarea/workarea.cpp
)

set(UNITER_SOURCES
    main.cpp
    ${NETWORK_SOURCES}
    ${MESSAGES_SOURCES}
    ${MANAGERS_SOURCES}
    ${RESOUCES_SOURCES}
    ${STATIC_WIDGETS_SOURCES}
    ${GENERATIVE_WIDGETS_SOURCES}
    ${DYNAMIC_WIDGETS_SOURCES}
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(Uniter WIN32 ${UNITER_SOURCES}
    widgets_static/uisettings.h widgets_static/uisettings.cpp
    widgets_generative/subsystemicon.h widgets_generative/subsystemicon.cpp

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
target_link_libraries(Uniter PRIVATE Qt6::Widgets)
target_link_libraries(Uniter PRIVATE Qt6::Widgets)
target_link_libraries(Uniter PRIVATE Qt6::Core)
target_link_libraries(Uniter PRIVATE Qt6::Widgets)

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
