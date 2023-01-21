cmake_minimum_required(VERSION 3.20)

option(TB_BUILD_SAMPLES "" OFF)

include(../toybox/CMakeLists.txt)

# Determine version from the vcpkg json manifest
file(READ vcpkg.json VCPKG_MANIFEST)
string(JSON VCPKG_VERSION GET ${VCPKG_MANIFEST} "version")

project(thehighseas
        VERSION ${VCPKG_VERSION}
        DESCRIPTION "A simple game built on the toybox engine"
        LANGUAGES C CXX)

set(GAME_NAME "thehighseas")
set(GAME_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(GAME_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(GAME_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})

option(FINAL "Compile with the intention to redistribute" OFF)
option(PROFILE_TRACY "Compile with support for the tracy profiler" ON)
option(COOK_ASSETS "Process assets for runtime loading" ON)

file(GLOB_RECURSE source "source/*.c")

tb_add_app(thehighseas ${source})
