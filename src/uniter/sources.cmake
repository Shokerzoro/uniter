
set(STATIC_WIDGETS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/mainwindow.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/mainwindow.h

    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/authwin/authwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/authwin/authwidget.h

    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/offlinewin/offlinewdg.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/offlinewin/offlinewdg.cpp

    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/workwin/workwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/workwin/workwidget.h

    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/workwin/workbar/workbar.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/workwin/workbar/workbar.cpp

    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/workwin/workarea/workarea.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets_static/workwin/workarea/workarea.cpp
)

set(GENERATIVE_WIDGETS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/widgets_generative/generativetab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets_generative/generativetab.h

    ${CMAKE_CURRENT_LIST_DIR}/widgets_generative/subsystemicon.h
    ${CMAKE_CURRENT_LIST_DIR}/widgets_generative/subsystemicon.cpp
)

set(DYNAMIC_WIDGETS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/widgets_dynamic/dynamicwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets_dynamic/dynamicwidget.h
)


set(DATA_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/data/datamanager.h
    ${CMAKE_CURRENT_LIST_DIR}/data/datamanager.cpp
)


set(CONTROL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/managers/appmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/managers/appmanager.h

    ${CMAKE_CURRENT_LIST_DIR}/managers/configmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/managers/configmanager.h

    ${CMAKE_CURRENT_LIST_DIR}/managers/uimanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/managers/uimanager.h
    ${CMAKE_CURRENT_LIST_DIR}/managers/uiconstraints.h
)


set(NETWORK_SOURCES
    #${CMAKE_CURRENT_LIST_DIR}/network/tcpconnector.cpp
    #${CMAKE_CURRENT_LIST_DIR}/network/tcpconnector.h
    #${CMAKE_CURRENT_LIST_DIR}/network/mainnetworker.cpp
    #${CMAKE_CURRENT_LIST_DIR}/network/mainnetworker.h
    #${CMAKE_CURRENT_LIST_DIR}/network/updaterworker.cpp
    #${CMAKE_CURRENT_LIST_DIR}/network/updaterworker.h
    #${CMAKE_CURRENT_LIST_DIR}/network/unetmestags.h

    ${CMAKE_CURRENT_LIST_DIR}/network/mocknetwork.h
    ${CMAKE_CURRENT_LIST_DIR}/network/mocknetwork.cpp
)

# = = = = = = = = common sources = = = = = = = = #

set(DATABASE_SOURCES

)

set(MESSAGES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/messages/unitermessage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/messages/unitermessage.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/resourceabstract.h
)


set(RESOUCES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/resources/material/segment.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/material/materialtemplatebase.h

    ${CMAKE_CURRENT_LIST_DIR}/resources/material/materialtemplatecomposite.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/material/materialtemplatesimple.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/material/materialtemplatesimple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/material/materialtemplatecomposite.cpp

    ${CMAKE_CURRENT_LIST_DIR}/resources/supply/purchase.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/supply/purchase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/supply/purchasecomplex.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/supply/purchasecomplex.cpp

    ${CMAKE_CURRENT_LIST_DIR}/resources/materialinstance/materialinstancebase.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/materialinstance/materialinstancesimple.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/materialinstance/materialinstancesimple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/materialinstance/materialinstancecomposite.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/materialinstance/materialinstancecomposite.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/materialinstance/materialinstancebase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/resourceabstract.cpp

    ${CMAKE_CURRENT_LIST_DIR}/resources/design/assembly.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/design/assembly.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/design/part.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/design/part.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/design/fileversion.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/design/project.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/design/project.cpp

    ${CMAKE_CURRENT_LIST_DIR}/resources/employee/permissions.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/employee/employee.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/employee/employee.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/plant/plant.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/plant/plant.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/plant/task.h
    ${CMAKE_CURRENT_LIST_DIR}/resources/plant/task.cpp
)

set(CONTRACT_SOURCES
    ${MESSAGES_SOURCES}
    ${RESOUCES_SOURCES}
    ${DATABASE_SOURCES}
)
