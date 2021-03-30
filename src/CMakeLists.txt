if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(LIBCPPFS "-lstdc++fs")
endif()

find_package(Threads)
find_path(PCAP_INCLUDE_DIR pcap/pcap.h)
find_library(PCAP_LIB pcap)
find_library(HS_LIB hs)
add_executable(${PROJECT_NAME} main.cpp controller.cpp util.cpp)
add_dependencies(${PROJECT_NAME} libevent)
target_include_directories(${PROJECT_NAME}
  PRIVATE ${CMAKE_SOURCE_DIR}/ext ${PROJECT_BINARY_DIR} ${PCAP_INCLUDE_DIR})
message("Hyperscan ${HS_LIB}")
if(HS_LIB STREQUAL "HS_LIB-NOTFOUND")
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/target/release/libevent.a
    ${LIBCPPFS}
    ${PCAP_LIB}
    ${CMAKE_DL_LIBS}
    Threads::Threads)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/target/release/libevent.a
    ${HS_LIB}
    ${LIBCPPFS}
    ${PCAP_LIB}
    ${CMAKE_DL_LIBS}
    Threads::Threads)
endif()
