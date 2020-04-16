find_package(fmt REQUIRED)
find_package(cppitertools CONFIG REQUIRED)

add_executable(lesson2)

target_sources(lesson2
    PRIVATE
        main.cpp
        directx12.cpp
        directx12.h
)

target_link_libraries(lesson2
    PRIVATE
        project_configuration
        lesson_common
        fmt::fmt
        cppitertools::cppitertools
        d3d12.lib
        dxgi.lib)