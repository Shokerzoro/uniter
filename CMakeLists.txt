#===============================================================================
# 1. ОСНОВНЫЕ НАСТРОЙКИ ПРОЕКТА
#===============================================================================
cmake_minimum_required(VERSION 3.21)
project(Uniter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#===============================================================================
# 2. ПОИСК И НАСТРОЙКА Qt6 + GOOGLE TESTS + TinyXML2 (FetchContent)
#===============================================================================
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)
qt_standard_project_setup()

# Fetch dependencies
include(FetchContent)

# Google Tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# TinyXML2
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        10.0.0
)

FetchContent_MakeAvailable(googletest tinyxml2)
enable_testing()

#===============================================================================
# 3. РЕЖИМЫ СБОРКИ И СТАТИЧЕСКАЯ ЛИНКОВКА
#===============================================================================
if(CMAKE_BUILD_TYPE MATCHES "Debug|Profile")
    # Динамика
    set(RELEASE OFF)
    set(DEBUG_INFO ON)

elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(RELEASE ON)
    set(DEBUG_INFO ON)

elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    # Чистый релиз: статика, без дебага
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(RELEASE ON)
    set(DEBUG_INFO OFF)
else()
    message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'")
endif()

#===============================================================================
# 4. МЕТАДАННЫЕ ПРОЕКТА
#===============================================================================
set(PROJECT_VERSION "0.0.0")
set(AUTHOR "shoker_zoro")
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")

#===============================================================================
# 5. ПУТИ СОБРАННЫХ ФАЙЛОВ И ДЕПЛОЯ
#===============================================================================
set(RESOURCES_DIR    "${CMAKE_SOURCE_DIR}/resources")
set(BUILD_DIR        "${CMAKE_BINARY_DIR}")
set(BIN_BUILD_DIR    "${BUILD_DIR}/bin")
set(CONFIG_BUILD_DIR "${BUILD_DIR}/config")
set(ETC_BUILD_DIR    "${BUILD_DIR}/etc")

file(MAKE_DIRECTORY "${RESOURCES_DIR}")
file(MAKE_DIRECTORY "${BIN_BUILD_DIR}")
file(MAKE_DIRECTORY "${CONFIG_BUILD_DIR}")
file(MAKE_DIRECTORY "${ETC_BUILD_DIR}")

#===============================================================================
# 6. ПУТИ для deploy (инсталлер и на сервер)
#===============================================================================
if(RELEASE)
    # deploy/release — то, что уходит на сервер
    set(DEPLOY_RELEASE_DIR "${CMAKE_SOURCE_DIR}/deploy/release")
    set(DEPLOY_RELEASE_BIN "${DEPLOY_RELEASE_DIR}/bin")

    # deploy/installer — всё для сборки инсталлера
    set(DEPLOY_INSTALLER_DIR "${CMAKE_SOURCE_DIR}/deploy/installer")

    # Новая структура инсталлера
    set(INSTALLER_BUILD_DIR "${CMAKE_BINARY_DIR}/installer")
    set(NSIS_INSTALLER_BUILD_DIR "${INSTALLER_BUILD_DIR}/nsis")
    set(DIST_INSTALLER_DEPLOY_DIR "${DEPLOY_INSTALLER_DIR}/dist")
    set(BIN_INST_DIR "${DIST_INSTALLER_DEPLOY_DIR}/bin")
    set(ICON_INSTALLER_DEPLOY_DIR "${DEPLOY_INSTALLER_DIR}/icons")

    file(MAKE_DIRECTORY "${DEPLOY_RELEASE_DIR}")
    file(MAKE_DIRECTORY "${DEPLOY_RELEASE_BIN}")
    file(MAKE_DIRECTORY "${DEPLOY_INSTALLER_DIR}")
    file(MAKE_DIRECTORY "${INSTALLER_BUILD_DIR}")
    file(MAKE_DIRECTORY "${NSIS_INSTALLER_BUILD_DIR}")
    file(MAKE_DIRECTORY "${DIST_INSTALLER_DEPLOY_DIR}")
    file(MAKE_DIRECTORY "${BIN_INST_DIR}")
    file(MAKE_DIRECTORY "${ICON_INSTALLER_DEPLOY_DIR}")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/installer/dist/bin")
