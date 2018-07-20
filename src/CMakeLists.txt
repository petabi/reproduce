find_library(LIBRDKAFKA_LIB rdkafka)
    add_executable(header2log header2log.c kafka_producer.c)
        target_link_libraries(header2log PRIVATE ${LIBRDKAFKA_LIB})
#install(TARGETS header2log RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})
