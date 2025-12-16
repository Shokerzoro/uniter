project(uniter_common LANGUAGES CXX)

add_library(uniter_common STATIC
    appfuncs.cpp
)

target_include_directories(uniter_common
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(uniter_common
    PUBLIC
        Qt6::Core
)

# Логика вывода/макросов
if(RELEASE AND NOT DEBUG_INFO)
    # Чистый релиз: отключаем qDebug
    target_compile_definitions(uniter_common PRIVATE
        QT_NO_DEBUG_OUTPUT
        RELEASE_BUILD)
elseif(RELEASE AND DEBUG_INFO)
    # RelWithDebInfo: релизная сборка, но с дебаг-инфой и логами
    target_compile_definitions(uniter_common PRIVATE
        RELEASE_BUILD
        DEBUG_BUILD)
else()
    # Debug / Profile: всё дебажное включено
    target_compile_definitions(uniter_common PRIVATE
        DEBUG_BUILD)
endif()

target_compile_features(uniter_common PUBLIC cxx_std_17)