endif()

#===============================================================================
# 7. ПОДПРОЕКТЫ (ВСЕГДА + ТЕСТЫ)
#===============================================================================
add_subdirectory(src/common)
add_subdirectory(src/uniter)
add_subdirectory(src/updater)
add_subdirectory(tests)

#===============================================================================
# 8. ГЕНЕРАЦИЯ КОНФИГУРАЦИОННЫХ ФАЙЛОВ (только RELEASE)
#===============================================================================
if(RELEASE)
    configure_file("${RESOURCES_DIR}/release/meta.XML.in"
                   "${BUILD_DIR}/meta.XML" @ONLY)
endif()

#===============================================================================
# 9. INSTALLER (только RELEASE)
#===============================================================================
if(RELEASE)
    # 9.1. Генерируем NSIS-скрипт из resources/installer/
    configure_file("${RESOURCES_DIR}/installer/installer.nsi.in"
        "${NSIS_INSTALLER_BUILD_DIR}/installer.nsi" @ONLY)
    configure_file("${RESOURCES_DIR}/installer/license.txt"
        "${NSIS_INSTALLER_BUILD_DIR}/license.txt" COPYONLY)

    # Иконки для NSIS из resources/installer/
    file(COPY "${RESOURCES_DIR}/installer/"
         DESTINATION "${NSIS_INSTALLER_BUILD_DIR}/"
         FILES_MATCHING PATTERN "*.ico" PATTERN "*.png")

    # 9.2. Цель installer зависит от бинарей
    add_custom_target(installer ALL
        DEPENDS Uniter Updater
        COMMENT "Preparing deploy and installer files")

    # 9.3. Раскладка файлов по deploy/*
    add_custom_command(TARGET installer POST_BUILD
        # deploy/release (для сервера):
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Uniter>"  "${DEPLOY_RELEASE_DIR}/Uniter.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Updater>" "${DEPLOY_RELEASE_BIN}/Updater.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${BUILD_DIR}/meta.XML"  "${DEPLOY_RELEASE_DIR}/meta.XML"

        # deploy/installer/dist (для инсталлера):
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Uniter>"  "${DIST_INSTALLER_DEPLOY_DIR}/Uniter.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Updater>" "${BIN_INST_DIR}/Updater.exe"

        # deploy/installer/nsis:
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NSIS_INSTALLER_BUILD_DIR}/installer.nsi" "${DEPLOY_INSTALLER_DIR}/nsis/installer.nsi"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NSIS_INSTALLER_BUILD_DIR}/license.txt"   "${DEPLOY_INSTALLER_DIR}/nsis/license.txt"

        # deploy/installer/icons:
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RESOURCES_DIR}/installer/installer_icon.ico"   "${ICON_INSTALLER_DEPLOY_DIR}/installer_icon.ico"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RESOURCES_DIR}/installer/uninstall_icon.ico"   "${ICON_INSTALLER_DEPLOY_DIR}/uninstall_icon.ico"

        # deploy/installer/build_installer.bat:
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RESOURCES_DIR}/installer/build_installer.bat" "${DEPLOY_INSTALLER_DIR}/build_installer.bat"

        COMMENT "Deploying files to deploy/release and deploy/installer"
    )
endif()

#===============================================================================
# 10. УСТАНОВКА И АВТОДЕПЛОЙ Qt DLL
#===============================================================================
include(GNUInstallDirs)
install(TARGETS Uniter DESTINATION ".")
qt_generate_deploy_app_script(TARGET Uniter
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

#===============================================================================
# 11. Автоматический запуск UniterTests после сборки (для всех типов сборки)
#===============================================================================
add_custom_target(run_tests ALL
    COMMAND cmd /C start "" cmd /K "\"$<TARGET_FILE:UniterTests> & echo. & echo Tests finished. & pause\""
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Google Tests (UniterTests) in separate console..."
)

add_dependencies(run_tests UniterTests)
