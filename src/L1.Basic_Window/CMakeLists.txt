find_package(fmt REQUIRED)
find_package(cppitertools CONFIG REQUIRED)


add_executable(lesson1)

target_sources(lesson1
    PRIVATE
        main.cpp
        directx12.cpp
        directx12.h
        window.cpp
        # window.h
        # window_implementation.inl
        # d3dx12.h
)

target_include_directories(lesson1
    PRIVATE
        ${LESSON_COMMON_SRC_DIR}
)

target_link_libraries(lesson1
    PRIVATE
        project_configuration
        fmt::fmt
        cppitertools::cppitertools
        d3d12.lib
        dxgi.lib)