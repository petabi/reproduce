find_library(LIBRDKAFKA_LIB rdkafka)
add_executable(header2log demo.cpp header2log.cpp rdkafka_producer.cpp)
target_link_libraries(header2log PRIVATE ${LIBRDKAFKA_LIB})
#install(TARGETS header2log RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})
