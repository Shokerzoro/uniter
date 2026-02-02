project(uniter_app LANGUAGES CXX)


set(STATIC_WIDGETS_SOURCES
    widgets_static/mainwindow.cpp
    widgets_static/mainwindow.h

    widgets_static/authwin/authwidget.cpp
    widgets_static/authwin/authwidget.h

    widgets_static/offlinewin/offlinewdg.h
    widgets_static/offlinewin/offlinewdg.cpp

    widgets_static/workwin/workwidget.cpp
    widgets_static/workwin/workwidget.h

    widgets_static/workwin/workbar/workbar.h
    widgets_static/workwin/workbar/workbar.cpp

    widgets_static/workwin/workarea/workarea.h
    widgets_static/workwin/workarea/workarea.cpp
)

set(GENERATIVE_WIDGETS_SOURCES
    widgets_generative/generativetab.cpp
    widgets_generative/generativetab.h

    widgets_generative/subsystemicon.h
    widgets_generative/subsystemicon.cpp
)

set(DYNAMIC_WIDGETS_SOURCES
    widgets_dynamic/dynamicwidget.cpp
    widgets_dynamic/dynamicwidget.h
)


set(DATA_SOURCES
    data/datamanager.h
    data/datamanager.cpp
)


set(CONTROL_SOURCES
    managers/appmanager.cpp
    managers/appmanager.h

    managers/configmanager.cpp
    managers/configmanager.h

    managers/uimanager.cpp
    managers/uimanager.h
)


set(NETWORK_SOURCES
    network/tcpconnector.cpp
    network/tcpconnector.h
    network/mainnetworker.cpp
    network/mainnetworker.h
    network/updaterworker.cpp
    network/updaterworker.h
    network/unetmestags.h

    network/mocknetwork.h
    network/mocknetwork.cpp
)

# = = = = = = = = common sources = = = = = = = = #

set(DATABASE_SOURCES

)

set(MESSAGES_SOURCES
    messages/unitermessage.cpp
    messages/unitermessage.h
    resources/resourceabstract.h
)


set(RESOUCES_SOURCES

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

set(CONTRACT_SOURCES
    ${MESSAGES_SOURCES}
    ${RESOUCES_SOURCES}
    ${DATABASE_SOURCES}
)

set(UNITER_SOURCES
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

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(Uniter WIN32 ${UNITER_SOURCES}
    managers/uiconstraints.h







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
target_link_libraries(Uniter PRIVATE Qt6::Core)
target_link_libraries(Uniter PRIVATE Qt6::Core)
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
