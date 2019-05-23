cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(DirectX_12_Lesson
        VERSION 0.0.1
        LANGUAGES CXX
        DESCRIPTION "Lessons in using Direct X 12 application"
)

add_definitions(-DUNICODE)

add_subdirectory(src)
