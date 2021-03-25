if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(LIBCPPFS "-lstdc++fs")
endif()

find_package(Threads)
find_path(PCAP_INCLUDE_DIR pcap/pcap.h)
find_library(PCAP_LIB pcap)
find_library(HS_LIB hs)
add_executable(${PROJECT_NAME} main.cpp controller.cpp converter.cpp producer.cpp report.cpp util.cpp)
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
#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})

add_library(libreproduce controller.cpp converter.cpp producer.cpp report.cpp util.cpp)
set_target_properties(libreproduce PROPERTIES
  OUTPUT_NAME reproduce
  POSITION_INDEPENDENT_CODE ON)
target_include_directories(libreproduce INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(libreproduce PRIVATE PRIVATE ${CMAKE_SOURCE_DIR}/ext ${PROJECT_BINARY_DIR} ${PCAP_INCLUDE_DIR})
if(HS_LIB STREQUAL "HS_LIB-NOTFOUND")
  target_link_libraries(libreproduce PRIVATE
    ${CMAKE_SOURCE_DIR}/target/release/libevent.a
    ${PCAP_LIB} ${LIBCPPFS})
else()
  target_link_libraries(libreproduce PRIVATE
    ${CMAKE_SOURCE_DIR}/target/release/libevent.a
    ${PCAP_LIB} ${HS_LIB} ${LIBCPPFS})
endif()
