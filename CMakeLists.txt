cmake_minimum_required(VERSION 3.12)

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


# "-DCMAKE_TOOLCHAIN_FILE=D:/Projects/CPP/.packages/vcpkg/scripts/buildsystems/vcpkg.cmake"