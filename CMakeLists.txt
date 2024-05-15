cmake_minimum_required (VERSION 3.15)

project ("proxyswiss")

set(Boost_USE_MULTITHREADED ON)

IF (CMAKE_BUILD_TYPE STREQUAL "Release")
  # Release
ELSE()
ENDIF()

IF (WIN32)
  # Windows
  MESSAGE(CHECK_START "[ Windows ]")
  add_definitions(/D_WIN32_WINNT=0x0601) # Win7
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
ELSE()
  # Linux/Mac
  IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
      # Linux and MAC, Debug
      MESSAGE(CHECK_START "[ Linux/Mac, Debug ]")
  ELSEIF(CMAKE_BUILD_TYPE STREQUAL "Release")
      # Linux and Mac, Release
      MESSAGE(CHECK_START "[ Linux/Mac, Release ]")
  ELSE()
      MESSAGE(CHECK_START "[ Linux/Mac, Release-based ]")
  ENDIF()
ENDIF()



# Boost
find_package(Boost 1.74.0 REQUIRED COMPONENTS date_time filesystem system thread context coroutine)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
include_directories(${Boost_INCLUDE_DIRS})

include_directories(src/)

add_subdirectory(src/common)
add_subdirectory(src/proxy)
add_subdirectory(src/proxyswiss)
