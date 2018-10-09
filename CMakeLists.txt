cmake_minimum_required(VERSION 3.12)
project(packetproducer VERSION 0.5.0 LANGUAGES CXX)

message("== Project Name: ${PROJECT_NAME}")
message("== Project Version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Release)
endif()
message("== Build Type: ${CMAKE_BUILD_TYPE} (cmake -DCMAKE_BUILD_TYPE=[Release|Debug] .)")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/version.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
