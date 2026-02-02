
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
    ${CMAKE_CURRENT_LIST_DIR}/widgets_generative/subsystemtab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/widgets_generative/subsystemtab.h

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
    ${CMAKE_CURRENT_LIST_DIR}/control/appmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/control/appmanager.h

    ${CMAKE_CURRENT_LIST_DIR}/control/configmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/control/configmanager.h

    ${CMAKE_CURRENT_LIST_DIR}/control/uimanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/control/uimanager.h
    ${CMAKE_CURRENT_LIST_DIR}/control/uiconstraints.h
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
    ${CMAKE_CURRENT_LIST_DIR}/contract/unitermessage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/unitermessage.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/resourceabstract.h
)


set(RESOUCES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/segment.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/materialtemplatebase.h

    ${CMAKE_CURRENT_LIST_DIR}/contract/material/materialtemplatecomposite.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/materialtemplatesimple.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/materialtemplatesimple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/materialtemplatecomposite.cpp

    ${CMAKE_CURRENT_LIST_DIR}/contract/supply/purchase.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/supply/purchase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/supply/purchasecomplex.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/supply/purchasecomplex.cpp

    ${CMAKE_CURRENT_LIST_DIR}/contract/materialinstance/materialinstancebase.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/materialinstance/materialinstancesimple.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/materialinstance/materialinstancesimple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/materialinstance/materialinstancecomposite.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/materialinstance/materialinstancecomposite.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/materialinstance/materialinstancebase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/resourceabstract.cpp

    ${CMAKE_CURRENT_LIST_DIR}/contract/design/assembly.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/assembly.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/part.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/part.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/fileversion.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/project.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/project.cpp

    ${CMAKE_CURRENT_LIST_DIR}/contract/employee/permissions.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/employee/employee.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/employee/employee.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/plant/plant.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/plant/plant.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/plant/task.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/plant/task.cpp
)

set(CONTRACT_SOURCES
    ${MESSAGES_SOURCES}
    ${RESOUCES_SOURCES}
    ${DATABASE_SOURCES}
)
