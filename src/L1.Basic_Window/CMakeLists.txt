find_package(fmt REQUIRED)
find_package(cppitertools CONFIG REQUIRED)

add_executable(lesson1)

target_sources(lesson1
    PRIVATE
        main.cpp
        directx12.cpp
        directx12.h
)

target_link_libraries(lesson1
    PRIVATE
        project_configuration
        lesson_common
        fmt::fmt
        cppitertools::cppitertools
        d3d12.lib
        dxgi.lib)