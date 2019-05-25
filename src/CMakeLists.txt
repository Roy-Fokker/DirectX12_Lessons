find_package(fmt)

add_executable(lesson1
    lesson1.cpp
)

target_compile_options(lesson1 PUBLIC
    "/permissive-"
)

target_compile_definitions(lesson1 PRIVATE
    DEBUG
    _DEBUG
)

target_link_libraries(lesson1
    fmt::fmt
    d3d12.lib
    dxgi.lib
)