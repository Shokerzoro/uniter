
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
    ${CMAKE_CURRENT_LIST_DIR}/data/sqlitedatabase.h
    ${CMAKE_CURRENT_LIST_DIR}/data/sqlitedatabase.cpp
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

# Qt-адаптеры к std-ресурсам из contract.
# Используются строго на границе "контракт ↔ Qt-код клиента".
set(CONTRACT_QT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/contract_qt/qt_compat.h
)

# CONTRACT_SOURCES устанавливается в contract/sources.cmake (включается в CMakeLists.txt до этого файла).
