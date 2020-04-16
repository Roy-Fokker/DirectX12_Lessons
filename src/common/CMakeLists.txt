add_library(lesson_common INTERFACE)

target_sources(lesson_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/clock.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/clock.h
        ${CMAKE_CURRENT_SOURCE_DIR}/window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/window.h
        ${CMAKE_CURRENT_SOURCE_DIR}/window_implementation.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/d3dx12.h
)

target_include_directories(lesson_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(lesson_common
    INTERFACE
        DEBUG
        _DEBUG
)