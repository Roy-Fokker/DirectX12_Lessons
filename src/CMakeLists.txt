find_package(fmt)


add_executable(lesson1
    lesson1.cpp
)

target_link_libraries(lesson1
    fmt::fmt
)