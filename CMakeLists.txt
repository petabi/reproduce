cmake_minimum_required(VERSION 3.12)
project(header2log VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
