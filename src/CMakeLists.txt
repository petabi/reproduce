if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(LIBCPPFS "-lstdc++fs")
endif()

find_package(Threads)
find_path(LIBRDKAFKA_INCLUDE_DIR librdkafka/rdkafkacpp.h)
find_library(LIBRDKAFKA_LIB rdkafka++)
find_library(PCAP_LIB pcap)
find_path(LIBHS_INCLUDE hs/hs.h)
find_library(HS_LIB hs)
add_executable(${PROJECT_NAME} main.cpp controller.cpp converter.cpp producer.cpp config.cpp report.cpp util.cpp matcher.cpp entropy_calculator.cpp sessions.cpp)
add_dependencies(${PROJECT_NAME} libevent)
target_include_directories(${PROJECT_NAME}
  PRIVATE ${LIBRDKAFKA_INCLUDE_DIR} ${LIBHS_INCLUDE} ${CMAKE_SOURCE_DIR}/ext ${PROJECT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRDKAFKA_LIB}
  ${CMAKE_SOURCE_DIR}/target/release/libevent.a
  ${HS_LIB}
  ${LIBCPPFS}
  ${PCAP_LIB}
  ${CMAKE_DL_LIBS}
  Threads::Threads)
#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})

add_library(libreproduce controller.cpp converter.cpp producer.cpp config.cpp report.cpp util.cpp matcher.cpp entropy_calculator.cpp sessions.cpp)
set_target_properties(libreproduce PROPERTIES
  OUTPUT_NAME reproduce
  POSITION_INDEPENDENT_CODE ON)
target_include_directories(libreproduce INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(libreproduce PRIVATE PRIVATE ${LIBRDKAFKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/ext ${PROJECT_BINARY_DIR})
target_link_libraries(libreproduce PRIVATE ${LIBRDKAFKA_LIB} ${PCAP_LIB} ${HS_LIB} ${LIBCPPFS})
