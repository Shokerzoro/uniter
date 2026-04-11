project(uniter_common LANGUAGES CXX)

add_library(uniter_common STATIC
    appfuncs.cpp
)

target_include_directories(uniter_common
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

# Qt для uniter_common:
#   Debug  → shared (DLL), Release / RelWithDebInfo → static
if(DEVKIT_QT_DYNAMIC)
    devkit_use_qt_shared(uniter_common Core)
else()
    devkit_use_qt_static(uniter_common Core)
endif()

# devkit_finalize() нужен т.к. AUTOMOC установлен devkit_use_qt_*
devkit_finalize(uniter_common)

# Макросы отладки / релиза
if(RELEASE AND NOT DEBUG_INFO)
    target_compile_definitions(uniter_common PRIVATE
        QT_NO_DEBUG_OUTPUT
        RELEASE_BUILD)
elseif(RELEASE AND DEBUG_INFO)
    target_compile_definitions(uniter_common PRIVATE
        RELEASE_BUILD
        DEBUG_BUILD)
else()
    target_compile_definitions(uniter_common PRIVATE
        DEBUG_BUILD)
endif()

target_compile_features(uniter_common PUBLIC cxx_std_17)
