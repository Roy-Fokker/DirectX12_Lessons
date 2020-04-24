cmake_minimum_required(VERSION 3.16)

if(DEFINED PROJECT_NAME)
    message(FATAL_ERROR "[Error]: Vcpkg.cmake must be included before the project() directive is first called.")
elseif(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "[Info]: Using VCPKG toolchain file")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# USAGE
# include(cmake/vcpkg.cmake)
# before project function
