# 3.25 is required for FetchContent_Declare(... SYSTEM) in tests/CMakeLists.txt,
# which is what keeps googletest's warnings out of our build log.
cmake_minimum_required(VERSION 3.25)

# The version is normally injected by CI from the git tag (-DCINES_VERSION=1.2.3).
# The fallback keeps local configure working on a fresh clone.
if(NOT DEFINED CINES_VERSION)
  set(CINES_VERSION "0.0.0")
endif()

project(Cines
  VERSION ${CINES_VERSION}
  DESCRIPTION "CINES - a two player chess game"
  HOMEPAGE_URL "https://github.com/sanketsudake/CHESS-in-Qt"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()

option(CINES_BUILD_TESTS "Build the test suite" ON)

# The rules library needs no Qt, so the jobs that only exercise it -- coverage
# and the sanitizers -- can skip installing Qt entirely and stay fast.
option(CINES_BUILD_APP "Build the Qt application" ON)
option(CINES_SLOW_TESTS "Include the deep perft levels (minutes, not seconds)" OFF)
option(CINES_WERROR "Treat compiler warnings as errors" OFF)

# Applied to every target we own. Third-party code pulled in by FetchContent
# does not link this, so its warnings stay out of our build log.
add_library(cines_warnings INTERFACE)
if(MSVC)
  target_compile_options(cines_warnings INTERFACE /W4 $<$<BOOL:${CINES_WERROR}>:/WX>)
else()
  target_compile_options(cines_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    $<$<BOOL:${CINES_WERROR}>:-Werror>)
endif()

add_subdirectory(src/core)

if(CINES_BUILD_APP)
  add_subdirectory(src/app)
endif()

if(CINES_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
