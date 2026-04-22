
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

set(BLOGIC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/blogic/pdmmanager.h
    ${CMAKE_CURRENT_LIST_DIR}/blogic/erpmanager.h
)


set(DATA_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/data/datamanager.h
    ${CMAKE_CURRENT_LIST_DIR}/data/datamanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/data/idataobserver.h
    ${CMAKE_CURRENT_LIST_DIR}/data/idataobserver.cpp
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
    ${CMAKE_CURRENT_LIST_DIR}/network/serverconnector.h
    ${CMAKE_CURRENT_LIST_DIR}/network/serverconnector.cpp

    ${CMAKE_CURRENT_LIST_DIR}/network/kafkaconnector.h
    ${CMAKE_CURRENT_LIST_DIR}/network/kafkaconnector.cpp

    ${CMAKE_CURRENT_LIST_DIR}/network/miniconnector.h
    ${CMAKE_CURRENT_LIST_DIR}/network/miniconnector.cpp
)

# = = = = = = = = common sources = = = = = = = = #

set(DATABASE_SOURCES

)

# Протокол и корневой абстрактный ресурс.
# После удаления XML-сериализации у ResourceAbstract нет .cpp.
set(MESSAGES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/contract/unitermessage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/unitermessage.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/resourceabstract.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/uniterprotocol.h
)


# Ресурсы сгруппированы по подсистемам.
# Сериализация удалена: у ресурсов остались только заголовочные файлы.
# Исключение: instance/instancesimple.cpp содержит setPrefixValue/Suffix-логику.
set(RESOUCES_SOURCES
    # --- Subsystem::MATERIALS --------------------------------------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/segment.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/templatebase.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/templatesimple.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/material/templatecomposite.h

    # --- Subsystem::INSTANCES --------------------------------------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/instance/instancebase.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/instance/instancesimple.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/instance/instancesimple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/contract/instance/instancecomposite.h

    # --- Subsystem::DESIGN -----------------------------------------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/designtypes.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/project.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/assembly.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/assemblyconfig.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/part.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/design/partconfig.h

    # --- Subsystem::PDM --------------------------------------------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/pdm/pdmtypes.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/pdm/pdmproject.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/pdm/snapshot.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/pdm/delta.h

    # --- Subsystem::DOCUMENTS --------------------------------------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/documents/doctypes.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/documents/doc.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/documents/doclink.h

    # --- Subsystem::MANAGER (employee, plant, integration) ---------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/manager/managertypes.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/manager/permissions.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/manager/employee.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/manager/plant.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/manager/integration.h

    # --- Subsystem::PURCHASES --------------------------------------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/supply/purchase.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/supply/purchasecomplex.h

    # --- Subsystem::GENERATIVE / GenSubsystem::PRODUCTION ----------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/production/productiontypes.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/production/productiontask.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/production/productionstock.h
    ${CMAKE_CURRENT_LIST_DIR}/contract/production/productionsupply.h

    # --- Subsystem::GENERATIVE / GenSubsystem::INTERGATION ---------------
    ${CMAKE_CURRENT_LIST_DIR}/contract/integration/integrationtask.h
)

set(CONTRACT_SOURCES
    ${MESSAGES_SOURCES}
    ${RESOUCES_SOURCES}
    ${DATABASE_SOURCES}
)
