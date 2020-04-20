find_package(fmt REQUIRED)
find_package(cppitertools CONFIG REQUIRED)

add_executable(lesson2)

target_sources(lesson2
    PRIVATE
        main.cpp
        dx_wrapped_types.h
        directx12.cpp
        directx12.h
        cmd_queue.cpp
        cmd_queue.h
        draw_cube.cpp
        draw_cube.h
        gpu_resource.cpp
        gpu_resource.h)

target_link_libraries(lesson2
    PRIVATE
        project_configuration
        lesson_common
        fmt::fmt
        cppitertools::cppitertools
        d3d12.lib
        dxgi.lib)