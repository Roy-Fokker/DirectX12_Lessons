cmake_minimum_required(VERSION 3.17)

include(cmake/vcpkg.cmake)
include(cmake/msvc_settings.cmake)

project(DirectX_12_Lesson
        VERSION 0.0.1
        LANGUAGES CXX
        DESCRIPTION "Lessons in using Direct X 12 application"
)

add_library(project_configuration INTERFACE)
set_msvc_project_configuration(project_configuration)
target_compile_features(project_configuration INTERFACE cxx_std_17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin/")

add_subdirectory(src)
