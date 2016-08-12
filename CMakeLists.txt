cmake_minimum_required(VERSION 3.0)
project(Gmodel LANGUAGES CXX)

#trying to force BUILD_TESTING to default to OFF
set(BUILD_TESTING OFF CACHE STRING "Build and run tests")

include(CTest)
enable_testing()

set(GMOD_MAJOR "1")
set(GMOD_MINOR "3")
set(GMOD_PATCH "0")

option(GMOD_OPTIMIZE "Compile with optimization" ON)
option(GMOD_SYMBOLS "Compile with debug symbols" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(GMOD_SANITIZE_ADDRESS "Use -fsanitize=address" OFF)

set(FLAGS "--std=c++11")
if(GMOD_OPTIMIZE)
  set(FLAGS "${FLAGS} -O2")
else()
  set(FLAGS "${FLAGS} -O0")
endif()
if(GMOD_SYMBOLS)
  set(FLAGS "${FLAGS} -g")
endif()
if(GMOD_SANITIZE_ADDRESS)
  set(FLAGS "${FLAGS} -fsanitize=address")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(FLAGS "${FLAGS} -fno-omit-frame-pointer -ferror-limit=1")
  set(FLAGS "${FLAGS} -Werror -Weverything -Wno-c++98-compat")
  set(FLAGS "${FLAGS} -Wno-c++98-compat-pedantic -Wno-padded")
  set(FLAGS "${FLAGS} -Wno-float-equal")
  set(FLAGS "${FLAGS} -Wno-weak-template-vtables")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(FLAGS "${FLAGS} -fno-omit-frame-pointer -fmax-errors=1")
  set(FLAGS "${FLAGS} -Werror -Wall -Wextra")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
else()
  message(FATAL_ERROR "Unexpected compiler type ${CMAKE_CXX_COMPILER_ID}")
endif()
set(CMAKE_CXX_FLAGS "${FLAGS}")

# CMake RPATH "always full" configuration, see:
# https://cmake.org/Wiki/CMake_RPATH_handling#Always_full_RPATH
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH False)
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH False)
# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)

add_library(gmodel gmodel.cpp)
target_include_directories(gmodel INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

add_subdirectory(tests)

install(FILES
  "${PROJECT_SOURCE_DIR}/gmodel.hpp"
  DESTINATION include)
install(TARGETS gmodel EXPORT gmodel-targets DESTINATION lib)
install(EXPORT gmodel-targets DESTINATION lib)
configure_file(
  "${PROJECT_SOURCE_DIR}/gmodel-config.cmake.in"
  "${PROJECT_BINARY_DIR}/gmodel-config.cmake"
  @ONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/gmodel-config-version.cmake.in"
  "${PROJECT_BINARY_DIR}/gmodel-config-version.cmake"
  @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/gmodel-config.cmake"
  "${PROJECT_BINARY_DIR}/gmodel-config-version.cmake"
  DESTINATION lib/gmodel)
