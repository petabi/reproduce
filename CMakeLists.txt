cmake_minimum_required(VERSION 3.12)
project(header2log VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

subdirs(src)
