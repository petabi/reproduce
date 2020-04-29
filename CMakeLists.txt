cmake_minimum_required(VERSION 3.10)
project(reproduce VERSION 0.9.8 LANGUAGES CXX)
message("== Project Name: ${PROJECT_NAME}")
message("== Project Version: ${PROJECT_VERSION}")
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Release)
endif()
message("== Build Type: ${CMAKE_BUILD_TYPE} (cmake -DCMAKE_BUILD_TYPE=[Release|Debug] .)")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/version.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(src)

include(ExternalProject)

ExternalProject_Add(libevent
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND cargo build --release
  BINARY_DIR "${CMAKE_SOURCE_DIR}"
  INSTALL_COMMAND ""
  BUILD_ALWAYS ON
  LOG_BUILD ON)

find_package(GTest)
if(GTEST_FOUND)
  enable_testing()
  add_subdirectory(tests)
endif()
