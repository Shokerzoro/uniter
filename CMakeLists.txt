#===============================================================================
# 1. ОСНОВНЫЕ НАСТРОЙКИ ПРОЕКТА
#===============================================================================
cmake_minimum_required(VERSION 3.22)
project(Uniter LANGUAGES CXX)

# DevKitToolchain.cmake задаёт стандарт, компилятор и пути сам.
# Здесь переопределяем только если toolchain не задал.
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

#===============================================================================
# 2. РЕЖИМЫ СБОРКИ
#===============================================================================
if(CMAKE_BUILD_TYPE MATCHES "^(Debug|Profile)$")
    set(RELEASE     OFF)
    set(DEBUG_INFO  ON)
    set(DEVKIT_QT_DYNAMIC ON)   # Debug: Qt shared + tinyxml2 shared

elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(RELEASE     ON)
    set(DEBUG_INFO  ON)
    set(DEVKIT_QT_DYNAMIC OFF)  # RelWithDebInfo: Qt static + tinyxml2 static

elseif(CMAKE_BUILD_TYPE MATCHES "^Release$")
    set(RELEASE     ON)
    set(DEBUG_INFO  OFF)
    set(DEVKIT_QT_DYNAMIC OFF)  # Release: Qt static + tinyxml2 static

else()
    message(FATAL_ERROR "[Uniter] Unknown CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'. "
                        "Use: Debug | Profile | RelWithDebInfo | Release")
endif()

message(STATUS "[Uniter] Build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "[Uniter] RELEASE       : ${RELEASE}")
message(STATUS "[Uniter] DEBUG_INFO    : ${DEBUG_INFO}")
message(STATUS "[Uniter] Qt dynamic    : ${DEVKIT_QT_DYNAMIC}")

#===============================================================================
# 3. GOOGLE TESTS — остаётся через FetchContent
#===============================================================================
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Отключаем установку gtest как системной библиотеки
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
# Не переопределяем /MT /MD в MSVC (для MinGW не критично, но хорошая практика)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
enable_testing()

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
# 6. ПУТИ ДЛЯ DEPLOY (только RELEASE)
#===============================================================================
if(RELEASE)
    set(DEPLOY_RELEASE_DIR      "${CMAKE_SOURCE_DIR}/deploy/release")
    set(DEPLOY_RELEASE_BIN      "${DEPLOY_RELEASE_DIR}/bin")
    set(DEPLOY_INSTALLER_DIR    "${CMAKE_SOURCE_DIR}/deploy/installer")
    set(INSTALLER_BUILD_DIR     "${CMAKE_BINARY_DIR}/installer")
    set(NSIS_INSTALLER_BUILD_DIR "${INSTALLER_BUILD_DIR}/nsis")
    set(DIST_INSTALLER_DEPLOY_DIR "${DEPLOY_INSTALLER_DIR}/dist")
    set(BIN_INST_DIR            "${DIST_INSTALLER_DEPLOY_DIR}/bin")
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
# 7. ПОДПРОЕКТЫ
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
    configure_file("${RESOURCES_DIR}/installer/installer.nsi.in"
        "${NSIS_INSTALLER_BUILD_DIR}/installer.nsi" @ONLY)
    configure_file("${RESOURCES_DIR}/installer/license.txt"
        "${NSIS_INSTALLER_BUILD_DIR}/license.txt" COPYONLY)

    file(COPY "${RESOURCES_DIR}/installer/"
         DESTINATION "${NSIS_INSTALLER_BUILD_DIR}/"
         FILES_MATCHING PATTERN "*.ico" PATTERN "*.png")

    add_custom_target(installer ALL
        DEPENDS Uniter Updater
        COMMENT "Preparing deploy and installer files")

    add_custom_command(TARGET installer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Uniter>"  "${DEPLOY_RELEASE_DIR}/Uniter.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Updater>" "${DEPLOY_RELEASE_BIN}/Updater.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${BUILD_DIR}/meta.XML"  "${DEPLOY_RELEASE_DIR}/meta.XML"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Uniter>"  "${DIST_INSTALLER_DEPLOY_DIR}/Uniter.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Updater>" "${BIN_INST_DIR}/Updater.exe"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NSIS_INSTALLER_BUILD_DIR}/installer.nsi" "${DEPLOY_INSTALLER_DIR}/nsis/installer.nsi"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NSIS_INSTALLER_BUILD_DIR}/license.txt"   "${DEPLOY_INSTALLER_DIR}/nsis/license.txt"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RESOURCES_DIR}/installer/installer_icon.ico" "${ICON_INSTALLER_DEPLOY_DIR}/installer_icon.ico"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RESOURCES_DIR}/installer/uninstall_icon.ico" "${ICON_INSTALLER_DEPLOY_DIR}/uninstall_icon.ico"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RESOURCES_DIR}/installer/build_installer.bat" "${DEPLOY_INSTALLER_DIR}/build_installer.bat"

        COMMENT "Deploying files to deploy/release and deploy/installer"
    )
endif()

#===============================================================================
# 10. АВТОЗАПУСК ТЕСТОВ ПОСЛЕ СБОРКИ
#===============================================================================
add_custom_target(run_tests ALL
    COMMAND cmd /C start "" cmd /K "\"$<TARGET_FILE:UniterTests> & echo. & echo Tests finished. & pause\""
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Google Tests (UniterTests) in separate console..."
)
add_dependencies(run_tests UniterTests)
