cmake_minimum_required(VERSION 3.12)
project(packetproducer VERSION 0.1.0 LANGUAGES CXX)

message("== Project Name: ${PROJECT_NAME}")
message("== Project Version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Release)
endif()
message("== Build Type: ${CMAKE_BUILD_TYPE} (cmake -DCMAKE_BUILD_TYPE=[Release|Debug] .)")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
add_definitions(-DPROGRAM_NAME=\"${PROJECT_NAME}\" -DPROGRAM_VERSION=\"${PROJECT_VERSION}\")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
