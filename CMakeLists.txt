cmake_minimum_required(VERSION 3.20)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Determine version from the vcpkg json manifest
file(READ vcpkg.json VCPKG_MANIFEST)
string(JSON VCPKG_VERSION GET ${VCPKG_MANIFEST} "version")

project(thehighseas
        VERSION ${VCPKG_VERSION}
        DESCRIPTION "A simple game built on the toybox engine"
        LANGUAGES C CXX)

option(FINAL "Compile with the intention to redistribute" OFF)
option(PROFILE_TRACY "Compile with support for the tracy profiler" ON)
option(COOK_ASSETS "Process assets for runtime loading" ON)

set(GAME_NAME "thehighseas")
set(GAME_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(GAME_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(GAME_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})

set(ENGINE_NAME "thehighseas")
set(ENGINE_VERSION_MAJOR 0)
set(ENGINE_VERSION_MINOR 1)
set(ENGINE_VERSION_PATCH 0)

set(config $<CONFIG>)

# Get target CPU architecture
include(${CMAKE_CURRENT_LIST_DIR}/cmake/TargetArch.cmake)
target_architecture(arch)

# Platform detection
if(WIN32)
  set(platform "windows")
elseif(APPLE)
  if(IOS)
    set(platform "ios")
  else()
    set(platform "macos")
  endif()
elseif(ANDROID)
  set(platform "android")
elseif(UNIX)
  set(platform "linux")
else()
  message(FATAL_ERROR "Unknown platform")
endif()

# Host detection
if(CMAKE_HOST_WIN32)
  set(host "windows")
elseif(CMAKE_HOST_UNIX)
  if(CMAKE_HOST_APPLE)
    set(host "osx")
  else()
    set(host "linux")
  endif()
else()
message(FATAL_ERROR "Unknown host")
endif()

if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "AMD64")
  set(host_arch "x64")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(host_arch "x64")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(host_arch "arm64")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
  set(host_arch "arm64")
else()
  message(FATAL_ERROR "Unknown host arch: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

set(host_triple ${host_arch}-${host})

# Set install prefix if not already overridden
set(thehighseas_install "${CMAKE_CURRENT_SOURCE_DIR}/install/${platform}")
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX ${thehighseas_install} CACHE PATH "Default Install Path" FORCE )
else()
    set(thehighseas_install ${CMAKE_INSTALL_PREFIX})
endif()

# Clang & GNU only for now
if(NOT ${CMAKE_C_COMPILER_ID} MATCHES "Clang" AND
   NOT ${CMAKE_C_COMPILER_ID} MATCHES "AppleClang" AND
   NOT ${CMAKE_C_COMPILER_ID} MATCHES "GNU")
  message(FATAL_ERROR "${CMAKE_C_COMPILER_ID} Unsupported - Clang & GNU only")
endif()

if(${VCPKG_TARGET_TRIPLET} MATCHES static OR ${VCPKG_TARGET_TRIPLET} MATCHES android)
  set(STATIC ON)
endif()


add_executable(thehighseas)

file(GLOB_RECURSE source "source/*.c")
target_sources(thehighseas ${source})

target_link_libraries(thehighseas PRIVATE toybox)