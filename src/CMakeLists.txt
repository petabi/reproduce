find_path(LIBRDKAFKA_INCLUDE_DIR librdkafka/rdkafkacpp.h)
find_library(LIBRDKAFKA_LIB rdkafka++)
find_library(PCAP_LIB pcap)
add_executable(${PROJECT_NAME} main.cpp controller.cpp converter.cpp producer.cpp config.cpp report.cpp util.cpp)
target_include_directories(${PROJECT_NAME}
  PRIVATE ${LIBRDKAFKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/ext ${PROJECT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRDKAFKA_LIB} ${PCAP_LIB})
#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})
